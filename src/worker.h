//
// Created by onceme on 2019/3/16.
//

#ifndef REDIS_3_0_ANNOTATED_WORKER_H
#define REDIS_3_0_ANNOTATED_WORKER_H

#endif //REDIS_3_0_ANNOTATED_WORKER_H



void workerReadConnHandle(aeEventLoop *el,int connfd, void *privdata, int mask);
void syncReadHandle(aeEventLoop *el,int connfd, void *privdata, int mask);
void workerPipeReadHandle(aeEventLoop *el,int connfd, void *privdata, int mask);
void rdWorkerThread_loop(int worker_id);

