
#include"../include/proto.h"
#ifndef MEDIALIB_H__
#define MEDIALIB_H__

struct mlib_listentry_st{   //数据结构的封装 抽象给别的模块看的  因为我们创建节目单的时候实际上只需要ID+描述
    chnid_t chnid;
    char *  desc;
};

int mlib_getchnlist(struct mlib_listentry_st **, int *);//参数1：二级指针 用户传来一个一级指针变量(结构体数组的起始位置)的地址，我来给它回填
int mlib_freechnlist(struct mlib_listentry_st *);

ssize_t mlib_readchn(chnid_t, void*, size_t); //从哪里读，读到哪里去，读取的字节数
#endif
