#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include "bint.h"
#include "test_bint.c"
#include <ctype.h>

// 테스트벡터는 256비트 (8워드) 기준
// main.c에서만 사용할 것이기 때문에 static으로 정의. 이렇게 하면 같은 이름을 사용한다 하더라도 충돌이 발생하지 않는다! (local)

// 32비트 워드 랜덤(의사난수..선형합동함수-예측이 쉽다!) 생성 - 윈도우에서는 15비트이기 때문에 5번 합성 => 32비트)
static uint32_t rand32(void) {
    uint32_t v = 0;
    for (int i = 0; i < 5; ++i) v = (v << 7) ^ (uint32_t)(rand() & 0x7F); // 0x7F == 0b01111111 
    // 호출한 난수열으로부터 하위 7비트만을 추출하고 5회 반복하여 총 35비트길이의 의사난수 완성.
    // n회 호출하여 이어 붙이기만 했기 때문에(길이 확장을 목적으로) 엔트로피는 증가하지 않을 것임...
    // 최종 결과가 uint32_t로 계산/저장되므로 상위 3비트는 자연스럽게 버려지고 하위 32비트만 남게된다.
    return v;
}

// 총 8워드 중 carry를 컨트롤하기 위해서는 최상위 워드만 봐주면 됨. arr[7]
// 최종 carry는 최상위 워드 합과 하위워드에서 발생한 carry에 의해 결정되기 때문.
// 하위 7워드는 자유롭게 랜덤 생성
static void gen_lower_words(WORD a[8], WORD b[8]) {
    for (int i = 0; i < 7; ++i) { // 하위워드는 7개. 위 함수 호출해서 채워준다
        a[i] = (WORD)rand32();
        b[i] = (WORD)rand32();
    }
}

// 하위워드에서 MSW로 올라오는 carry_in 계산
static uint32_t carry_into_msw(const WORD a[8], const WORD b[8]) {
    uint64_t carry = 0; // 다음 워드로 전이되는 carry는 
    // carry[i] == a[i] + b[i] + carry[i-1]의 최상위비트이므로 32비트로는 담을 수 없음(0xFFFFFFFF끼리의 연산 경우)
    // 주의할 점은 워드간의 자리올림과 비트단위의 carry는 다른것이므로 구분할 것
    for (int i = 0; i < 7; ++i) {
        uint64_t s = (uint64_t)a[i] + b[i] + carry;
        carry = (s >> 32);
    }
    return (uint32_t)carry; // MSW에 유입되는 캐리는 마지막에 발생하는 캐리임
}

// 최종 캐리 컨트롤 함수
// 랜덤하게 비트열을 생성하므로 carry가 발생하는 경우, 발생하지 않는 경우도 랜덤이니 테스트벡터위해 별도로 컨트롤
static void pick_msw_for_target(WORD a[8], WORD b[8], int want_final_carry) { // want_final_carry가 타겟 캐리이다. 발생/발생하지않음(1/0)
    for (;;) { // 난수열에 의해 캐리가 결정되므로 특성상 내가 원하는 조건을 만족할때까지 계속해서 시행해야함
        a[7] = (WORD)rand32(); // 우선 랜덤
        uint32_t cin = carry_into_msw(a, b); // cin == 최상위워드로 올라오는 캐리비트(carry_into_msw)
        if (!want_final_carry) { // 캐리가 발생하길 원치 않는경우,
            // a7 + b7 + cin < 2^32가 되도록 b7 범위 제한
            // 위 부등식은 a7 + b7 + cin < 0xFFFFFFFF 와 동치이며, b7에 대해 정리하면
            // b7 <= 0xFFFFFFFF - a7 - cin
            // 이 경계값을 max_b로 두고 b7을 0~max_b중에서 선택하면 반드시 캐리가 발생하지 않는다.
            // 단 이 경계값이 음수인 경우 b7이 unsigned이면서 b7 <= 음수인 경우는 존재하지 않으므로 다시시행해야함
            uint64_t max_b = (uint64_t)0xFFFFFFFFu - a[7] - cin;
            if ((int64_t)max_b >= 0) { // max_b 즉 경계값이 음수가 아니면 
                // 0~max_b에서 임의 선택
                uint32_t r = rand32();
                b[7] = (WORD)((max_b == 0) ? 0u : (r % (uint32_t)(max_b + 1))); // max_b가 0이면 선택지는 0 하나뿐임.
                // 그 외의 경우에는 모듈로 연산 r % N의 결과 범위는 0~N-1이므로 범위 0~max_b에 대해 r % max_b+1
                // 검증
                uint64_t sum = (uint64_t)a[7] + b[7] + cin;
                if ((sum >> 32) == 0) return; 
            }
            // 불가능하면 다시 시도
        }
        else { // 캐리가 발생하길 원하는 경우
            // a7 + b7 + cin >= 2^32 되도록 b7 하한 설정
            int64_t min_b = (int64_t)0x100000000ULL - (int64_t)a[7] - (int64_t)cin; // [min_b .. 0xFFFFFFFF]
            if (min_b < 0) min_b = 0; // 이미 큰 a7이면 어떤 b7를 선택해도도 캐리발생
            if (min_b <= 0xFFFFFFFF) { // note : 32비트 최댓값을 넘어서는 경우 다시시도
                uint32_t width = 0xFFFFFFFFu - (uint32_t)min_b + 1u; // 선택가능한 범위의 수: 0xFFFFFFFF − min_b + 1
                uint32_t r = rand32();
                b[7] = (WORD)((uint32_t)min_b + (width == 0 ? 0u : (r % width))); // 경계값을 넘어서야하므로 min_b 를 더해주기. r % width로 유효범위 내에서 랜덤선택
                // 검증
                uint64_t sum = (uint64_t)a[7] + b[7] + cin;
                if ((sum >> 32) == 1) return; // 캐리 확정
            }
            // 불가능하면 다시 시도
        }
    }
}

// 벡터 1개 생성: want_final_carry=0/1
static void gen_256_add_vector(WORD a[8], WORD b[8], int want_final_carry) {
    gen_lower_words(a, b);
    pick_msw_for_target(a, b, want_final_carry);
}

// WORD[8] -> bint로 세팅
static void set_bint_256(BINT** out, const WORD w[8]) { // 읽기전용 - const
    set_bint_from_word_array(out, (WORD*)w, 8);
    normalize_wordlen(*out);
}

// 기대 캐리 검증: 수동 최상위 합 확인
static int expected_final_carry(const WORD a[8], const WORD b[8]) {
    uint64_t carry = 0;
    for (int i = 0; i < 8; ++i) {
        uint64_t s = (uint64_t)a[i] + b[i] + carry;
        carry = (s >> 32);
    }
    return (int)carry; // 0 또는 1
}

// 보기 좋게 출력
static void print_words_hex(const char* label, const WORD w[8]) {
    printf("%s 0x", label);
    for (int i = 7; i >= 0; --i) printf("%08x", (unsigned)w[i]); // 배열은 리틀엔디언으로 저장되어있으므로 출력은 반대로
    printf("\n");
}

// 256-bit 덧셈 테스트 실행: 캐리 3개, 미캐리 2개
static void run_256bit_add_tests(void) {
    srand(12345); // note : srand로 시드를 지정하면 이후에 실행되는 rand는 동일시드로 실행된다. 
    // 실패가 항상 같은 시드로부터 발생하므로 재현성이 구현되어 원인추적이 용이하다.

    // 목표 시나리오: 1=캐리o, 0=캐리x
    int targets[5] = { 1, 1, 1, 0, 0 };

    for (int t = 0; t < 5; ++t) {
        WORD A[8] = { 0 }, B[8] = { 0 };
        gen_256_add_vector(A, B, targets[t]); // 최상위워드 조정해서 A, B 생성

        // 덧셈을 실제 수행하기 전에 A+B의 이론상 캐리가 목표하고자 하는 캐리와 동일한지 (테스트벡터가 잘 생성 되었는지) 검증
        int exp_carry = expected_final_carry(A, B);
        if (exp_carry != targets[t]) {
            fprintf(stderr, "생성기 오류: 기대 캐리 %d != 실제 %d\n", targets[t], exp_carry);
            exit(1);
        }

        // BINT 세팅 및 덧셈
        BINT* a = NULL, * b = NULL, * sum = NULL;
        set_bint_256(&a, A);
        set_bint_256(&b, B);
        add_bint(&sum, a, b); 

        // 출력
        printf("\n[Case %d] target carry=%d\n", t + 1, targets[t]);
        print_words_hex("A =", A);
        print_words_hex("B =", B);
        printf("Result of addition: ");
        print_bint_hex(sum);

        
        // 덧셈 이후에 기대 캐리가 실제 계산한 바와 같은지 검증
        int got_carry = expected_final_carry(A, B);
        if (got_carry != targets[t]) {
            fprintf(stderr, "검증 실패: 계산된 캐리 %d, 목표 %d\n", got_carry, targets[t]);
            exit(1);
        }

        free_bint(&a);
        free_bint(&b);
        free_bint(&sum);
    }
    printf("\n== 256-bit addition tests completed ==\n");
}

// gpt피셜 "입력을 사실상 무제한으로 받을 수 있는 함수"
static char* read_token_unbounded(FILE* in) {
    // note : FILE 구조체에 대하여:
    // "FILE*는 C 표준 입출력 라이브러리(<stdio.h>)가 제공하는 스트림 핸들의 포인터"
    // 내부적으로 버퍼, 현재 파일 위치, 에러/EOF 상태 등을 담은 구조체(FILE)가 있다!


    size_t cap = 1024, len = 0; // 초기 용량 1024바이트, 필요 시 2배씩 증가. len은 현재까지 담은 문자 수(0 ≤ len < cap)
    // 엄밀히 cap은 동적 버퍼 buf에 현재 할당되어 있는 총 용량(수용 가능한 문자 수) -> 최대 '\0' 포함 몇 글자까지 담을 수 있는지?
    char* buf = (char*)malloc(cap);  // buf는 토큰을 연속적으로 담기 위한 작업 버퍼이다.
    
    // 메모리 할당 실패 경우 처리
    if (!buf) { fprintf(stderr, "alloc fail\n"); exit(1); }

    int c;
    // isspace로 판단되는 모든 선행 공백 제거
    do {
        c = fgetc(in);        // note : fgetc는 "unsigned char"로 해석한 뒤 int로 반환하며, 오류 발생 시 EOF를 리턴
        // 하나의 비부호화 문자를 읽고, 다음 문자를 가리키도록 포인터 값을 증가시킨다.
        // 즉 한 글자 한 글자씩 읽는다.
        if (c == EOF) { free(buf); return NULL; } // 파일 끝 혹은 오류발생 시 루프 탈출
    } while (isspace(c)); // 공백이 아닐 때까지 반복

    // note : 토큰은“공백이 아닌 문자들의 최대 연속 구간”, 버퍼는 데이터를 임시로 저장하는 연속 메모리 영역
    // 입력에서 다음 토큰의 길이를 미리 알 수 없기 때문에 동적 버퍼를 이용해야한다.
    // 무엇을 한 덩어리로 읽을지 정하려면 “경계”가 필요하고, 여기서는 공백을 경계로 한 토큰 단위를 사용
    
    for (;;) {
        if (len + 1 >= cap) { // 다음 문자 + 종단문자('\0')를 담을 공간 확인. 모자란 경우
            size_t ncap = cap << 1; // 용량을 2배로
            char* t = (char*)realloc(buf, ncap); // 메모리도 재할당
            
            // 메모리 할당 실패 경우
            if (!t) { free(buf); fprintf(stderr, "realloc fail\n"); exit(1); }
            
            
            cap = ncap; buf = t;  // 새 버퍼로 갱신
        }
        buf[len++] = (char)c;  // 현재 문자 저장

        c = fgetc(in);  // 다음 문자 읽기
        if (c == EOF || isspace((unsigned char)c)) break;  // 파일 끝 또는 공백이면 토큰 종료
        // isspace에 넘기는 값은 반드시 EOF 또는 unsigned char로 표현 가능한 값이어야한다. 내부적으로 0..255 인덱스를 갖는 테이블 조회를 많이 사용하기 때문
    }
    buf[len] = '\0';   // NUL 종단
    return buf;  // 결론은 동적 할당된 C 문자열 char* 출력
}
// 요약
// fgetc(in)으로 한 글자씩 읽고
// 공백은 건너뛴 뒤, 다음 공백 또는 EOF가 나올 때까지 토큰을 동적 버퍼에 누적
// 누적 중 용량이 차면 realloc()으로 2배씩 키워 사실상 무제한 길이를 수용
// 결과를 널 종료한 char*로 반환





void print_add_bint(BINT* a, BINT* b) { // add 호출
    BINT* result = NULL;
    add_bint(&result, a, b);
    printf("Result of addition: ");
    print_bint_hex(result);  
    free_bint(&result);      
}

void print_sub_bint(BINT* a, BINT* b) {
    BINT* result = NULL;
    sub_bint(&result, a, b);     
    printf("Result of subtraction: ");
    print_bint_hex(result);      
    free_bint(&result);
}






void cmp_bint_example(BINT* a, BINT* b) {
    int cmp_result = cmp_bint(a, b);  
    if (cmp_result > 0) {
        printf("a is greater than b\n");
    }
    else if (cmp_result < 0) {
        printf("a is less than b\n");
    }
    else {
        printf("a is equal to b\n");
    }
}


int main() {
    printf("== BINT UNIT TEST SUITE ==\n");

    test_init_alloc();
    printf("[OK] test_init_alloc passed\n");

    test_free_bint_nullsafe();
    printf("[OK] test_free_bint_nullsafe passed\n");

    test_set_bint_from_uint64();
    printf("[OK] test_set_bint_from_uint64 passed\n");

    test_set_bint_from_word_array_basic();
    printf("[OK] test_set_bint_from_word_array_basic passed\n");

    test_add_unsigned_basic();
    printf("[OK] test_add_unsigned_basic passed\n");

    test_add_unsigned_with_carry();
    printf("[OK] test_add_unsigned_with_carry\n");

    test_add_bint();
    printf("[OK] test_add_bint_with_carry passed\n");

    // test_add_bint_small_carry_extend();
    // printf("[OK] test_add_bint_small_carry_extend passed\n");

    // test_mul_bint_small_basic();
    // printf("[OK] test_mul_bint_small_basic passed\n");

    test_bint_is_zero_cases();
    printf("[OK] test_bint_is_zero_cases passed\n");

    test_cmp_bint_basic();
    printf("[OK] test_cmp_bint_basic passed\n");

    test_normalize_wordlen_trim();
    printf("[OK] test_normalize_wordlen_trim passed\n");

    test_sub_unsigned_basic();
    printf("[OK] test_sub_unsigned_basic passed\n");
    
    //// 배열 뜯어 어디가 문제인지 확인해보기
    //BINT* a = NULL, * b = NULL, * res = NULL;
    //WORD aa[] = { 0x1 };
    //WORD bb[] = { 0x2, 0x3, 0x4 };
    //set_bint_from_word_array(&a, aa, 1); 
    //set_bint_from_word_array(&b, bb, 3);
    //add_unsigned(&res, a, b);
    //print_bint_hex(a);
    //print_bint_hex(b);
    //print_bint_hex(res);
    //for (size_t i = 0; i < b->wordlen; ++i) {
    //    printf("b->val[%zu] = 0x%08x\n", i, b->val[i]);
    //}

    printf("\n== ALL TESTS COMPLETED ==\n");

    BINT* bint_a = NULL;
    BINT* bint_b = NULL;
    
    run_256bit_add_tests();

    printf("Input for first BINT (hex: 0x..., or decimal):\n");
    char* tok_a = read_token_unbounded(stdin);
    if (!tok_a) { fprintf(stderr, "입력 종료(첫 번째 값 없음)\n"); return 1; }

  
    set_bint_from_str(&bint_a, tok_a);
    free(tok_a);

    printf("Input for second BINT (hex: 0x..., or decimal):\n");
    char* tok_b = read_token_unbounded(stdin);
    if (!tok_b) {
        fprintf(stderr, "입력 종료(두 번째 값 없음)\n");
        free_bint(&bint_a);
        return 1;
    }

    set_bint_from_str(&bint_b, tok_b);
    free(tok_b);

    // 최종 방어: 파싱 실패 등으로 NULL이 남아있으면 중단
    if (!bint_a || !bint_b) {
        fprintf(stderr, "파싱 실패: BINT가 생성되지 않았습니다.\n");
        free_bint(&bint_a);
        free_bint(&bint_b);
        return 1;
    }

    
    print_add_bint(bint_a, bint_b);  
    print_sub_bint(bint_a, bint_b);  
    cmp_bint_example(bint_a, bint_b); 

    free_bint(&bint_a);
    free_bint(&bint_b);

 


    
    return 0;
}

