/*thr_list模块与thr_channel模块都是从medialib模块取数据*/

#ifndef SERVER_CONF_H__
#define SERVER_CONF_H__

#define DEFAULT_MEDIADIR "/var/media"
#define DEFAULT_IF "eth0"

enum
{
    RUN_DAEMON = 1,  //守护进程 后台运行
    RUN_FOREGROUND   //前台运行
};

struct server_conf_st{
    char *rcvport;
    char *mgroup;
    char *media_dir;
    char *runmode;//前台运行还是后台运行
    char *ifname;
};

extern struct server_conf_st server_conf;//以供外部程序使用
extern int server_sd;
extern struct sockaddr_in sndaddr;

#endif