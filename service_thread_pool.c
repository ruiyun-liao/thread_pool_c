#include "service_thread_pool.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>

static int is_thread_alive(pthread_t tid)
{
    if (ESRCH == pthread_kill(tid, 0)) 
    {
        return 0;
    }
    return 1;
}

static void *threadpool_thread(void *threadpool) 
{
  threadpool_t *pool = (threadpool_t *)threadpool;
  thread_task_t task;

  while (1)
  {
     pthread_mutex_lock(&(pool->lock_pool));

     while ((pool->queue_size == 0) && (!pool->wait_destory))
     { 
         printf("thread %d is waiting \n", (unsigned int)pthread_self());
         pthread_cond_wait(&(pool->queue_not_empty), &(pool->lock_pool));

         if (pool->thread_num_wait_exit > 0)
         {
             pool->thread_num_wait_exit--;
             if (pool->thread_num_alive > pool->thread_num_min)
             {
                 printf("thread %d exit\n", (unsigned int)pthread_self());
                 pool->thread_num_alive--;
                 pthread_mutex_unlock(&(pool->lock_pool));
                 pthread_exit(NULL);
             }
         }
     }

     if (pool->wait_destory) 
     {
         pthread_mutex_unlock(&(pool->lock_pool));
         printf("thread %d exit\n", (unsigned int)pthread_self());
         pthread_exit(NULL); 
     }

     task.function = pool->task_queue[pool->queue_front].function; 
     task.arg = pool->task_queue[pool->queue_front].arg;

     pool->queue_front = (pool->queue_front + 1) % pool->queue_max_size; 
     pool->queue_size--;

     pthread_cond_broadcast(&(pool->queue_not_full));

     pthread_mutex_unlock(&(pool->lock_pool));

     printf("thread %d work\n", (unsigned int)pthread_self());
     pthread_mutex_lock(&(pool->thread_counter));                
     pool->thread_num_busy++;
     pthread_mutex_unlock(&(pool->thread_counter));

     (*(task.function))(task.arg);                                   

 
     printf("thread %d end work\n", (unsigned int)pthread_self());
     pthread_mutex_lock(&(pool->thread_counter));
     pool->thread_num_busy--;
     pthread_mutex_unlock(&(pool->thread_counter));
  }

  pthread_exit(NULL);
}

static void *admin_thread(void *threadpool)
{
    int i;
    threadpool_t *pool = (threadpool_t *)threadpool;
    while (!pool->wait_destory)
    {
        printf("********admin thread working********\n");
        sleep(5);                                     
        pthread_mutex_lock(&(pool->lock_pool));                 
        int queue_size = pool->queue_size;                
        int thread_num_alive = pool->thread_num_alive;    
        pthread_mutex_unlock(&(pool->lock_pool));      

        pthread_mutex_lock(&(pool->thread_counter));
        int thread_num_busy = pool->thread_num_busy;           
        pthread_mutex_unlock(&(pool->thread_counter));

        printf("busy-%d    alive-%d\n", thread_num_busy, thread_num_alive);

		int thread_num_free = thread_num_alive - thread_num_busy;
        if (queue_size >= thread_num_free && thread_num_alive <= pool->thread_num_max)
        {
            pthread_mutex_lock(&(pool->lock_pool));
            int new_thread_num = 0;

            for (i=0; i < pool->thread_num_max && new_thread_num < pool->thread_num_operate 
                                  && pool->thread_num_alive < pool->thread_num_max; i++)
            {
                if (pool->threads[i] == 0 || !is_thread_alive(pool->threads[i]))
                {
                    pthread_create(&(pool->threads[i]), NULL, threadpool_thread, (void *)pool);
                    new_thread_num++;
                    pool->thread_num_alive++;
                    printf("admin create thread %ld\n", pool->threads[i]);
                }
            }

            pthread_mutex_unlock(&(pool->lock_pool));
        }

        if ((thread_num_busy*2) < thread_num_alive  &&  thread_num_alive > (pool->thread_num_min + pool->thread_num_operate))
        {
            pthread_mutex_lock(&(pool->lock_pool));
            pool->thread_num_wait_exit = pool->thread_num_operate;
            pthread_mutex_unlock(&(pool->lock_pool));

            for (i = 0; i < pool->thread_num_operate; i++)
            {
                pthread_cond_signal(&(pool->queue_not_empty));
                printf("admin clear thread\n");
            }
        }

    }

    return NULL;
}


static int threadpool_free(threadpool_t *pool)
{
    if (pool == NULL)
      return -1;
    if (pool->task_queue)
        free(pool->task_queue);
    if (pool->threads)
    {
        free(pool->threads);
        pthread_mutex_lock(&(pool->lock_pool));                
        pthread_mutex_destroy(&(pool->lock_pool));
        pthread_mutex_lock(&(pool->thread_counter));
        pthread_mutex_destroy(&(pool->thread_counter));
        pthread_cond_destroy(&(pool->queue_not_empty));
        pthread_cond_destroy(&(pool->queue_not_full));
    }
    free(pool);
    pool = NULL;

    return 0;
}

threadpool_t *threadpool_create(int thread_num_min, int thread_num_max, int queue_max_size, int thread_num_operate)
{                   
    int i;
    threadpool_t *pool = NULL;
    do
    {
    	pool = (threadpool_t *)malloc(sizeof(threadpool_t));
        if (NULL == pool)
        {
            printf("malloc threadpool fail\n");
            break;    
        }
		
		pool->thread_num_min = thread_num_min;
		pool->thread_num_max = thread_num_max;
		pool->thread_num_busy = 0;
		pool->thread_num_alive = thread_num_min;
		pool->thread_num_wait_exit = 0;
		pool->thread_num_operate = thread_num_operate;
		pool->queue_front = 0;
		pool->queue_rear = 0;
		pool->queue_size = 0;
		pool->queue_max_size = queue_max_size;
		pool->wait_destory = 0;

        pool->threads = (pthread_t *)malloc(sizeof(pthread_t) * thread_num_max);
        if(NULL == pool->threads)
        {
            printf("malloc threads fail\n");
            break;
        }
        memset(pool->threads, 0, sizeof(pthread_t) * thread_num_max);

        pool->task_queue = (thread_task_t *)malloc(sizeof(thread_task_t) * queue_max_size);
        if(NULL == pool->task_queue)
        {
            printf("malloc task queue fail\n");
            break;
        }
 
        if(0 > pthread_mutex_init(&(pool->lock_pool), NULL) ||  0 > pthread_mutex_init(&(pool->thread_counter), NULL))
    	{
			printf("malloc lock fail\n");
            break;
    	}
               
		if(0 > pthread_cond_init(&(pool->queue_not_empty), NULL) ||  0 > pthread_cond_init(&(pool->queue_not_full), NULL))
		{
			printf("malloc cond fail\n");
            break;
		}
		
        for (i = 0; i < thread_num_min; i++)
        {
            pthread_create(&(pool->threads[i]), NULL, threadpool_thread, (void *)pool);
            printf("init thread %d... \n", (unsigned int)pool->threads[i]);
        }
		
        pthread_create(&(pool->admin_thread), NULL, admin_thread, (void *)pool);

        return pool;
    } while(0);

    threadpool_free(pool);
    return NULL;
}


int threadpool_destroy(threadpool_t *pool)
{
    int i;
    if (pool == NULL)
    {
      return -1;
    }
    pool->wait_destory = 1;

    pthread_join(pool->admin_thread, NULL);

    for (i=0; i<pool->thread_num_alive; i++)
    {
      pthread_cond_broadcast(&(pool->queue_not_empty));
    }

    for (i=0; i<pool->thread_num_alive; i++)
    {
      pthread_join(pool->threads[i], NULL);
    }

    threadpool_free(pool);
    return 0;
}

int threadpool_add_task(threadpool_t *pool, void *(*function)(void *arg), void *arg)
{
    pthread_mutex_lock(&(pool->lock_pool));

    while ((pool->queue_size == pool->queue_max_size) && (!pool->wait_destory))
        pthread_cond_wait(&(pool->queue_not_full), &(pool->lock_pool));

    if (pool->wait_destory)
    {
        pthread_mutex_unlock(&(pool->lock_pool));
        return -1;
    }

    if (pool->task_queue[pool->queue_rear].arg != NULL)
    {
        free(pool->task_queue[pool->queue_rear].arg);
        pool->task_queue[pool->queue_rear].arg = NULL;
    }

    pool->task_queue[pool->queue_rear].function = function;
    pool->task_queue[pool->queue_rear].arg = arg;
    pool->queue_rear = (pool->queue_rear + 1) % pool->queue_max_size;  
    pool->queue_size++;

    pthread_cond_signal(&(pool->queue_not_empty));
    pthread_mutex_unlock(&(pool->lock_pool));

    return 0;
}





