#ifndef THR
#define THR
#include <syslog.h>
#include <pthread.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "server_conf.h"
#include "medialib.h"
#include "../include/proto.h"


int thr_channer_create(struct mlib_listentry_st*);//创建节目线程  给每个频道都创建一个 传入频道信息
int thr_channer_destory(struct mlib_listentry_st*);
int thr_channer_destoryadd(void); //all

#endif
