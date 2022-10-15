// 多线程
#define _GNU_SOURCE

#include <stdint.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sched.h>   // cpu ?
#include <pthread.h> // need gcc -pthread

#define PAGE_BYTES (4096) // 4 KB

int64_t result_page0[PAGE_BYTES / (int)sizeof(int64_t)];
int64_t result_page1[PAGE_BYTES / (int)sizeof(int64_t)];
int64_t result_page2[PAGE_BYTES / (int)sizeof(int64_t)];
int64_t result_page3[PAGE_BYTES / (int)sizeof(int64_t)];


typedef struct 
{
    int64_t *cache_write_ptr; // to write
    int cpu_id;
    int length;
} param_t;

void *work_thread(void *param) // parameter
{
    param_t *p = (param_t *)param;
    int64_t *ptr   = p->cache_write_ptr;
    int cpu_id = p->cpu_id;
    int length = p->length;

    cpu_set_t mask;
    CPU_ZERO(&mask);
    CPU_SET(cpu_id, &mask);

    // 亲和cpu
    pthread_setaffinity_np(pthread_self(), sizeof(mask), &mask);

    printf("    *thread[%lu] running on cpu[%d] writes to %p\n", pthread_self(), sched_getcpu(), ptr);


    for(int i = 0; i < length; i ++ )
    {
        // write 
        *ptr += 1;
    }

    return NULL;
}

// 在下面的例子中，我们没有关心线程安全，即没有进行线程同步，所以数据可能有问题。 
// 我们知识希望它们在某个物理地址进行写操作验证效率问题
int LENGTH = 1234123412;
void true_sharing_run()  // 效率很低，因为需要频繁读写dram
{
    pthread_t t1, t2;

    param_t p1 = {
        .cache_write_ptr = &result_page0[0],
        .cpu_id = 0,
        .length = LENGTH
    };
    
    param_t p2 = {
        .cache_write_ptr = &result_page0[0],
        .cpu_id = 1,
        .length = LENGTH
    };

    long t0 = clock();

    pthread_create(&t1, NULL, work_thread, (void *)&p1);
    pthread_create(&t2, NULL, work_thread, (void *)&p2);
    
    // main should wait for thread complete
    pthread_join(t1, NULL);
    pthread_join(t2, NULL);

    printf("[True Sharing]\n\telapsed tick tock: %ld\n", clock() - t0);
    printf("result[0] = %ld;\n", result_page0[0]);
}


void false_sharing_run() 
{
    pthread_t t1, t2;

    param_t p1 = {
        .cache_write_ptr = &result_page1[0],
        .cpu_id = 0,
        .length = LENGTH
    };
    
    param_t p2 = {
        .cache_write_ptr = &result_page1[1],
        .cpu_id = 1,
        .length = LENGTH
    };

    long t0 = clock();

    pthread_create(&t1, NULL, work_thread, (void *)&p1);
    pthread_create(&t2, NULL, work_thread, (void *)&p2);
    
    // main should wait for thread complete
    pthread_join(t1, NULL);
    pthread_join(t2, NULL);

    printf("[False Sharing]\n\telapsed tick tock: %ld\n", clock() - t0);
    printf("result[0] = %ld;\tresult[1]=%ld\n", result_page1[0], result_page1[1]);
}


void no_sharing_run() 
{
    pthread_t t1, t2;

    param_t p1 = {
        .cache_write_ptr = &result_page2[0],
        .cpu_id = 0,
        .length = LENGTH
    };
    
    param_t p2 = {
        .cache_write_ptr = &result_page3[0],
        .cpu_id = 1,
        .length = LENGTH
    };

    long t0 = clock();

    pthread_create(&t1, NULL, work_thread, (void *)&p1);
    pthread_create(&t2, NULL, work_thread, (void *)&p2);
    
    // main should wait for thread complete
    pthread_join(t1, NULL);
    pthread_join(t2, NULL);

    printf("[No Sharing]\n\telapsed tick tock: %ld\n", clock() - t0);
    printf("result[0] = %ld;\tresult[1]=%ld\n\n", result_page2[0], result_page3[0]);
}


int main()
{
    true_sharing_run();
    false_sharing_run();
    no_sharing_run();
    printf("pass normally!\n");
}