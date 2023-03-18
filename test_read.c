#define _GNU_SOURCE

#include <stdlib.h>
#include <string.h>
#include <libaio.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <time.h>
#include "stderrmsg.h"

#define FILEPATH "./aio.txt"

int test_main(); //basic single read test...
void *test_main2(void * args);
int test_main2_threads(void);

int64_t timespecDiff(struct timespec *timeA_p, struct timespec *timeB_p)
{
  return ((timeA_p->tv_sec * 1000000000) + timeA_p->tv_nsec) -
           ((timeB_p->tv_sec * 1000000000) + timeB_p->tv_nsec);
}


int main(int argc, char const *argv[])
{
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    test_main2_threads();
    clock_gettime(CLOCK_MONOTONIC, &end);
    printf("time eslaped [%ld] \n", timespecDiff(&end, &start));

    return 0;
}



int io_submit_whit_retry(io_context_t ctx, long nr, struct iocb *iocbs[])
{
    int ret, counter = 0, rnr = nr;
    while(counter != nr) {
        ret = io_submit(ctx, rnr, &iocbs[counter]);
        if(ret < 0 ) {
            if(ret != -EAGAIN)
                err_sys("io_submit : [%d] ", ret);
            //err_ret("io_submit err : [%d] ", ret);
            io_queue_run(ctx);
            continue;
        }
        //err_ret("io_submit ok: [%d] ", ret);
        counter += ret;
        rnr -= ret;
    }
    return counter;
}



void aioCbFunc(io_context_t ctx, struct iocb *iocb, long res, long res2)
{
    //printf(" io cb res [%ld] res2 [%ld]\n", res, res2);
}


struct main2Args {
    int idx;
    io_context_t context;
};

int test_main2_threads(void) {
    #define TEST_THREADS 10
    pthread_t pids[TEST_THREADS];
    struct main2Args argArr[TEST_THREADS];
    int ret;

    io_context_t context;
    memset(&context, 0, sizeof(io_context_t));
    ret = io_queue_init(10, &context);
    if(ret != 0)
        err_sys(" io init err ");
    
    for (size_t i = 0; i < TEST_THREADS; i++)
    {
        argArr[i].idx = i;
        argArr[i].context = context;

        ret = pthread_create(&pids[i], NULL, test_main2, &argArr[i]);
        if(ret != 0)
            err_sys(" p cre err");
    }

    for (size_t i = 0; i < TEST_THREADS; i++)
    {
        pthread_join(pids[i], NULL);
    }
    
    
}


/**
 * reason: if not use O_DIRECT, then the is sync ----> io_genevnets is return blocked...
 * 
 * test mode:
 */
void *test_main2(void * args)
{
    struct main2Args *mArgs = (struct main2Args *)args;
    io_context_t context = mArgs->context;

    int nEvents = 1024;
    struct iocb io[nEvents], *p[nEvents];
    for (size_t i = 0; i < nEvents; i++)
    {
        p[i] = &io[i];
    }
    
    struct io_event e[nEvents];
    unsigned nr_events = 10;
    struct timespec timeout;
    char *wbuf;
    char *wbuf2;
    int wbuflen = 40960 * 5;
    int ret, num = 0, i;

    posix_memalign((void **)&wbuf, 512, wbuflen);
    posix_memalign((void **)&wbuf2, 512, wbuflen);
    memset(wbuf, 'A', wbuflen);
    memset(wbuf2, '_', wbuflen);

    timeout.tv_sec = 0;
    timeout.tv_nsec = 1000000;

    int fd;
    char fileName[50];
    sprintf(fileName, FILEPATH "_%d", mArgs->idx);
    fd = open(fileName, O_CREAT | O_RDWR | O_DIRECT, 0644);

    if (fd < 0) {
        printf("open error: %d\n", errno);
        return NULL;
    }

    for (size_t i = 0; i < nEvents; i++)
    {
        io_prep_pwrite(&io[i], fd, i % 2 == 0 ? wbuf : wbuf2, wbuflen, 4096 + wbuflen * i);
        io_set_callback(&io[i], aioCbFunc);
        (&io[i])->key = i;
    }


    if ((ret = io_submit_whit_retry(context, nEvents, p)) != nEvents)
    {
        err_ret("io_submit error: [%d] ", ret);
        return NULL;
    }

    // int retCnt = 0;
    // struct io_event eRet[nEvents];
    // while (1) {
    //     ret = io_getevents(context, 0, nEvents, eRet, &timeout);    
    //     if (ret < 0) {
    //         err_ret("io_getevents error: %d ", ret);
    //         break;
    //     }

    //     if (ret > 0) {
    //         retCnt += ret;
    //     }

    //     if(retCnt == nEvents) {
    //         break;
    //     }

    //     printf("........\n");
    // }

    io_queue_run(context);

}



int test_main()
{
    io_context_t context;
    struct iocb io[1], *p[1] = {&io[0]};
    struct io_event e[1];
    unsigned nr_events = 10;
    struct timespec timeout;
    char *wbuf;
    int wbuflen = 1024;
    int ret, num = 0, i;

    posix_memalign((void **)&wbuf, 512, wbuflen);

    memset(&context, 0, sizeof(io_context_t));

    timeout.tv_sec = 0;
    timeout.tv_nsec = 10000000;

    int fd = open(FILEPATH, O_CREAT|O_RDWR|O_DIRECT, 0644); // 1. 打开要进行异步IO的文件
    if (fd < 0) {
        printf("open error: %d\n", errno);
        return 0;
    }

    if (0 != io_setup(nr_events, &context)) {               // 2. 创建一个异步IO上下文
        printf("io_setup error: %d\n", errno);
        return 0;
    }

    //io_prep_pwrite(&io[0], fd, wbuf, wbuflen, 0);           // 3. 创建一个异步IO任务
    io_prep_pread(&io[0], fd, wbuf, wbuflen, 512);

    if ((ret = io_submit(context, 1, p)) != 1) {            // 4. 提交异步IO任务
        printf("io_submit error: %d\n", ret);
        io_destroy(context);
        return -1;
    }

    while (1) {
        ret = io_getevents(context, 1, 1, e, &timeout);     // 5. 获取异步IO的结果
        if (ret < 0) {
            printf("io_getevents error: %d\n", ret);
            break;
        }

        if (ret > 0) {
            printf("result, res2: %lu, res: %lu\n", e[0].res2, e[0].res);
            printf("read context: [%s]\n", wbuf);
            break;
        }
    }

    return 0;
}

