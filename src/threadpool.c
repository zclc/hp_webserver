#include "threadpool.h"

typedef enum {
    immediate_shutdown = 1,
    graceful_shutdown = 2
} zv_threadpool_sd_t;

static int threadpool_free(zv_threadpool_t *pool);
static void *threadpool_worker(void *arg);

/**
 * @brief 初始化线程
 * 
 * @param thread_num 线程池中线程的数量
 * @return zv_threadpool_t* 线程池的指针
 */
zv_threadpool_t *threadpool_init(int thread_num) {
    if (thread_num <= 0) {
        log_err("the arg of threadpool_init must greater than 0");
        return NULL;
    }

    zv_threadpool_t *pool;
    if ((pool = (zv_threadpool_t *)malloc(sizeof(zv_threadpool_t))) == NULL) {
        goto err;
    }

    pool->thread_count = 0; // 当前线程池中线程数量
    pool->queue_size = 0; // 任务队列中任务的数量
    pool->shutdown = 0; // 线程池是否关闭 0 不关闭
    pool->started = 0;  // 线程池是否开始 0 没有启动
    // 分配线程数组所需的空间
    pool->threads = (pthread_t *)malloc(sizeof(pthread_t) * thread_num);
    // 任务队列头结点不存储数据
    pool->head = (zv_task_t *)malloc(sizeof(zv_task_t));    /* dummy head */

    if ((pool->threads == NULL) || (pool->head == NULL)) {
        goto err;
    }

    pool->head->func = NULL;
    pool->head->arg = NULL;
    pool->head->next = NULL;

    // 初始化互斥量
    if (pthread_mutex_init(&(pool->lock), NULL) != 0) {
        goto err;
    }

    // 初始条件变量
    if (pthread_cond_init(&(pool->cond), NULL) != 0) {
        pthread_mutex_destroy(&(pool->lock));
        goto err;
    }
    
    // 创建线程并添加到线程池中
    int i;
    for (i=0; i<thread_num; ++i) {
        if (pthread_create(&(pool->threads[i]), NULL, threadpool_worker, (void *)pool) != 0) {
            threadpool_destroy(pool, 0);
            return NULL;
        }
        log_info("thread: %08x started", (uint32_t) pool->threads[i]);

        pool->thread_count++;
        pool->started++;
    }

    return pool;

err:
    if (pool) {
        threadpool_free(pool);
    }

    return NULL;
}

/**
 * @brief 往线程池中添加任务
 * 
 * @param pool 线程池实例
 * @param func 回调函数
 * @param arg 回调函数参数
 * @return int 0添加成功，其他不成功
 */
int threadpool_add(zv_threadpool_t *pool, void (*func)(void *), void *arg) {
    int rc, err = 0;
    if (pool == NULL || func == NULL) {
        log_err("pool == NULL or func == NULL");
        return -1;
    }
    
    if (pthread_mutex_lock(&(pool->lock)) != 0) {
        log_err("pthread_mutex_lock");
        return -1;
    }

    if (pool->shutdown) {
        err = zv_tp_already_shutdown;
        goto out;
    }
    
    // TODO: use a memory pool
    zv_task_t *task = (zv_task_t *)malloc(sizeof(zv_task_t));
    if (task == NULL) {
        log_err("malloc task fail");
        goto out;
    }
    
    // TODO: use a memory pool
    task->func = func;
    task->arg = arg;
    task->next = pool->head->next;
    pool->head->next = task;

    pool->queue_size++;
    
    rc = pthread_cond_signal(&(pool->cond));
    check(rc == 0, "pthread_cond_signal");

out:
    if(pthread_mutex_unlock(&pool->lock) != 0) {
        log_err("pthread_mutex_unlock");
        return -1;
    }
    
    return err;
}

/**
 * @brief 释放线程池资源
 * 
 * @param pool 线程池实例
 * @return int 0成功，其他值不成功
 */
int threadpool_free(zv_threadpool_t *pool) {
    if (pool == NULL || pool->started > 0) {
        return -1;
    }

    if (pool->threads) {
        free(pool->threads);
    }

    zv_task_t *old;
    /* pool->head is a dummy head */
    while (pool->head->next) {
        old = pool->head->next;
        pool->head->next = pool->head->next->next;
        free(old);
    }

    return 0;
}

/**
 * @brief 结束线程池生命
 * 
 * @param pool 线程池实例
 * @param graceful 1 等待其他线程退出后结束， 0  立即结束，不等其他线程结束
 * @return int 0 成功结束
 */
int threadpool_destroy(zv_threadpool_t *pool, int graceful) {
    int err = 0;

    if (pool == NULL) {
        log_err("pool == NULL");
        return zv_tp_invalid;
    }
    
    if (pthread_mutex_lock(&(pool->lock)) != 0) {
        return zv_tp_lock_fail;
    }
    
    do {
        // set the showdown flag of pool and wake up all thread    
        if (pool->shutdown) {
            err = zv_tp_already_shutdown;
            break;
        }

        pool->shutdown = (graceful)? graceful_shutdown: immediate_shutdown;
        
        if (pthread_cond_broadcast(&(pool->cond)) != 0) {
            err = zv_tp_cond_broadcast;
            break;
        }

        if (pthread_mutex_unlock(&(pool->lock)) != 0) {
            err = zv_tp_lock_fail;
            break;
        }
        
        int i;
        for (i=0; i<pool->thread_count; i++) {
            if (pthread_join(pool->threads[i], NULL) != 0) {
                err = zv_tp_thread_fail;
            }
            log_info("thread %08x exit", (uint32_t) pool->threads[i]);
        }
             
    } while(0);

    if (!err) {
        pthread_mutex_destroy(&(pool->lock));
        pthread_cond_destroy(&(pool->cond));
        threadpool_free(pool);
    }

    return err;
}

/**
 * @brief 线程工作函数（消费者线程）
 * 
 * @param arg 所属线程池实例
 * @return void* 
 */
static void *threadpool_worker(void *arg) {
    if (arg == NULL) {
        log_err("arg should be type zv_threadpool_t*");
        return NULL;
    }

    zv_threadpool_t *pool = (zv_threadpool_t *)arg;
    zv_task_t *task;

    while (1) {
        pthread_mutex_lock(&(pool->lock)); // 任务队列锁
        
        while ((pool->queue_size == 0) && !(pool->shutdown)){ // 没有任务且 线程池没有关闭
            pthread_cond_wait(&(pool->cond), &(pool->lock));
        }

        // 检查线程池关闭标志
        if (pool->shutdown == immediate_shutdown) {
            break;
        } else if ((pool->shutdown == graceful_shutdown) && pool->queue_size == 0) {
            break;
        }

        // 从任务队列第一个结点取任务
        task = pool->head->next;
        if (task == NULL) {
            pthread_mutex_unlock(&(pool->lock));
            continue;
        }

        pool->head->next = task->next;
        pool->queue_size--;

        pthread_mutex_unlock(&(pool->lock));

        (*(task->func))(task->arg);
        /* TODO: memory pool */
        // 任务队列 资源的申请是由添加任务函数做的，归还是由线程工作函数做的，仍是在线程池模块内完成的
        free(task);
    }

    // 线程池中运行的线程数
    pool->started--;
    pthread_mutex_unlock(&(pool->lock));
    pthread_exit(NULL);

    return NULL;
}
