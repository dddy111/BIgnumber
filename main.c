#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include "bint.h"
#include "test_bint.c"
// 여기 함수는 gpt 임시함수
void input_bint_from_user(BINT** p_bint) {
    char input[1024];

    printf("Enter a big integer in hexadecimal (with 0x prefix): ");
    if (scanf("%1023s", input) != 1) {
        fprintf(stderr, "Invalid input.\n");
        exit(1);
    }

    size_t len = strlen(input);
    if (len < 3 || !(input[0] == '0' && (input[1] == 'x' || input[1] == 'X'))) {
        fprintf(stderr, "Hex input must start with 0x or 0X.\n");
        exit(1);
    }

    const char* hex_str = input + 2;
    size_t hex_len = strlen(hex_str);
    size_t word_count = (hex_len + 7) / 8;

    WORD* temp = (WORD*)calloc(word_count, sizeof(WORD));
    if (!temp) {
        fprintf(stderr, "Memory allocation failed.\n");
        exit(1);
    }

    for (size_t i = 0; i < word_count; ++i) {
        size_t start = (hex_len > 8 * (i + 1)) ? hex_len - 8 * (i + 1) : 0;
        size_t count = (hex_len > 8 * (i + 1)) ? 8 : (hex_len - 8 * i);
        char chunk[9] = { 0 };
        strncpy(chunk, hex_str + start, count);
        temp[i] = (WORD)strtoul(chunk, NULL, 16);
    }
    set_bint_from_word_array(p_bint, temp, word_count);
    free(temp);
}

void add_bint_example(BINT* a, BINT* b) {
    BINT* result = NULL;
    add_bint(&result, a, b);  
    printf("Result of addition: ");
    print_bint_hex(result);  
    free_bint(&result);      
}


void sub_bint_example(BINT* a, BINT* b) {
    BINT* result = NULL;
    sub_unsigned(&result, a, b);  
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

    printf("Input for first BINT:\n");
    input_bint_from_user(&bint_a);

    printf("Input for second BINT:\n");
    input_bint_from_user(&bint_b);

    add_bint_example(bint_a, bint_b);  
    sub_bint_example(bint_a, bint_b);  
    cmp_bint_example(bint_a, bint_b); 

    free_bint(&bint_a);
    free_bint(&bint_b);

 


    
    return 0;
}

