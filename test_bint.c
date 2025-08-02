#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include "bint.h"


bool sub_unsigned_called = false;


// note : �׽�Ʈ�� assert.h ����� ���ǵ� ������ �Լ� assert�� �̿�
// �־��� ������ false�� ��� ���α׷��� ��� �����Ű�� ���� �޽��� ���


// init�Լ� �׽�Ʈ
void test_init_alloc(void) {
    BINT* b = NULL;  
    init_bint(&b, 3);   // 3���常ŭ �Ҵ� ��û
  
    
    assert(b != NULL);  // b�� ���������� �ʱ�ȭ �ƴ°�?
    assert(b->val != NULL); // val �����Ͱ� ��ȿ�� �޸𸮸� ����Ű����? (malloc ���� �������� ��� - �� �޸� �Ҵ��� �Ǿ�����)
    assert(b->alloc == 3);  // ��û�� �Ҵ� ũ��� ��ġ�ϴ°�?
    assert(b->wordlen == 0);  // wordlen�� �ٸ��� �ʱ�ȭ �ƴ°�?
    for (size_t i = 0; i < b->alloc; ++i) // �迭 ���� ���� 0���� �ʱ�ȭ �Ǿ�����?
        assert(b->val[i] == 0);
    free_bint(&b);
}

// free_bint �Լ� �׽�Ʈ
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


// ���� 16���� �迭��  bint������ ���� �������ֱ� ������ ��ȯ������ �ʿ����� ���� (10���� �Է½� ��� ����)
// uint64�� bint�� ĳ���� �Ͽ��� �� ��Ȯ�ϰ� ��ȯ�Ǵ� ���� �´���?
void test_set_bint_from_uint64(void) {
    BINT* b = NULL;
    set_bint_from_uint64(&b, 0xFFFFFFFFFFFFFFFF); // uint64�� �ִ밪���� ���κ� üũ
    assert(b->wordlen == 2);  // �Է°��� 1����(32��Ʈ)�� �ʰ��ϸ� wordlen�� 2�� �Ǿ�� ��
    assert(b->val[0] == 0xFFFFFFFF); // �迭�� ����/���� �κ��� val[0], val[1]�� ���� ���ҵ� ���� 0xFFFFFFFF�� �Ǿ����
    assert(b->val[1] == 0xFFFFFFFF);
    free_bint(&b);
}


void test_set_bint_from_word_array_basic(void) {
    //BINT* b = NULL;
    //WORD arr[] = { 0x00ABCDEF }; // ���� ���ӵ� ������ �׽�Ʈ (1����� ����)
    //set_bint_from_word_array(&b, arr, 1);
    //assert(b->wordlen == 1); // ������� ����� ����?
    //assert(b->val[0] == 0x00ABCDEF);  // �迭�� �������� ��Ȯ�� �Ҵ�Ǵ���?
    //free_bint(&b); ������ �׽�Ʈ
    BINT* b = NULL;
    WORD arr[] = { 0x00ABCDEF, 0x01234567, 0xffffffff }; // ���� ���ӵ� ������ �׽�Ʈ (1����� ����)
    set_bint_from_word_array(&b, arr, 3);
    assert(b->wordlen == 3); // ������� ����� ����?
    assert(b->val[0] == 0x00ABCDEF);  // �迭�� �������� ��Ȯ�� �Ҵ�Ǵ���?
    assert(b->val[1] == 0x01234567);
    assert(b->val[2] == 0xffffffff);
    free_bint(&b);
}

// ���񰪿����� ���� (carry�� �߻����� �ʴ� ���)
// �ϴ� �⺻���� ������ �۵��ϴ������� ����, carry�� ���� ������ ������ �и�
void test_add_unsigned_basic(void) {
    BINT* a = NULL, * b = NULL, * res = NULL;
    WORD aa[] = { 0x1 }; // �ǿ����ڴ� ���� carry�� �߻����� �ʵ��� ���� ��
    WORD bb[] = { 0x2, 0x3, 0x4 };  // �޸� �Ҵ��� �� �� ���̸� �������� �ϱ� ������
    // �� �ǿ����� ������̸� �ٸ��� ����. �� carry�� �߻����� �ʰ� ���� ��
    set_bint_from_word_array(&a, aa, 1); // ���� BINT ����ü�� ĳ����
    set_bint_from_word_array(&b, bb, 3);
    add_unsigned(&res, a, b);
    // ����
    assert(res != NULL);
    assert(res->wordlen == 3);          // carry ���� �� wordlen�� b�� ����(3)�� ���ƾ� ��
    
    // ���� �߻� ����
    assert(res->val[0] == 0x3);         // 0x1 + 0x2 == 0x3 ���� assert ��� ����
    // ���� ���� ���ɼ� 1 : a->val[i] �Ǵ� b->val[i]���� �߸��� �ʱ�ȭ
    // == set_bint_from_word_array()���� �߸� ���� �����ϴ� ����ε�, ���� �������� �׽�Ʈ��� ���� (�Ⱒ)
    // ���� ���� ���ɼ� 2: WORD�� �߸������ߴ�? (32��Ʈ�� �ƴѰ��) -> bint.h�� ����� ��Ȯ�� ���ǵǾ�����. sizeof(WORD) = 4���� Ȯ�� (�Ⱒ)
    // �迭 ���� ���� :  
    // b->val[0] = 0x00000004
    // b->val[1] = 0x00000003
    // b->val[2] = 0x00000002
    // �迭�� �������� ���� �ǰ� �־���
    // �̴� set_bint_from_word_array()�� �迭�� ���������ϱ� ��������,
    // arr[]�� ���� ������� ����Ǿ��ִٰ� ������ �Ǽ����� ������
    // ��ǲ �Ķ���͸� ���ڿ����� ���� �迭�� �޵��� �ٲپ��µ�, �� �κ��� ��ó �ٲ��� ����
    
    // ���� 0x1234567890ABCDEF �̶�� ����� �д� ���ڿ��� 0x12345678�� �ֻ���������.
    // �׷��� "���ڿ��� ���" ��������� �������� ����Ǿ����.
    //  val[0] == 0x90ABCDEF, val[1] == 0x12345678 �� ���� ä������,
    // �׷��� ����������� �����ϴ� add�Լ��� �浹�� �Ͼ ����.
    // ���� -> input �Ķ���͸� ���ڿ��� �ƴ� ���� �迭�� �ٲٴ� �������� bint�� ĳ�����ϴ� �Լ��� ����� ������� �ʾ�(������ ���ڿ����� �����迭) �浹 �߻�
    // set_bint_from_word_array() �׽�Ʈ �Լ� ���� �ʿ� -> �Է� ���� ������ �ϳ��� ������ �����Ѱ���(�Ҵ�Ǵ��� ���θ� Ȯ���ϰ� ������ Ȯ������ ����)
    
    assert(res->val[1] == 0x3);         // a���� word[1]�� �����Ƿ� b�� �� 0x3 �״��
    assert(res->val[2] == 0x4);         // ��

    // �޸� ����
    free_bint(&a);
    free_bint(&b);
    free_bint(&res);
}
// �̾, carry�� �߻��ϴ� ���
void test_add_unsigned_with_carry(void) {
    BINT* a = NULL, * b = NULL, * res = NULL;
    WORD aa[] = { 0xFFFFFFFF };
    WORD bb[] = { 0x1 };
    set_bint_from_word_array(&a, aa, 1); // 
    set_bint_from_word_array(&b, bb, 1);
    add_unsigned(&res, a, b);   // maxlen == 1�̰� (�� �� ����) carry�� ����� alloc = maxlen + 1 ( == 2)�� �޸� �Ҵ�Ǿ�� ����
    assert(res->alloc == 2);
    assert(res->wordlen == 2); // carry�� �߻��Ͽ� wordlen�� �����ؾ��Ѵ�.
    assert(res->val[0] == 0x00000000);
    assert(res->val[1] == 0x00000001);
    free_bint(&a); free_bint(&b); free_bint(&res);
}

void test_add_bint(void) { // add_bint (��ȣ ���� �Լ�)
    BINT* a = NULL, * b = NULL, * res = NULL;
    // extern bool sub_unsigned_called;
    // �Լ����� ��������� ��ȣ�� �ٸ� �ǿ������� ������ ������.
    // ������ �̰�� ������ ȣ��ǹǷ� ������� ���� �Ǵ��ؼ��� �ȵǰ� �� ���İ����� Ȯ���ϴ� ���� �߰� -> �⺻���� false
    // ȣ��Ǵ¼��� true�� ��ȯ�ɰ���. bint.c������ ����� ���̹Ƿ� extern���� ���������� ������ �� �� �ֵ��� ��
    
    // a + b
    WORD aa1[] = { 0x2 };
    WORD bb1[] = { 0x3 };
    set_bint_from_word_array(&a, aa1, 1);
    set_bint_from_word_array(&b, bb1, 1);
    add_bint(&res, a, b);
    assert(res->wordlen == 1);
    assert(res->val[0] == 0x5); // ������ �� ����Ǿ�����? (���������� 2������)
    assert(res->is_negative == false); // ��ȣ�� �� ���� �Ǿ�����? 
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
    assert(res->val[0] == 0x5); // ���� ����� ��������
    assert(res->is_negative == true); // ��ȣ ���� ����������?
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
    assert(sub_unsigned_called == true);  // ������ ȣ���ϴ� ��� ������ ȣ��Ǿ����� Ȯ��
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
    assert(sub_unsigned_called == true); // ���� �����Լ� ȣ�⿩�� Ȯ��
    free_bint(&a); free_bint(&b); free_bint(&res);
}

// ���� �� �Լ��� 10���� �Է½� �ʿ������� �Լ��� ������ �߰��� �ʿ��ϴ�.
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

// ��ƿ�Լ� ����
// ����Լ��� �������� ������ ������ � �ʿ��ϴ�.
void test_bint_is_zero_cases(void) {
    BINT* b = NULL;
    set_bint_from_uint64(&b, 0); // b�� val[0] == 0, wordlen == 1
    // wordlen�� 1�� ����̸鼭�� �迭 ���ΰ��� 0�̾�� 0��.
    assert(bint_is_zero(b));  // �������� 0
    b->val[0] = 1;
    assert(!bint_is_zero(b)); // val[0] == 1�̱� ������ false�� ��ȯ�Ǿ�� ����(�׷��Ƿ� !false �� assert)
    free_bint(&b);
}


void test_cmp_bint_basic(void) {  // ���� compare ��� ����
    BINT* a = NULL, * b = NULL;

    // ������̸� ������ ��Һ񱳸� �� �� �ִ� ���
    WORD w1[] = { 0x1 };
    WORD w2[] = { 0x1, 0x1 }; // ���� -> 0x1, 0x0�̾����� �̰�� normalize������ w1�� w2�� �������Ե�
    set_bint_from_word_array(&a, w1, 1); // ���⿡�� normalize�� ����ȣ��
    set_bint_from_word_array(&b, w2, 2);
    assert(cmp_bint(a, b) == -1);  // wordlen �񱳷� b�� �� ŭ
    free_bint(&a); free_bint(&b);
    
    // a == b
    WORD wa[] = { 0xABCD };
    WORD wb[] = { 0xABCD };
    set_bint_from_word_array(&a, wa, 1);
    set_bint_from_word_array(&b, wb, 1);
    assert(cmp_bint(a, b) == 0);  // a == b ������ 0
    free_bint(&a); free_bint(&b);

    // a < b
    WORD wc[] = { 0xABCD };
    WORD wd[] = { 0xABCE };
    set_bint_from_word_array(&a, wc, 1);
    set_bint_from_word_array(&b, wd, 1);
    assert(cmp_bint(a, b) == -1); // a < b �������� ũ�� -1
    free_bint(&a); free_bint(&b);

    // a > b
    WORD we[] = { 0xABCE };
    WORD wf[] = { 0xABCD };
    set_bint_from_word_array(&a, we, 1);
    set_bint_from_word_array(&b, wf, 1);
    assert(cmp_bint(a, b) == 1);  // a > b ������ ũ�� 1
    free_bint(&a); free_bint(&b);
}

void test_normalize_wordlen_trim(void) {
    BINT* b = NULL;
    init_bint(&b, 5); // val[] = {0x1, 0x0, 0x0, 0x0, 0x0}, wordlen = 5 �� ��������
    b->val[0] = 0x1;
    b->val[1] = 0x0;
    b->val[2] = 0x0;
    b->val[3] = 0x0;
    b->val[4] = 0x0;
    b->wordlen = 5;
    normalize_wordlen(b); // �Լ� ȣ���� ���� �ʿ���� ����� ���� trimmed�ǰ� wordlen�� 1�̵Ǿ����
    assert(b->wordlen == 1);
    free_bint(&b);
}

void test_sub_unsigned_basic(void) { // ��������� borrow�� �߻����� �ʴ� �⺻���� ���
    BINT* a = NULL, * b = NULL, * res = NULL;

    // a = 0x00001000 , b = 0x00000001 
    WORD wa[] = { 0x1000 };
    WORD wb[] = { 0x0001 };
    set_bint_from_word_array(&a, wa, 1);
    set_bint_from_word_array(&b, wb, 1);

    sub_unsigned(&res, a, b);

    // result == 0x0FFF �̾��.
    assert(res->wordlen == 1); // ���� ���̴� ������ �����ϴ�.
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

    // result = 0xFFFFFFFF (���� ����), 0x00000000 (���� ����) ���
    assert(res->wordlen == 1);           // ���� ����� 0�� �Ǿ� ���������� �߷����Ѵ�.
    assert(res->val[0] == 0xFFFFFFFF);   // 2^32 - 1 = 0xFFFFFFFF

    free_bint(&a); free_bint(&b); free_bint(&res);
}

