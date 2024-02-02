#ifndef PROTO_H__
#define PROTO_H__

#include<site_type.h>

//channel------频道			list-------节目单    0号频道发节目单



#define DEFAULT_MGROUP 			"224.2.2.2"   、/*多播组地址*/
#define DEFAULT_RCVPORT 		"1989"

#define CHNNR 					100 /*总的频道数*/
#define LISTCHNID				0 	/*零号频道主要往外发送节目单*/
#define MINCHNID 				1  	/*最小的频道ID*/
#define MAXCHNID 				(MINCHNID+CHNNR-1)	/*最大的频道ID*/

#define MSG_CHANNEL_MAX			(65536-20-8)  /*包最大长度				一个推荐报的长度减去IP报头再减去UDP报的报头*/
#define MAX_DATA				(MSG_CHANNEL_MAX-sizeof chnid_t) /*data包的最大大小*/

#define MSG_LIST_MAX 			(65536-20-8)
#define MAX_ENTRY 				(MSG_LIST_MAX -sizeof chnid_t)


/*两种包-----一种包负责往外发送数据*/

struct msg_channel_st{
	chnid_t chnid;	 /*0-255  256种可能*/   /*一定是在[MIBCHNID,MAXCHNID]*/
	uint8_t data[1];	/*多用变长数组*/

}__attribute__((packed));//以紧凑的方式对齐


/*    节目单
 *1 music :xxxxxxxxxxxxxxxxxxxxxx
 *2 sport :xxxxxxxxxxxxxxxxxxxxxx*/

struct msg_listentry_st{ //对应上述1 music :xxxxxxxxxxxxxxxxxxxxxx 类似一条 下面的list中就必须有一个此结构体数组   列表项
	chnid_t chnid;
	uint16_t len;//当前包的长度
	uint8_t desc[1];
}__attribute__((packed));

struct msg_list_st{
	chnid_t chnid;							/*一定是LISTCHIND*/
	struct msg_listentry_st entry[1];
}__attribute__((packed));

#endif
