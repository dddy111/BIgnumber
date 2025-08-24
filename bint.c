#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "bint.h"


// wordlen 정제해주는 함수. (wordlen을 계산해주는 함수는 아님)
void normalize_wordlen(BINT* b) {
    if (b == NULL || b->val == NULL) return; // BINT 포인터 자체가 NULL이거나 배열이 NULL이면 NULL에 접근하게되는것이니 오류발생

    size_t i = b->wordlen;  // 실제 유효 워드 범위까지만 확인(alloc이 아닌 wordlen) alloc을 사용하면 불필요한 인덱스까지 보게된다.(wordlen보다 큰값이니)
    while (i > 1 && b->val[i - 1] == 0) { // 최상위인덱스부터 하위비트 방향으로 0이있으면 인덱스를 감소시켜서
        i--; // 0이 아닌값이 저장돼있는 인덱스부분에서 루프를 멈춰서
    }
    b->wordlen = i;  // 인덱스를 wordlen으로 취하게 하면 정제완료
}
// == > 일단 이 프로젝트 전반에서 0에 대한 표준은 wordlen이 "1"이면서 배열의 첫번째 값이 0일 것을 기준으로 함..
// 이 함수는 normalize 대상의 길이가 1 이상일때만 작동하므로, wordlen이 0인 입력을 받으면 1의 길이를 보장해주지 않음.
// wordlen == 0을 허용하게되면, for (ssize_t i = wordlen - 1; i >= 0; --i) 와 같은 코드에서 언더플로우가 발생함.
// 때문에 길이보정은 호출하는 쪽에서 필수적으로 해주어야 한다.



// alloc 계산 함수
size_t calc_alloc_from_str10(const char* str) {
    if (str == NULL) return 0;  // 포인터가 NULL이면 NULL역참조 문제 발생

    size_t len = strlen(str);  // 문자 길이 구함

    // 10진수 9자리는 대략 1개의 32비트 워드를 차지한다.
     // 10^9 ~= 4.29 * 10^9
    size_t approx_words = (len / 9) + 1;  // 그러므로 문자열 길이를 9로 나눈 값만큼 워드가 필요
    // 단 10/9 같은 케이스는 연산 결과는 1이지만 나머지 1/9부분을 저장할 공간도 더 필요하니
    // 워드를 하나만큼 더 잡아주어야한다.
    return approx_words + 1; // 연산결과 자릿수 늘어나는거까지 고려해서 거기에 하나 더잡는다.
}


void init_bint(BINT** pptr, size_t alloc) { // size_t를 이용해 메모리를 잡을때 그 구현환경에 맞는 최대 크기로 보장토록하기
    if (pptr == NULL) {  // NULL을 역참조하려는 경우
        fprintf(stderr, "Error: NULL pointer passed to init_bint.\n"); // 오류출력은 standard error을 사용해 정상적인 출력과 구분
        exit(1);
    }
    if (*pptr != NULL) { // 전에 사용중이던 BINT를 위한 메모리가 해제되지 않은경우
        free((*pptr)->val);
        free(*pptr);
    }

    *pptr = (BINT*)calloc(1, sizeof(BINT));
    if (*pptr == NULL) { // 구조체 대상 메모리 공간 부족시
        fprintf(stderr, "Error: Failed to allocate BINT structure.\n");
        exit(1);
    }


    (*pptr)->val = (WORD*)calloc(alloc, sizeof(WORD));
    if ((*pptr)->val == NULL) { // val배열 메모리 할당 실패경우
        fprintf(stderr, "Error: Failed to allocate val array.\n");
        free(*pptr);
        exit(1);
    }
    // 초기화
    (*pptr)->is_negative = false;
    (*pptr)->wordlen = 0;
    (*pptr)->alloc = alloc;
}
void free_bint(BINT** p_bint) {
    if (*p_bint == NULL) return;  // 구조체가 이미 비어있다면 해제할 필요가 없음

    if ((*p_bint)->val != NULL)  // 구조체 내부의 val 배열이 비어있지 않으면 따로 해제
        free((*p_bint)->val);

    free(*p_bint);  // 구조체 메모리 해제
    *p_bint = NULL;  // p_bint가 이미 해제된 메모리주소를 가리키고 있기때문에 재접근 방지용
}

// 초기화하고 uint64를 BINT로 넣음
void set_bint_from_uint64(BINT** p_bint, uint64_t input) { // BINT**로 변경
    // NULL 포인터 체크
    if (p_bint == NULL) {
        fprintf(stderr, "Error: NULL BINT** pointer in set_bint_from_uint64.\n");
        exit(1);
    }

    // p_bint가 가리키는 BINT가 NULL이거나 / val 배열이 없거나 / 할당 크기가 부족하면 (uint64_t는 최대 2워드 필요) 초기화
    // init_bint는 *p_bint가 가리키는 메모리를 재할당하므로, 이전에 할당된 *p_bint를 직접 넘겨야 함.
    if (*p_bint == NULL || (*p_bint)->val == NULL || (*p_bint)->alloc < 2) {
        init_bint(p_bint, 2); // init_bint는 BINT**를 받으므로 p_bint를 직접 전달
    }

    BINT* bint = *p_bint; // 매번 역참조하는거 번거로우니까 편의상 추가로 포인터 사용

    bint->is_negative = false;
    // input을 워드단위로 계속 잘라서 저장할거니까 루프종료조건이 input이 0이되는 시점이니 입력값 자체가 0인부분은 따로 처리
    if (input == 0) {
        bint->val[0] = 0; // 0이면 최하위워드만 보면됨
        bint->wordlen = 1; // 0을 저장하기 위한 최소 워드 길이는 보장할 것.
        // 나머지 워드는 0으로 초기화
        for (size_t k = 1; k < bint->alloc; ++k) bint->val[k] = 0;
        return;
    }

    size_t i = 0;
    while (input > 0) {
        // 저장할 인덱스가 alloc을 넘어서면 워드 하나만큼 재할당
        if (i >= bint->alloc) {
            WORD* new_val = (WORD*)realloc(bint->val, sizeof(WORD) * (bint->alloc + 1));
            if (new_val == NULL) { // 유효하지 않은 메모리를 가리키는경우 :
                fprintf(stderr, "Error: Failed to reallocate memory in set_bint_from_uint64.\n");
                exit(1);
            }
            bint->val = new_val; // val도 갱신해주고
            bint->alloc++; // 늘어난 워드만큼 alloc도 갱신
        }
        bint->val[i++] = (WORD)(input & 0xFFFFFFFF);  // 32비트만큼 미트마스킹해서 추출하고 
        input >>= 32; // 다음으로 이동
    }

    bint->wordlen = i;  // 마지막 인덱스를 wordlen으로 받고
    // 나머지 워드를 0으로 초기화
    for (size_t k = i; k < bint->alloc; ++k) bint->val[k] = 0;
    normalize_wordlen(bint); // wordlen 정제해서 저장
}

// 출력함수
void print_bint_hex(const BINT* bint) {
    if (bint == NULL) {
        printf("NULL BINT\n");
        return;
    }

    if (bint->is_negative) printf("-");  //  bint가 음수인경우 앞에 부호 출력
    // 기본적으로 절댓값과 부호비트를 따로 들고 다니기 때문에 부호비트만 보면 연산 필요없이 -/+ 구분하여 출력하기 용이하다.

    printf("0x");
    for (ssize_t i = bint->wordlen - 1; i >= 0; --i) {  // Little Endian으로 저장되니 뒤집어서
        printf("%08x", bint->val[i]);
    }
    printf("\n");
}




void set_bint_from_str10(BINT** p_bint, const char* str) {  // init을 호출해서 사용할거기 때문에 구조체 주소도 같이준다.
    if (p_bint == NULL || str == NULL) {  // NULL 역참조케이스 예외처리
        fprintf(stderr, "Error: NULL pointer in set_bint_from_str10.\n");
        exit(1);
    }

    size_t alloc = calc_alloc_from_str10(str);
    init_bint(p_bint, alloc); // init으로 초기화하면 val과 wordlen 모두 0
    (*p_bint)->val[0] = 0;  
    (*p_bint)->wordlen = 1;  // 그런데 wordlen == 0이면  bint_is_zero (wordlen==1 && val[0]==0)가 False가 되어 add_bint_small함수가 잘못된 경로로 빠진다고함) -gpt => 추가 이해 필요
    size_t len = strlen(str);

    for (size_t i = 0; i < len; ++i) {
        if (str[i] < '0' || str[i] > '9') continue;  // i번째 문자가 10진수 숫자 0~9에 포함되는지 확인  => 공백/특수문자 필터링
        mul_bint_small(*p_bint, 10);  // 자릿수가 늘어나면 10을 곱해주기
        add_bint_small(*p_bint, str[i] - '0');  // 자릿수별 덧셈 -> char는 정수형으로 캐스팅하기위해 아스키 이용 (기존값에 누적되어 더해짐)
        // 0의 아스키코드는 48이고 1은 49... 9는 57까지 차례로 정의되어있음.
        // 문자열끼리의 연산은 아스키코드의 연산이고 같은 간격으로 정의되어있기 때문에 빼주면 정수로 캐스팅된다.
        // ex) '9' - '0' == 57 - 48 == 9(정수)
    }
}



// 16진수를 bint로 캐스팅하는 함수임. 10진수는 보류
void set_bint_from_word_array(BINT** p_bint, const WORD* arr, size_t len) {
    if (p_bint == NULL || arr == NULL) {
        fprintf(stderr, "Error: NULL pointer in set_bint_from_word_array.\n");
        exit(1);
    }

    // 메모리 할당
    init_bint(p_bint, len);

    BINT* b = *p_bint;
    b->wordlen = len;

    // 배열 복사
    for (size_t i = 0; i < len; ++i) {
        b->val[i] = arr[i];  // 주의 : 역순저장하지말것
    }

    // 0 정리
    normalize_wordlen(b);
}

// 문자열(16진수) 받아서 bint로
void set_bint_from_str16(BINT** p_bint, const char* s) {
    if (p_bint == NULL || s == NULL) {
        fprintf(stderr, "Error: NULL in set_bint_from_str16.\n");
        exit(1);
    }
    // s의 타입이 const char* 이므로, s++는 1바이트(= sizeof(char)) 앞으로 이동할 것임
    
    // 넘기기전에 정리해주기
    while (*s == ' ' || *s == '\t' || *s == '\n') ++s; // 주소값을 계속해서 증가시켜 비공백문자를 가리키게함

    // 접두어도 무시
    if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) s += 2;
    

    // 선행 0 제거 (전체가 0이면 한 자리 유지)
    const char* p = s;  // s 그대로 재사용
    while (*p == '0') ++p;
    if (*p == '\0') { // 0...만 있는 경우
        set_bint_from_uint64(p_bint, 0);
        return;
    }
    s = p; // 가리키는 주소 갱신 (나중에 재사용할 것이므로)

    // 길이/워드수 계산
    size_t hex_len = strlen(s);
    size_t words = (hex_len + 7) / 8; // 초과한만큼 추가 워드를 더 할당해서 저장해야하니 패딩

    WORD* arr = (WORD*)calloc(words, sizeof(WORD));
    if (!arr) { fprintf(stderr, "alloc fail\n"); exit(1); }

    // 뒤에서부터 8비트씩 끊어 리틀엔디언 WORD 배열 구성 (즉 i = 0이 최하위워드가 됨)
    for (size_t i = 0; i < words; ++i) {
        size_t start_idx = (hex_len > (i + 1) * 8) ? (hex_len - (i + 1) * 8) : 0; // 잘라낼 부분의 시작 인덱스
        // 맨 앞 블록이 8글자 미만이면 시작 인덱스를 0으로 설정
        size_t chunk_len = (hex_len > i * 8) ? (hex_len - i * 8) - start_idx : 0; //“이번에 잘라낼 16진수 조각이 몇 글자인가(최대 8)?”
        // 구간은 [start_idx, start_idx + chunk_len) 이렇게 될 것. 
        // s="123456789ABC" (hex_len=12) 이면
        // i = 0 : start_idx = 4, chunk_len=8  -> "56789ABC" → arr[0](LSW)
        // i = 1 : start_idx = 0, chunk_len = 4 -> "1234" → arr[1](MSW)
        
        char part[9] = { 0 }; // 32비트 16진수의 8글자 + '\0' 으로 9칸
        memcpy(part, s + start_idx, chunk_len);
        // void *memcpy(void *dest, const void *src, size_t n);로부터
        // 시작위치 : src, 실제길이 n, 
        arr[i] = (WORD)strtoul(part, NULL, 16); // "string to unsigned long" 정수로 캐스팅
    }

    set_bint_from_word_array(p_bint, arr, words); 
    free(arr);
}

// 문자열을 보고 16진/10진 자동 판별 -> 이건 제대로 동작하는지 모르겠음.. 나중에 확인할 것
void set_bint_from_str(BINT** p_bint, const char* s) {
    if (p_bint == NULL || s == NULL) {
        fprintf(stderr, "Error: NULL in set_bint_from_str.\n");
        exit(1);
    }

    // 공백 무시하기
    while (*s == ' ' || *s == '\t' || *s == '\n') ++s;

    // 0x/0X 접두사면 16진
    if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) {
        set_bint_from_str16(p_bint, s);
        return;
    }

    // 16진 문자가 하나라도 있으면 16진으로 간주
    for (const char* q = s; *q; ++q) {
        if ((*q >= 'A' && *q <= 'F') || (*q >= 'a' && *q <= 'f')) {
            set_bint_from_str16(p_bint, s);
            return;
        }
    }

    // 그 외는 10진 처리(있는거 갖다 붙임)
    set_bint_from_str10(p_bint, s); 
}


// 덧셈의 최상위함수로 일단 부호만을 결정
void add_bint(BINT** result, const BINT* a, const BINT* b) {

    if (result == NULL || a == NULL || b == NULL) {
        fprintf(stderr, "Error: NULL BINT pointer in add_bint.\n");
        exit(1);
    }
    // 왼쪽값이 크거나 같도록 세팅한 뒤 왼쪽에서 오른쪽방향으로 연산
    // 가령 a가 음수이고 b가 양수인경우 뺄셈과도 같으므로 절댓값을 비교하여 부호를 판정하고 값은 절댓값 차연산으로 취하도록 합
    // a + b
    if (!a->is_negative && !b->is_negative) {
        add_unsigned(result, a, b);
        (*result)->is_negative = false;
    }
    // -a + -b = -(a + b)
    else if (a->is_negative && b->is_negative) {
        add_unsigned(result, a, b);
        (*result)->is_negative = true;
    }
    // a + (-b) = a - b
    else if (!a->is_negative && b->is_negative) {
        if (cmp_bint(a, b) >= 0) {
            sub_unsigned(result, a, b);
            (*result)->is_negative = false;
        }
        else {
            sub_unsigned(result, b, a);
            (*result)->is_negative = true;
        }
    }
    // -a + b = b - a
    else { // (a->is_negative && !b->is_negative)
        if (cmp_bint(b, a) >= 0) {
            sub_unsigned(result, b, a);
            (*result)->is_negative = false;
        }
        else {
            sub_unsigned(result, a, b);
            (*result)->is_negative = true;
        }
    }
}

// 절댓값만 가지고 덧셈 (BINT + BINT)
void add_unsigned(BINT** result, const BINT* a, const BINT* b) {
    if (result == NULL || a == NULL || b == NULL) {  // NULL 역참조 예외처리
        fprintf(stderr, "Error: NULL BINT pointer in add_unsigned.\n");
        exit(1);
    }

    // 더 긴 길이 기준으로 메모리 할당
    size_t maxlen = (a->wordlen > b->wordlen) ? a->wordlen : b->wordlen;
    init_bint(result, maxlen + 1);  // (여유있게 하나 더)
    BINT* r = *result;  // (*result) -> 처럼 매번 역참조하지않도록 편의변수

    uint64_t carry = 0;

    for (size_t i = 0; i < maxlen; ++i) {  //  배열에서 꺼내는 단위는 워드(32 bit)지만 넘칠 수 있으니 uint64로 설정
        uint64_t av = (i < a->wordlen) ? a->val[i] : 0;  // 인덱스가 wordlen을 벗어나면 다음 인덱스은 0으로 채워 자릿수 맞춰주기
        uint64_t bv = (i < b->wordlen) ? b->val[i] : 0;  // b도 마찬가지
        uint64_t sum = av + bv + carry;

        r->val[i] = (WORD)(sum & 0xFFFFFFFF);  // 32비트 연산의 결과 최대 33비트인 경우가 있으므로 하위 32비트만 추출하고 나머지는 다음 워드로 전달
        // worst case : (0xFFFFFFFF + 0xFFFFFFFF) + 1  = > 2^33 -1 (0x1FFFFFFFF)  -> 이경우 0x00000001과 0xFFFFFFFF으로 분리
        carry = sum >> 32;  // 분리된 나머지 carry bit는 다음 루프에서 사용할 수 있도록 갱신
    }

    r->wordlen = maxlen;  // wordlen은 더 긴쪽의 길이로 설정
    if (carry > 0) {  // 모든 루프가 끝났는데도 carry가 남아있으면 
        r->val[r->wordlen++] = (WORD)carry;  // 최상위 인덱스에 따로 워드형태로 캐스팅해서 저장
    }
}


void add_bint_small(BINT* b, uint32_t addend) {   // 매우 큰 수인 b에 가감수 addend자체를 더함
   if (b == NULL) {
        fprintf(stderr, "Error: NULL BINT pointer in add_bint_small.\n");
        exit(1);
    }
    // b->val이 NULL일 수도 있으므로, 최소 1 워드는 할당되어 있어야 함.
    if (b->val == NULL || b->alloc == 0) {
        fprintf(stderr, "Error: BINT not properly initialized (val or alloc is NULL/0) in add_bint_small.\n");
        exit(1);
    }

    // 누적값도 0이고 더할 값도 0이면 이미 0이라는 값을 가지고 있기때문에 연산필요가 없다.
    if (bint_is_zero(b) && addend == 0) {
        return;
    }
    // 누적값은 0인데 더할 값은 0이 아니면, 즉 연산을 처음 시작하는경우 초기설정
    else if (bint_is_zero(b)) {
        set_bint_from_uint64(&b, addend); // 기본설정해주는 함수
        b->is_negative = false; // 0에서 시작하여 addend를 더했으므로 양수
        return;
    }

    uint64_t sum = (uint64_t)b->val[0] + addend;  // 최하위워드에 addend를 더함
    b->val[0] = (WORD)(sum & 0xFFFFFFFF);  // carry가 발생했을 수도 있으니 32비트만큼 추출하고
    uint64_t carry = sum >> 32;  // 나머지 비트인 carry 비트는 따로 떼어냄

    // 다른 인덱스는 carry를 전달해주기만 하면 되므로 간단해진다.
    size_t i = 1;  // 위에서 val[0]은 연산이 되었으니 인덱스는 1부터 시작
    while (carry && i < b->wordlen) { // 이전 자리에서 발생한 carry는 다음 자리로 전달되어야함.
        // carry == 0이면 더 이상 전달할 carry가 없기 때문에 종료. 인덱스가 유효 범위를 벗어나도 종료.
        sum = (uint64_t)b->val[i] + carry;
        b->val[i] = (WORD)(sum & 0xFFFFFFFF);
        carry = sum >> 32;
        ++i;
    } // (현재 값 + carry) -> 32비트만 추출하고, carry는 다시 다음 워드로. 똑같음.

    if (carry > 0) { // 만약 루프가 다 끝났는데도 carry가 남아있다?
        if (b->wordlen == b->alloc) { // 현재 할당된 공간이 부족하면
            WORD* new_val = (WORD*)realloc(b->val, sizeof(WORD) * (b->alloc + 1));
            // realloc 실패 시 처리
            if (new_val == NULL) {
                fprintf(stderr, "Error: Failed to reallocate memory in add_bint_small.\n");
                exit(1);
            }
            b->val = new_val; // realloc 성공 시 새 주소 할당
            b->alloc++; // 할당한 워드 총 개수가 늘어났으니 갱신
        }
        b->val[b->wordlen++] = (WORD)carry; // 메모리가 충분하다면 바로 최상위 인덱스에 넣어주면 된다.
    }
}


// 절댓값 비교함수. 왼쪽이 크면 1, 오른쪽이 크면 -1 같으면 0
int cmp_bint(const BINT* a, const BINT* b) { // 비교만 할거기때문에 파라미터타입은 읽기전용으로
    if (a == NULL || b == NULL) {
        fprintf(stderr, "Error: NULL BINT pointer in cmp_bint.\n");
        exit(1);
    }
    // 워드 길이만으로 크기비교를 할수 있으면 좋다.
    if (a->wordlen > b->wordlen) return 1;  // 왼쪽이 크면 1
    if (a->wordlen < b->wordlen) return -1;  // 오른쪽이 크면 -1

    // 그런데 워드 길이가 같은 경우에는 val배열 내부에 들어가서 크기비교를 추가로 해준다.
    for (ssize_t i = a->wordlen - 1; i >= 0; --i) {
        // size_t로 잡으면 wordlen이 0일때 wordlen -1이 오버플로우되어 매우 큰 양수가 될 수 있으니
        // 안전하게 ssize_t로 잡아 음수개념을 추가하여 애초에 루프가 실행되지 않도록함
        if (a->val[i] > b->val[i]) return 1;
        if (a->val[i] < b->val[i]) return -1;
    }
    return 0; // 길이도 같으면서 모든 워드가 같으면 여기에 도달하고 0을 return함
}
// 절댓값  뺄셈
void sub_unsigned(BINT** result, const BINT* a, const BINT* b) {
    extern bool sub_unsigned_called;
    sub_unsigned_called = false;
    sub_unsigned_called = true; // 테스트 과정에서 실제로 호출되었는지 확인하기위한 코드
    if (result == NULL || a == NULL || b == NULL) {
        fprintf(stderr, "Error: NULL BINT pointer in sub_unsigned.\n");
        exit(1);
    }
    // 전제조건은 왼쪽파라미터가 오른쪽보다 크거나 같다( a >= b)
    // result는 더 큰수 기준으로 할당되어야함
    init_bint(result, a->wordlen);
    BINT* r = *result;

    int64_t borrow = 0;

    for (size_t i = 0; i < a->wordlen; ++i) { // 최하위부터 연산

        // val[i]는 WORD (uint32_t)이므로, 음수 연산을 위해 int64_t로 캐스팅
        int64_t av = (int64_t)a->val[i]; // 워드값 가져오기
        int64_t bv = (i < b->wordlen) ? (int64_t)b->val[i] : 0; // 기본적으로 a가 더 길기때문에 b에서 모자란부분이 생김. 0으로 패딩

        int64_t diff = av - bv - borrow; // 현재 워드 - 빼는 워드 - 이전비트 borrow

        if (diff < 0) { // 결과가 음수이면 borrow 발생
            diff += ((int64_t)1 << 32); // 빌려주는 단위는 word
            // 오버플로우방지 위해 int_64_t형태로 캐스팅. 내가 원하는 값은 2^32 (0x0000000100000000)
            borrow = 1; // 다음 워드로 1을 빌려줌
        }
        else {
            borrow = 0; // 빌림 발생 안함
        }

        r->val[i] = (WORD)diff; // result에다가 i번째 연산결과 저장
    }

    r->wordlen = a->wordlen;
    // 뺄셈으로 저장하는데에 필요한 word수가 감소하는 경우가 있으니 a 기준 재조정
    // 예를 들어 100 - 99 = 1 인 경우 wordlen이 2 -> 1로 줄어야 함. 결과값의 길이는 a의 길이보다 작거나 같다.

    normalize_wordlen(r); // 다듬어주기

    // 결과가 0인 경우 wordlen이 0이 될 수 있으므로, 1로 유지하고 val[0] = 0으로 함
    if (r->wordlen == 0 && r->alloc > 0) {
        r->val[0] = 0;
        r->wordlen = 1;
    }
}


// 0인지 확인
bool bint_is_zero(const BINT* b) {
    if (b == NULL) return false; // NULL은 0이 아님 (또는 오류 처리 필요)
    // normalize_wordlen이 호출되었다고 가정하면, 0은 wordlen=1, val[0]=0 이어야 합니다.
    return (b->wordlen == 1 && b->val[0] == 0);
}

// 출력함수 내부용 곱셈 (BINT 곱셈 아님) -> 더이상 입력 인자가 문자열이 아니므로 캐스팅할 필요도 없어졌다. 혹여 10진수 입력경우 필요
void mul_bint_small(BINT* b, uint32_t m) { // m은 곱하는 수
    if (b == NULL) { // NULL포인터 체크
        fprintf(stderr, "Error: NULL BINT pointer in mul_bint_small.\n");
        exit(1);
    }
    if (b->val == NULL || b->alloc == 0) { // BINT 초기화상태 확인
        fprintf(stderr, "Error: BINT not properly initialized (val or alloc is NULL/0) in mul_bint_small.\n");
        exit(1);
    }

    // 곱하는 수 m이 0이면 결과는 0
    if (m == 0) {
        set_bint_from_uint64(b, 0); // b를 0으로 설정
        return;
    }
    uint64_t carry = 0;

    for (size_t i = 0; i < b->wordlen; ++i) { // 하위 워드부터 연산
        uint64_t prod = (uint64_t)b->val[i] * m + carry; // 64비트로 계산하여 오버플로우 방지 => product = 현재워드 * m + carry 형태
        b->val[i] = (WORD)(prod & 0xFFFFFFFF); // 하위 32비트 추출하고 
        carry = prod >> 32; // 캐리는 다음 워드로
    }
    // 루프가 다 끝났는데도 carry가 남아있는경우
    if (carry > 0) {
        if (b->wordlen == b->alloc) { // 현재 할당된 공간이 부족하면
            WORD* new_val = (WORD*)realloc(b->val, sizeof(WORD) * (b->alloc + 1)); // 워드 1개만큼 재할당을 해주는데
            // 실패시 예외처리
            if (new_val == NULL) {
                fprintf(stderr, "Error: Failed to reallocate memory in mul_bint_small.\n");
                exit(1);
            }
            b->val = new_val; // realloc 성공 시 새 주소 할당
            b->alloc++; // alloc 갱신해주기
        }
        b->val[b->wordlen++] = (WORD)carry; // 최상위 인덱스에 carry 별도로 저장
    }
    normalize_wordlen(b); // 최종적으로 비트 정제
}


// add의 최상위 함수 add_bint가 부호를 결정하는 방식은 if와 &연산을 이용해 ++/-- 는 덧셈으로, +-/-+는 뺄셈으로 치환하는 것이었음 (4회 개별분기)
// 이 함수에서는 부호의 같고 다름을 sign bit의 xor으로 처리하여 구분
void sub_bint(BINT** result, const BINT* a, const BINT* b) {
    if (!result || !a || !b) {
        fprintf(stderr, "Error: NULL BINT pointer in sub_bint.\n");
        exit(1);
    }

   
    BINT av = *a; av.is_negative = false; // 구조체 복사해서 절댓값으로
    BINT bv = *b; bv.is_negative = false;

    const int sa = a->is_negative ? 1 : 0;
    const int sb = b->is_negative ? 1 : 0;
    const int do_add = sa ^ sb;              
    // 부호가 다른 두 수의 뺄셈은 절댓값 덧셈으로 치환함 (sign bits xor = 1) 
    // (+) - (-) -> (+) + |b|
    // {-) - (+) -> (-)(|a| + |b|)  


    if (do_add) {
        add_unsigned(result, &av, &bv);
        if (*result) (*result)->is_negative = bint_is_zero(*result) ? false : (sa != 0); // 만약 0이면 양수로취급
        // *result != NULL 이면 조건문이 실행되고, 0인지의 여부가 bool으로 is_negative에 저장. 
        // 간단히 wordlen이 0이면 부호를 양수로, 0이 아니면 a의 부호를 따라감
        return;
    }

    // 부호가 같을 때는 절댓값 비교하여 큰쪽에서 작은쪽을 빼주면 됨
    int cmp = cmp_bint(&av, &bv);    // |a| ? |b|
    const BINT* hi = (cmp >= 0) ? &av : &bv; // cmp로 high 와 low결정
    const BINT* lo = (cmp >= 0) ? &bv : &av;

    sub_unsigned(result, hi, lo);

    
    if (*result) {
        if (bint_is_zero(*result)) { // 마찬가지로 0인경우 양수처리
            (*result)->is_negative = false;
        }
        else {
            int neg = (cmp >= 0) ? sa : (sa ^ 1); // 절댓값 비교 결과 a가 크다면 a의 부호를, 아닌경우 반대
            (*result)->is_negative = (neg != 0);
        }
    }
}

