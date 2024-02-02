#ifndef THR_LIST_H__
#define THR_LIST_H__
#include"medialib.h"
//将线程的创建和线程的销毁进行封装

int thr_list_create(struct mlib_listentry_st*,int);
int thr_list_destory(void);

#endif
