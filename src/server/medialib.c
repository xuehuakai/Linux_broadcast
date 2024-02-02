#include <stdio.h>
#include <stdlib.h>
#include <glob.h>
#include <syslog.h>
#include <string.h>
#include <errno.h>
#include <error.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "server_conf.h"
#include "medialib.h"
#include "../include/proto.h"
#include "mytbf.h"
#define PATH_SIZE 1024 //存放路径
//真正用到的数据结构隐藏在.C文件中


//每一个频道都拥有这样的结构体
typedef struct channel_context_st{
		chnid_t chnid;  //ID
		char *desc; //描述
		glob_t mp3glob;
		int pos;//下标表示
		int fd;//文件描述符
		off_t offset;
		mytbf_t *tbf; //流量控制  记住：播放器，解码器一定要进行流量控制，就好比QT聊天室进行发送图片一样，不进行处理，就会产生粘包等问题
}st_channel;

/*开环流控：当前只需要实现流通算法而不进行对方是否接到了这样的一个反馈信息的确认。
  闭环流控：必须要对方进行反馈信息的确认。*/

/*数组存放当前所有节目的信息  不用0号位*/

st_channel channel[MAXCHNID + 1];//101个 但是实际上我目前就3个频道

#define LINEBUFSIZE 1024
#define MP3_BITRATE 64*1024
static struct channel_context_st *path2entry(const char* path)
{
		syslog(LOG_INFO, "current path :%s\n", path);
		char pathstr[PATH_SIZE] = {'\0'};
		char linebuf[LINEBUFSIZE];
		FILE *fp;
		struct channel_context_st *me;
		static chnid_t curr_id = MINCHNID;
		strcat(pathstr, path);
		strcat(pathstr, "/desc.txt");
		fp = fopen(pathstr, "r");
		syslog(LOG_INFO, "channel dir:%s\n", pathstr);
		if(fp == NULL)
		{
				syslog(LOG_INFO, "%s is not a channel dir(can't find desc.txt)", path);
				return NULL;
		}
		if(fgets(linebuf, LINEBUFSIZE, fp) == NULL)
		{
				syslog(LOG_INFO, "%s is not a channel dir(cant get the desc.text)", path);
				fclose(fp);
				return NULL;
		}
		fclose(fp);
		me = malloc(sizeof(*me));
		if(me == NULL)
		{
				syslog(LOG_ERR, "malloc():%s", strerror(errno));
				return NULL;
		}

		me->tbf = mytbf_init(MP3_BITRATE/8, MP3_BITRATE/8*5);
		if(me->tbf == NULL)
		{
				syslog(LOG_ERR, "mytbf_init():%s", strerror(errno));
				free(me);
				return NULL;
		}
		me->desc = strdup(linebuf);
		strncpy(pathstr, path, PATH_SIZE);
		strncat(pathstr, "/*.mp3", PATH_SIZE);
		if(glob(pathstr, 0, NULL, &me->mp3glob) != 0)
		{
				curr_id++;
				syslog(LOG_ERR, "%s is not a channel dir(can not find mp3 files", path);
				free(me);
				return NULL;
		}
		me->pos = 0;
		me->offset = 0;
		me->fd = open(me->mp3glob.gl_pathv[me->pos], O_RDONLY);
		if(me->fd <0 )
		{
				syslog(LOG_WARNING, "%s open failed.",me->mp3glob.gl_pathv[me->pos]);
				free(me);
				return NULL;
		}
		me->chnid = curr_id;
		curr_id++;
		return me;
}
int mlib_getchnlist(struct mlib_listentry_st **res, int *res_num){  //负责把mlib_listentry_st这样的结构体数组的起始位置 回填给用户
		char path[PATH_SIZE];
		glob_t globres;
		//对数组初始化
		int i,num=0;
		struct mlib_listentry_st *ptr;
		st_channel *ans;



		for (i = 0; i < MAXCHNID;i++)
		{
				channel[i].chnid=-1;
		}

		snprintf(path,PATH_SIZE,"%s/*",server_conf.media_dir);//打印媒体库当前的位置在哪里  /*代表着解析这个目录下面的内容

		if(glob(path,0,NULL,&globres))/*glob解析当前媒体库   看看从当前媒体库中能解析出来多少目录 eg:3  解析出来后
										对目录里面的内容进行二次解析，拿到mp3文件和desc文件*/
		{
				return -1;
		}
		ptr = malloc(sizeof(struct mlib_listentry_st) * globres.gl_pathc);
		for (i = 0;i<globres.gl_pathc;i++){
				//globres.gl_pathv[i] -> "/var/media/ch1"

			
				ans=path2entry(globres.gl_pathv[i]); //把路径变成每一条记录
				//path2entry解析出来所有的mp3
				if(ans!=NULL){

				syslog(LOG_ERR,"path2entry() return : %d %s .",ans->chnid,ans->desc);
				memcpy(channel+ans->chnid,ans,sizeof(*ans));
				ptr[num].chnid=ans->chnid;
				ptr[num].desc=strdup(ans->desc);
				num++; //每次解析出来一个合适的目录结构的话，自增num
			}
		}

		*res = realloc(ptr, sizeof(st_channel) * num);

		*res_num = num;//目录结构数目
		return 0;
}



int mlib_freechnlist(struct mlib_listentry_st *ptr){
		free(ptr);
		ptr=NULL;
		return 0;
}

//打开下一个  当前是失败了或者已经读取完毕才会调用open_next
static int open_next(chnid_t chnid){
		int i;
		for (i = 0; i < channel[chnid].mp3glob.gl_pathc;++i){
				channel[chnid].pos++;

				if(channel[chnid].pos==channel[chnid].mp3glob.gl_pathc) //当前pos等于最大值  也就是一个都没有打开  要么从头开始一遍
				{
						channel[chnid].pos = 0;
						return -1;
						break;
				}

				close(channel[chnid].fd);
				channel[chnid].fd=open(channel[chnid].mp3glob.gl_pathv[channel[chnid].pos],O_RDONLY); //打开gl_pathv里的这个歌名 read Only
				if(channel[chnid].fd<0){
						syslog(LOG_WARNING,"open(%s):%s",channel[chnid].mp3glob.gl_pathv[channel[chnid].pos],strerror(errno)); //日志记录这首歌打开失败
				}
				else  //成功
				{
						channel[chnid].offset = 0;//这个新的文件从头开始读
						return 0;
				}
		}

		syslog(LOG_ERR, "None of mp3s in channel %d is available", chnid);
}


ssize_t mlib_readchn(chnid_t chnid, void*buf , size_t size){ //从哪里读，读到哪里去，读取的字节数
		int tbfsize,len,next_ret=0;
		tbfsize=mytbf_fetchtoken(channel[chnid].tbf,size);//进行流量限制

		while(1){
				//从当前频道（结构体）当中读取内容放到buf中
				len=pread(channel[chnid].fd, buf, tbfsize, channel[chnid].offset);//从一个文件的offset开始读tbfsize个字节到buf中
				if(len<0){
						//这个文件有问题，转而读取下一首歌

						open_next(chnid);
				}
				else if(len==0){
						syslog(LOG_WARNING,"media file :%s pread():%s",channel[chnid].mp3glob.gl_pathv[channel[chnid].pos],strerror(errno));//播放完毕的提示
						next_ret=open_next(chnid);
						break;
				}
				else{
						syslog(LOG_DEBUG, "media file %s is over.", channel[chnid].mp3glob.gl_pathv[channel[chnid].pos]);
						channel[chnid].offset += len;
						break;
				}
		}

		if(tbfsize-len>0)
				mytbf_returntoken(channel[chnid].tbf, tbfsize - len);//归还

return len;
}


