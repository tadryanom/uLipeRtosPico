/**
 * 							ULIPE RTOS PICO
 *
 *  @file tests.c
 *
 *  @brief test master file
 *
 */

#include "tests.h"
#include "../ulipe_rtos_pico.h"

#if(ENABLE_TESTING > 0)


/** test sequence table */
test_seq_t *test_tbl[] = {

#if(TEST_THREAD > 0)
    &thread_test_set,
#endif

#if(TEST_TIMERS > 0)
    &timers_test_set,
#endif

#if(TEST_SEMA > 0)
    &sema_test_set,
#endif

#if(TEST_MUTEX > 0)
    &mutex_test_set,
#endif

#if(TEST_MESSAGE > 0)
    &message_test_set,
#endif

#if(TEST_MALLOC > 0)
    &malloc_test_set
#endif
    NULL
};


uint32_t noof_tests = sizeof(test_tbl)/sizeof(test_seq_t *);

/**
 *  @fn test_main()
 *  @brief main test point entry point 
 *  @param  n/a
 *  @return n/a
 */
void test_main(void)
{
    ULIPE_ASSERT(noof_tests > 1);
    printf("%s: starting the test system of ulipe pico \n\r", __func__)

    test_seq_t *ptr = test_tbl[0];
    uint32_t i = 0;

    uint32_t passed = 0;
    uint32_t failed = 0;

    while(ptr++) {
        if(!ptr->entry(ptr->test_api, ptr->test_data)) {
            passed++;
            printf("%s: test: %s -- passed \n\r", __func__, ptr->test_name);        
        } else {
            failed++;
            printf("%s: test: %s -- failed \n\r", __func__, ptr->test_name);                    
        }

    }

    printf("%s: all tests %d were executed -- passed: %d -- failed: %d \n\r", __func__, noof_tests - 1, passed, failed);                        
    printf("%s: ending the test system of ulipe pico \n\r", __func__);    
    thread_suspend(NULL);
}


/**
 *  @fn main
 *  @brief entry point of test bed 
 *  @param  n/a
 *  @return n/a
 */
int main(int argc, char **argv)
{
    tid_t test_thread;

    kernel_init();

    test_thread = thread_create(1024, 16);
    ULIPE_ASSERT(test_thread != NULL);
    thread_start(test_main, NULL, test_thread);

    kernel_start();
    return 0;
}
#endif
