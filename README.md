# thread_pool_c
Implementing thread pools using C

test.c   A simple example of using thread pools.

service_thread_pool.c : The actual implementation of the thread pool.

service_thread_pool.h : The function interface left for the user.
    
    threadpool_t *threadpool_create(int thread_num_min, int thread_num_max, int queue_max_size, int thread_num_operate);
    int threadpool_destroy(threadpool_t *pool);
    int threadpool_add_task(threadpool_t *pool, void *(*function)(void *arg), void *arg);
