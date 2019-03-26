//
// Created by onceme on 2019/3/16.
//
#include "redis.h"
#include "ae.h"
#include "worker.h"


//通过连接的度事件触发      接收reactor线程触发的读事件
void workerReadConnHandle(aeEventLoop *el,int connfd, void *privdata, int mask){
    redisClient *c = (redisClient*) privdata;

    //从worker线程事件循环中删除这个连接 ，避免重复执行
    aeDeleteFileEvent(el,connfd,AE_WRITABLE);
    redisLog(REDIS_DEBUG,"workerReadHandle reactor_id %d connfd %d ",c->reactor_id,connfd);
    processInputBuffer(c);  //执行客户端操作命令

//    close(c->fd);
    //
    //将返回结果抛给原来的reactor线程 的操作

}

//通过监听管道，接收reactor线程触发的读事件
void workerPipeReadHandle(aeEventLoop *el,int pipfd, void *privdata, int mask){
    redisLog(REDIS_NOTICE,"workerReadHandle pipfd %d ",pipfd);
    thWorker *worker = privdata;
    int worker_id = worker->worker_id;
    int n;
    char buf[5];

    if ((n=read(pipfd, buf, 5)) != 5) {
//        if ((n=recv(pipfd, &c, sizeof(c), 0)) > 0){
        redisLog(REDIS_WARNING,"workerReadHandle Can't read for worker(id:%d) socketpairs[1](%d) n %d",worker->worker_id,pipfd,n);

    }
    redisLog(REDIS_VERBOSE,"workerReadHandle connfd %s ",buf);

    //TODO 从无锁队列从取出client信息
//    redisClient *c;
//    listNode *node;
    void *node;
    do{     //循环弹出所有队列
        node = atomListPop(server.worker[worker_id].clients);
        if(NULL==node){
            redisLog(REDIS_NOTICE,"listPop node null");
            break;
//            return;
        }
        redisClient *c = (redisClient*) node;

//        c = listNodeValue(node);
        redisLog(REDIS_VERBOSE,"workerReadHandle c %p connfd %d ",c,c->fd);

        //有可能取出的c结构体中数据为空(可能是队列有问题？)
        if(NULL==c || 0==c->fd){
            redisLog(REDIS_NOTICE,"workerReadHandle data empty reactor_id %d  c->request_times %d connfd %d  querybuf %s list len %lu",c->reactor_id, c->request_times,c->fd,c->querybuf,server.worker[worker_id].clients->len);
            freeClient(c);
            break;
//            return;

        }
        if(NULL==c->querybuf || sdslen(c->querybuf)<1){
            redisLog(REDIS_NOTICE,"workerReadHandle2 c->querybuf null c %p connfd %s  connfd %d ",c,buf,c->fd);

            continue;
//            return;

        }
        redisLog(REDIS_VERBOSE,"workerReadHandle2 c %p c->querybuf %s connfd %s  connfd %d ",c,c->querybuf,buf,c->fd);


    redisLog(REDIS_NOTICE,"workerReadHandle reactor_id %d  c->request_times %d connfd %d  querybuf %s list len %lu",c->reactor_id, c->request_times,c->fd,c->querybuf,server.worker[worker_id].clients->len);
        processInputBuffer(c);  //执行客户端操作命令
    }while(NULL!=node); //循环取队列
    redisLog(REDIS_VERBOSE,"workerhandel loop end");

}

/**
 * ReactorThread main Loop
 * 线程循环内容
 */
void rdWorkerThread_loop(int worker_id) {
    // 线程中的事件状态
    aeEventLoop *el;

    //线程id
    pthread_t thread_id = pthread_self();
    //创建事件驱动器
    el = aeCreateEventLoop(REDIS_MAX_CLIENTS);


    redisLog(REDIS_WARNING,"rdWorkerThread_loop worker_id %d ", worker_id);

    //存储线程相关信息
    server.worker[worker_id].worker_id = worker_id;
    server.worker[worker_id].pidt = thread_id;
    server.worker[worker_id].el = el;

    server.worker[worker_id].clients = atomListCreate();

    //监听本线程管道
    int readfd = server.worker[0].pipWorkerFd;
    if (aeCreateFileEvent(el,readfd,AE_READABLE,
                          workerPipeReadHandle, &server.worker[worker_id]) == AE_ERR)
    {
        redisPanic("Can't create the serverCron time event.");
        exit(1);
    }

    //进入事件循环
    aeMain(el);

}
