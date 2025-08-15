#ifndef BINT_H
#define BINT_H

#ifdef _MSC_VER
#include <BaseTsd.h>
typedef SSIZE_T ssize_t;  // vs studio에서는 ssize_t인식을 못해서 이렇게 추가해야한다고 함.
#else
#include <sys/types.h>  // ssize_t for POSIX
#endif

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

extern bool sub_unsigned_called; // 뺄셈함수 호출 확인

typedef uint32_t WORD; // 일단은 32비트 워드만 취급합니다.

typedef struct {
    bool is_negative;   // 부호
    size_t wordlen;   // 유효한 실제 워드길이(앞에 붇는 0같은 데이터 제외)
    size_t alloc;      // 할당된 총 워드수
    WORD* val;         // 워드 배열 
} BINT;


// 기본 메모리 관리 및 설정

void init_bint(BINT** p_bint, size_t alloc_words);
void free_bint(BINT** p_bint);

void set_bint_from_uint64(BINT** p_bint, uint64_t input);
void set_bint_from_word_array(BINT** p_bint, const WORD* arr, size_t len);
void set_bint_from_str10(BINT** p_bint, const char* str);
void set_bint_from_str16(BINT** p_bint, const char* s);
void set_bint_from_str(BINT** p_bint, const char* s);
// 출력
void print_bint_hex(const BINT* bint);

// 연산
void add_bint(BINT** result, const BINT* a, const BINT* b);
void add_unsigned(BINT** result, const BINT* a, const BINT* b);

void sub_unsigned(BINT** result, const BINT* a, const BINT* b);

// 출력함수를 위한 내부 함수
void mul_bint_small(BINT* b, uint32_t m);
void add_bint_small(BINT* b, uint32_t a);

// 나머지 유틸함수
int cmp_bint(const BINT* a, const BINT* b);        // 절댓값 비교
bool bint_is_zero(const BINT* b);                  // 0 여부 확인
void normalize_wordlen(BINT* b);

#endif
