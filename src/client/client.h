#ifndef CLIENT_H__
#define CLIENT_H__
#include"../include/proto.h"

#define DEFAULT_PLAYERCMD "/usr/bin/mpg123 - /dev/null" //使用 /usr/bin/mpg123 命令来播放音频文件，并将输出重定向到 /dev/null，这意味着播放器的输出将被丢弃，不会显示在终端上。
/*
   struct client_conf_st{ //指定用户可以来指定的标识
   char* rcvport;
   char* mgroup;
   char* player_cmd;//命令行的传输
   };
   */

struct client_conf_st {
		char* rcvport;
		char* mgroup;
		char* player_cmd;
};


/*在这里也声明下，防止别的.c文件使用的时候找不到*/

//extern struct client_conf_st client_conf; //extern 扩展了当前全局变量的作用域


#endif
