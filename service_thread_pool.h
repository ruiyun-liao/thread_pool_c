#ifndef __SERVICE_THREAD_POOL_H__
#define __SERVICE_THREAD_POOL_H__

#include <pthread.h>

typedef struct {
    void *(*function)(void *);
    void *arg;
} thread_task_t;


typedef struct {
    pthread_t *threads;            
    pthread_t admin_thread;        

    int thread_num_min;            
    int thread_num_max;            
    int thread_num_alive;          
    int thread_num_busy;           
    int thread_num_wait_exit;      
    int thread_num_operate;
	
    thread_task_t *task_queue;     
    int queue_max_size;                  
    pthread_cond_t  queue_not_full;      
    pthread_cond_t  queue_not_empty;     
    int queue_front;                     
    int queue_rear;                      
    int queue_size; 

    int wait_destory;                    

    pthread_mutex_t lock_pool;           
    pthread_mutex_t thread_counter;      
}threadpool_t;

/**
 * @name        threadpool_create
 * @breif       create a thread pool
 * @param[in]   thread_num_min		minimum number of threads in the thread pool
 * @param[in]   thread_num_max		maximum number of threads in the thread pool
 * @param[in]   queue_max_size		maximum size of task queue
 * @param[in]   thread_num_operate	increase or decrease the number of threads each time
 * @retval      threadpool_t *		a pointer to a thread pool
 */
threadpool_t *threadpool_create(int thread_num_min, int thread_num_max, int queue_max_size, int thread_num_operate);

/**
 * @name        threadpool_destroy
 * @breif       destroy a thread pool
 * @param[in]   pool	 the thread pool you want to destroy
 * @retval      int		 error code           
 */
int threadpool_destroy(threadpool_t *pool);

/**
 * @name        threadpool_add_task
 * @breif       add a task to queue of thread pool
 * @param[in]   pool		  the thread pool you want to use
 * @param[in]   function	  the function you want to run
 * @param[in]   arg           the parameters of the function you want to run
 * @retval      int		      error code
 * @warn   the arg must to be created in malloc()
 */
int threadpool_add_task(threadpool_t *pool, void *(*function)(void *arg), void *arg);

#endif
