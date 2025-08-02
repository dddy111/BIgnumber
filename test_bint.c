#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include "bint.h"


bool sub_unsigned_called = false;


// note : 테스트는 assert.h 헤더에 정의된 디버깅용 함수 assert를 이용
// 주어진 조건이 false인 경우 프로그램을 즉시 종료시키고 오류 메시지 출력


// init함수 테스트
void test_init_alloc(void) {
    BINT* b = NULL;  
    init_bint(&b, 3);   // 3워드만큼 할당 요청
  
    
    assert(b != NULL);  // b가 정상적으로 초기화 됐는가?
    assert(b->val != NULL); // val 포인터가 유효한 메모리를 가리키는지? (malloc 등이 실패했을 경우 - 즉 메모리 할당이 되었는지)
    assert(b->alloc == 3);  // 요청한 할당 크기와 일치하는가?
    assert(b->wordlen == 0);  // wordlen이 바르게 초기화 됐는가?
    for (size_t i = 0; i < b->alloc; ++i) // 배열 내부 값은 0으로 초기화 되었는지?
        assert(b->val[i] == 0);
    free_bint(&b);
}

// free_bint 함수 테스트
void test_free_bint_nullsafe(void) {
    BINT* b = NULL;
    init_bint(&b, 3); 

    b->val[0] = 0x12345678;
    b->val[1] = 0xABCDEF01;
    b->val[2] = 0;

    assert(b != NULL);
    assert(b->val != NULL);
    assert(b->alloc == 3);

    free_bint(&b);  

    
    assert(b == NULL); 
}


// 이제 16진수 배열을  bint구조에 맞춰 가져다주기 때문에 변환과정이 필요하지 않음 (10진수 입력시 사용 예정)
// uint64를 bint로 캐스팅 하였을 때 정확하게 변환되는 것이 맞는지?
void test_set_bint_from_uint64(void) {
    BINT* b = NULL;
    set_bint_from_uint64(&b, 0xFFFFFFFFFFFFFFFF); // uint64의 최대값으로 경계부분 체크
    assert(b->wordlen == 2);  // 입력값이 1워드(32비트)를 초과하면 wordlen은 2가 되어야 함
    assert(b->val[0] == 0xFFFFFFFF); // 배열의 하위/상위 부분인 val[0], val[1]은 각각 분할된 값인 0xFFFFFFFF가 되어야함
    assert(b->val[1] == 0xFFFFFFFF);
    free_bint(&b);
}


void test_set_bint_from_word_array_basic(void) {
    //BINT* b = NULL;
    //WORD arr[] = { 0x00ABCDEF }; // 대충 연속된 값으로 테스트 (1워드로 저장)
    //set_bint_from_word_array(&b, arr, 1);
    //assert(b->wordlen == 1); // 워드길이 제대로 동작?
    //assert(b->val[0] == 0x00ABCDEF);  // 배열의 최하위에 정확히 할당되는지?
    //free_bint(&b); 실패한 테스트
    BINT* b = NULL;
    WORD arr[] = { 0x00ABCDEF, 0x01234567, 0xffffffff }; // 대충 연속된 값으로 테스트 (1워드로 저장)
    set_bint_from_word_array(&b, arr, 3);
    assert(b->wordlen == 3); // 워드길이 제대로 동작?
    assert(b->val[0] == 0x00ABCDEF);  // 배열의 최하위에 정확히 할당되는지?
    assert(b->val[1] == 0x01234567);
    assert(b->val[2] == 0xffffffff);
    free_bint(&b);
}

// 절댓값에대한 덧셈 (carry가 발생하지 않는 경우)
// 일단 기본적인 덧셈이 작동하는지부터 보고, carry에 대한 연산은 별도로 분리
void test_add_unsigned_basic(void) {
    BINT* a = NULL, * b = NULL, * res = NULL;
    WORD aa[] = { 0x1 }; // 피연산자는 각각 carry가 발생하지 않도록 작은 값
    WORD bb[] = { 0x2, 0x3, 0x4 };  // 메모리 할당은 더 긴 길이를 기준으로 하기 때문에
    // 두 피연산자 워드길이를 다르게 설정. 단 carry는 발생하지 않게 작은 값
    set_bint_from_word_array(&a, aa, 1); // 각각 BINT 구조체로 캐스팅
    set_bint_from_word_array(&b, bb, 3);
    add_unsigned(&res, a, b);
    // 검증
    assert(res != NULL);
    assert(res->wordlen == 3);          // carry 없음 → wordlen은 b의 길이(3)와 같아야 함
    
    // 문제 발생 지점
    assert(res->val[0] == 0x3);         // 0x1 + 0x2 == 0x3 에서 assert 결과 오류
    // 문제 원인 가능성 1 : a->val[i] 또는 b->val[i]에서 잘못된 초기화
    // == set_bint_from_word_array()에서 잘못 값을 복사하는 경우인데, 상위 순서에서 테스트결과 정상 (기각)
    // 문제 원인 가능성 2: WORD를 잘못정의했다? (32비트가 아닌경우) -> bint.h에 제대로 정확히 정의되어있음. sizeof(WORD) = 4까지 확인 (기각)
    // 배열 내부 검증 :  
    // b->val[0] = 0x00000004
    // b->val[1] = 0x00000003
    // b->val[2] = 0x00000002
    // 배열이 역순으로 저장 되고 있었음
    // 이는 set_bint_from_word_array()가 배열을 역순저장하기 때문으로,
    // arr[]가 상위 워드부터 저장되어있다고 가정한 실수에서 기인함
    // 인풋 파라미터를 문자열에서 워드 배열로 받도록 바꾸었는데, 이 부분을 미처 바꾸지 않음
    
    // 가령 0x1234567890ABCDEF 이라면 사람이 읽는 문자열은 0x12345678이 최상위워드임.
    // 그래서 "문자열인 경우" 저장과정은 역순으로 진행되어야함.
    //  val[0] == 0x90ABCDEF, val[1] == 0x12345678 와 같이 채워졌고,
    // 그래서 하위워드부터 연산하는 add함수와 충돌이 일어난 것임.
    // 정리 -> input 파라미터를 문자열이 아닌 워드 배열로 바꾸는 과정에서 bint로 캐스팅하는 함수는 제대로 변경되지 않아(여전히 문자열기준 역순배열) 충돌 발생
    // set_bint_from_word_array() 테스트 함수 수정 필요 -> 입력 워드 개수가 하나라서 검증에 실패한것임(할당되는지 여부만 확인하고 순서를 확인하지 않음)
    
    assert(res->val[1] == 0x3);         // a에는 word[1]이 없으므로 b의 값 0x3 그대로
    assert(res->val[2] == 0x4);         // 상동

    // 메모리 정리
    free_bint(&a);
    free_bint(&b);
    free_bint(&res);
}
// 이어서, carry가 발생하는 경우
void test_add_unsigned_with_carry(void) {
    BINT* a = NULL, * b = NULL, * res = NULL;
    WORD aa[] = { 0xFFFFFFFF };
    WORD bb[] = { 0x1 };
    set_bint_from_word_array(&a, aa, 1); // 
    set_bint_from_word_array(&b, bb, 1);
    add_unsigned(&res, a, b);   // maxlen == 1이고 (더 긴 길이) carry를 대비해 alloc = maxlen + 1 ( == 2)로 메모리 할당되어야 정상
    assert(res->alloc == 2);
    assert(res->wordlen == 2); // carry가 발생하여 wordlen은 증가해야한다.
    assert(res->val[0] == 0x00000000);
    assert(res->val[1] == 0x00000001);
    free_bint(&a); free_bint(&b); free_bint(&res);
}

void test_add_bint(void) { // add_bint (부호 결정 함수)
    BINT* a = NULL, * b = NULL, * res = NULL;
    // extern bool sub_unsigned_called;
    // 함수에도 언급했지만 부호가 다른 피연산자의 연산은 뺄셈임.
    // 때문에 이경우 뺄셈이 호출되므로 결과값만 보고 판단해서는 안되고 잘 거쳐갔는지 확인하는 변수 추가 -> 기본값은 false
    // 호출되는순간 true로 변환될것임. bint.c에서도 사용할 것이므로 extern으로 전역변수를 가져다 쓸 수 있도록 함
    
    // a + b
    WORD aa1[] = { 0x2 };
    WORD bb1[] = { 0x3 };
    set_bint_from_word_array(&a, aa1, 1);
    set_bint_from_word_array(&b, bb1, 1);
    add_bint(&res, a, b);
    assert(res->wordlen == 1);
    assert(res->val[0] == 0x5); // 덧셈이 잘 수행되었는지? (간접적으로 2차검증)
    assert(res->is_negative == false); // 부호가 잘 결정 되었는지? 
    free_bint(&a); free_bint(&b); free_bint(&res);

    // -a + -b = -(a + b)
    WORD aa2[] = { 0x2 };
    WORD bb2[] = { 0x3 };
    set_bint_from_word_array(&a, aa2, 1);
    set_bint_from_word_array(&b, bb2, 1);
    a->is_negative = true;
    b->is_negative = true;
    add_bint(&res, a, b);
    assert(res->wordlen == 1);
    assert(res->val[0] == 0x5); // 연산 결과는 절댓값으로
    assert(res->is_negative == true); // 부호 결정 정상적인지?
    free_bint(&a); free_bint(&b); free_bint(&res);

    // a + (-b) = a - b where a > b
    WORD aa3[] = { 0x5 };
    WORD bb3[] = { 0x2 };
    set_bint_from_word_array(&a, aa3, 1);
    set_bint_from_word_array(&b, bb3, 1);
    b->is_negative = true;

    sub_unsigned_called = false;
    add_bint(&res, a, b);
    assert(res->wordlen == 1);
    assert(res->val[0] == 0x3);
    assert(res->is_negative == false);
    assert(sub_unsigned_called == true);  // 뺄셈을 호출하는 경우 실제로 호출되었는지 확인
    free_bint(&a); free_bint(&b); free_bint(&res);

    // -a + b = b - a where a < b
    WORD aa4[] = { 0x2 };
    WORD bb4[] = { 0x5 };
    set_bint_from_word_array(&a, aa4, 1);
    set_bint_from_word_array(&b, bb4, 1);
    a->is_negative = true;

    sub_unsigned_called = false;
    add_bint(&res, a, b);
    assert(res->wordlen == 1);
    assert(res->val[0] == 0x3);
    assert(res->is_negative == false);
    assert(sub_unsigned_called == true); // 역시 뺄셈함수 호출여부 확인
    free_bint(&a); free_bint(&b); free_bint(&res);
}

// 이하 두 함수는 10진수 입력시 필요해지는 함수로 검증이 추가로 필요하다.
//void test_add_bint_small_carry_extend(void) {
//    BINT* b = NULL;
//    WORD bb[] = { 0xFFFFFFFF };
//    set_bint_from_word_array(&b, bb, 1);
//    add_bint_small(b, 1);
//    assert(b->wordlen == 2);
//    assert(b->val[0] == 0x00000000);
//    assert(b->val[1] == 0x00000001);
//    free_bint(&b);
//}

//void test_mul_bint_small_basic(void) {
//    BINT* b = NULL;
//    WORD bb[] = { 0x2 };
//    set_bint_from_word_array(&b, bb, 1);
//    mul_bint_small(b, 3);
//    assert(b->val[0] == 6);
//    free_bint(&b);
//}

// 유틸함수 검증
// 출력함수에 쓰였으나 여전히 나눗셈 등에 필요하다.
void test_bint_is_zero_cases(void) {
    BINT* b = NULL;
    set_bint_from_uint64(&b, 0); // b는 val[0] == 0, wordlen == 1
    // wordlen이 1인 경우이면서도 배열 내부값이 0이어야 0임.
    assert(bint_is_zero(b));  // 정상적인 0
    b->val[0] = 1;
    assert(!bint_is_zero(b)); // val[0] == 1이기 때문에 false가 반환되어야 정상(그러므로 !false 를 assert)
    free_bint(&b);
}


void test_cmp_bint_basic(void) {  // 절댓값 compare 결과 검증
    BINT* a = NULL, * b = NULL;

    // 워드길이만 가지고 대소비교를 할 수 있는 경우
    WORD w1[] = { 0x1 };
    WORD w2[] = { 0x1, 0x1 }; // 수정 -> 0x1, 0x0이었으나 이경우 normalize때문에 w1과 w2가 같아지게됨
    set_bint_from_word_array(&a, w1, 1); // 여기에서 normalize를 내부호출
    set_bint_from_word_array(&b, w2, 2);
    assert(cmp_bint(a, b) == -1);  // wordlen 비교로 b가 더 큼
    free_bint(&a); free_bint(&b);
    
    // a == b
    WORD wa[] = { 0xABCD };
    WORD wb[] = { 0xABCD };
    set_bint_from_word_array(&a, wa, 1);
    set_bint_from_word_array(&b, wb, 1);
    assert(cmp_bint(a, b) == 0);  // a == b 같으면 0
    free_bint(&a); free_bint(&b);

    // a < b
    WORD wc[] = { 0xABCD };
    WORD wd[] = { 0xABCE };
    set_bint_from_word_array(&a, wc, 1);
    set_bint_from_word_array(&b, wd, 1);
    assert(cmp_bint(a, b) == -1); // a < b 오른쪽이 크면 -1
    free_bint(&a); free_bint(&b);

    // a > b
    WORD we[] = { 0xABCE };
    WORD wf[] = { 0xABCD };
    set_bint_from_word_array(&a, we, 1);
    set_bint_from_word_array(&b, wf, 1);
    assert(cmp_bint(a, b) == 1);  // a > b 왼쪽이 크면 1
    free_bint(&a); free_bint(&b);
}

void test_normalize_wordlen_trim(void) {
    BINT* b = NULL;
    init_bint(&b, 5); // val[] = {0x1, 0x0, 0x0, 0x0, 0x0}, wordlen = 5 로 잡을것임
    b->val[0] = 0x1;
    b->val[1] = 0x0;
    b->val[2] = 0x0;
    b->val[3] = 0x0;
    b->val[4] = 0x0;
    b->wordlen = 5;
    normalize_wordlen(b); // 함수 호출결과 상위 필요없는 워드는 전부 trimmed되고 wordlen은 1이되어야함
    assert(b->wordlen == 1);
    free_bint(&b);
}

void test_sub_unsigned_basic(void) { // 상위워드로 borrow가 발생하지 않는 기본적인 경우
    BINT* a = NULL, * b = NULL, * res = NULL;

    // a = 0x00001000 , b = 0x00000001 
    WORD wa[] = { 0x1000 };
    WORD wb[] = { 0x0001 };
    set_bint_from_word_array(&a, wa, 1);
    set_bint_from_word_array(&b, wb, 1);

    sub_unsigned(&res, a, b);

    // result == 0x0FFF 이어야.
    assert(res->wordlen == 1); // 워드 길이는 여전히 동일하다.
    assert(res->val[0] == 0x0FFF);

    free_bint(&a); free_bint(&b); free_bint(&res);
}
void test_sub_unsigned_with_borrow(void) {
    BINT* a = NULL, * b = NULL, * res = NULL;

    // a = 0x00000000 0x00000001 (2^32)
    // b = 0x00000001 (1)
    WORD wa[] = { 0x00000000, 0x00000001 };
    WORD wb[] = { 0x00000001 };
    set_bint_from_word_array(&a, wa, 2);
    set_bint_from_word_array(&b, wb, 1);

    sub_unsigned(&res, a, b);

    // result = 0xFFFFFFFF (하위 워드), 0x00000000 (상위 워드) 기대
    assert(res->wordlen == 1);           // 상위 워드는 0이 되어 정상적으로 잘려야한다.
    assert(res->val[0] == 0xFFFFFFFF);   // 2^32 - 1 = 0xFFFFFFFF

    free_bint(&a); free_bint(&b); free_bint(&res);
}

