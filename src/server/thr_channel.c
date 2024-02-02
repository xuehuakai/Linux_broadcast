#include"thr_channel.h"


//明确联系哪个线程对应的哪个频道，所以数据结构放在.c当中隐藏起来
struct thr_channel_ent_sdt{
    chnid_t chnid;
    pthread_t tid;
};

struct thr_channel_ent_sdt thr_channel[CHNNR];
static int tid_nextpos = 0;

//对应频道的线程处理函数    组织好当前待发送的数据，通过socket发送出去
static void *thr_channel_snder(void* ptr){

    struct msg_channel_st *sbuf;
    struct mlib_listentry_st *ent = ptr;
    sbuf = malloc(MSG_CHANNEL_MAX);//申请最大的内存空间
    int len;
    if(sbuf==NULL)
        {
            syslog(LOG_ERR, "malloc();%s", strerror(errno));
            exit(1);
        }

        sbuf->chnid = ent->chnid;
while(1){
        len=mlib_readchn(ent->chnid, sbuf->data, MAX_DATA);
		syslog(LOG_DEBUG, "mlib_readchn() len: %d", len);
        if(sendto(server_sd, sbuf, len+sizeof(chnid_t),0,(void*)&sndaddr,sizeof sndaddr)<0)
        {
            syslog(LOG_ERR, "thr_channel(%d):send to ():%s", ent->chnid, strerror(errno));
        	break;
		}
		else

		{
			syslog(LOG_DEBUG,"thr_channel(%d):sendto() successed",ent->chnid);
	}

        sched_yield();//主动出让调度器
    }
    pthread_exit(NULL);//永远不会执行到
}


int thr_channer_create(struct mlib_listentry_st*ptr){//创建节目线程  给每个频道都创建一个 传入频道信息  ptr有频道信息
    int err;

    err=pthread_create(&thr_channel[tid_nextpos].tid,NULL,thr_channel_snder,ptr);  //线程标识回填了
    if(err){
        syslog(LOG_WARNING,"pthread_create():%s",strerror(err));
        return -err;
    }
    thr_channel[tid_nextpos].chnid = ptr->chnid; //把频道号放进去
    tid_nextpos++;
	return 0;
}

//销毁对应频道线程
int thr_channer_destory(struct mlib_listentry_st*ptr){

    int i;
    for (i = 0; i < CHNNR;i++){

        if(thr_channel[i].chnid==ptr->chnid){
            if(pthread_cancel(thr_channel[i].tid)<0){
                syslog(LOG_ERR,"pthread_cancel():the thread os channel %d",ptr->chnid);
                return -ESRCH;
            }
			}
             pthread_join(thr_channel[i].tid,NULL);
             thr_channel[i].chnid = -1;
           
            return 0;
        }
}
//销毁全部频道对应线程
int thr_channer_destoryadd(void){
        int i;
        for (i = 0; i < CHNNR;i++){
             if(thr_channel[i].chnid > 0)
        {
            if(pthread_cancel(thr_channel[i].tid) < 0)
            {
                syslog(LOG_ERR, "pthread_cancel():thr thread of channel:%d", thr_channel[i].tid);
                return -ESRCH;
            }
            pthread_join(thr_channel[i].tid, NULL);
            thr_channel[i].chnid =  -1;
        }
        }
}
