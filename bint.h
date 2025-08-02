#ifndef BINT_H
#define BINT_H

#ifdef _MSC_VER
#include <BaseTsd.h>
typedef SSIZE_T ssize_t;  // vs studio������ ssize_t�ν��� ���ؼ� �̷��� �߰��ؾ��Ѵٰ� ��.
#else
#include <sys/types.h>  // ssize_t for POSIX
#endif

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

extern bool sub_unsigned_called; // �����Լ� ȣ�� Ȯ��

typedef uint32_t WORD; // �ϴ��� 32��Ʈ ���常 ����մϴ�.

typedef struct {
    bool is_negative;   // ��ȣ
    size_t wordlen;   // ��ȿ�� ���� �������(�տ� �Ѵ� 0���� ������ ����)
    size_t alloc;      // �Ҵ�� �� �����
    WORD* val;         // ���� �迭 
} BINT;


// �⺻ �޸� ���� �� ����

void init_bint(BINT** p_bint, size_t alloc_words);
void free_bint(BINT** p_bint);

void set_bint_from_uint64(BINT** p_bint, uint64_t input);
void set_bint_from_word_array(BINT** p_bint, const WORD* arr, size_t len);

// ���
void print_bint_hex(const BINT* bint);

// ����
void add_bint(BINT** result, const BINT* a, const BINT* b);
void add_unsigned(BINT** result, const BINT* a, const BINT* b);

void sub_unsigned(BINT** result, const BINT* a, const BINT* b);

// ����Լ��� ���� ���� �Լ�
void mul_bint_small(BINT* b, uint32_t m);
void add_bint_small(BINT* b, uint32_t a);

// ������ ��ƿ�Լ�
int cmp_bint(const BINT* a, const BINT* b);        // ���� ��
bool bint_is_zero(const BINT* b);                  // 0 ���� Ȯ��
void normalize_wordlen(BINT* b);

#endif
