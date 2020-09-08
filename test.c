#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "service_thread_pool.h"

static void *test(void *arg)
{
    int *p = (int *)arg;
    printf("task seq = %d\n", *p); 
}

int main()
{
    threadpool_t *pool = threadpool_create(2, 10, 5, 2);
    int i = 0;
    int *p = NULL;
    while(1)
    {
        if(1000 <= i) i = 0;
        p = (int *)malloc(sizeof(int));
        *p = i;
        threadpool_add_task(pool, test, p);
        i ++;
        //usleep(10000);  
    }
    return 0;
}