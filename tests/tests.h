/**
 * 							ULIPE RTOS PICO
 *
 *  @file tests.h
 *
 *  @brief test master file
 *
 */

#ifndef __TEST_H
#define __TEST_H

#if(ENABLE_TESTING > 0)

/* test sequence package, it will used to call the tests in chain */
typedef struct test_seq{
    char *test_name;
    void *test_api;
    void *test_data;
    int (*test_entry)(struct test_seq * seq);
} test_seq_t;


/** enable or disable test of individually components of kernel */
#define TEST_THREAD         0
#define TEST_TIMERS         0
#define TEST_SEMA           0
#define TEST_MUTEX          0
#define TEST_MESSAGE        0
#define TEST_MALLOC         0 


#endif
#endif