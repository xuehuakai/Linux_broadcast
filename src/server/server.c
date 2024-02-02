#include <stdio.h>
#include <stdlib.h>
#include <proto.h>
#include <unistd.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <error.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#include "server_conf.h"
#include "medialib.h"
#include "thr_channel.h"
#include "thr_list.h"



/*
    -M 指定多播组
    -P 指定接收端口
    -F 前台运行
    -D 指定媒体库位置
    -I Interface name 指定网络设备 （网卡）
    -H 显示帮助
*/

struct server_conf_st server_conf = {.rcvport = DEFAULT_RCVPORT,
                                     .runmode=RUN_DAEMON,
                                     .ifname=DEFAULT_IF,
                                     .media_dir = DEFAULT_MEDIADIR,
                                     .mgroup = "224.2.2.2"
                                    };
int server_sd;  //全局变量可以定义在这个位置  但是它的声明应该放在相应的.h文件中
struct sockaddr_in sndaddr;//对端的地址
static struct mlib_listentry_st* list;
static  struct mlib_listentry_st *list;
static int socket_init(void){

    struct ip_mreqn mreq;
    server_sd=socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);
    if(server_sd<0)
        syslog(LOG_ERR, "socket():%s", strerror(errno));

    inet_pton(AF_INET, server_conf.mgroup, &mreq.imr_multiaddr);
    inet_pton(AF_INET, "0.0.0.0", &mreq.imr_address);
    mreq.imr_ifindex=if_nametoindex(server_conf.ifname);
    if (setsockopt(server_sd, IPPROTO_IP, IP_MULTICAST_IF, &mreq, sizeof mreq) < 0)
       { syslog(LOG_ERR, "setsockopt(IP_MULTICAST_IF):%s", strerror(errno));
        exit(1);
       }
    //bind();

    //发送端的信息配置
    sndaddr.sin_family = AF_INET;
    sndaddr.sin_port = htons(atoi(server_conf.rcvport));
    inet_pton(AF_INET,server_conf.mgroup,&sndaddr.sin_addr.s_addr);

    return 0;
}


static void daemon_exit(int s){ //当一个进程即将结束的时候要做的工作

	thr_list_destroy();
	//thr_list_destory();
	thr_channer_destoryadd();	
	mlib_freechnlist(list);

	syslog(LOG_WARNING,"signal-%d caught,exit now.",s);
    closelog();


    exit(0);
}


static int daemonize(void ){
    pid_t pid;
    int fd;
    pid=fork();
    if(pid<0){
       // fprintf(stderr, "fork()\n");
       syslog(LOG_ERR, "foek() 失败:", strerror(errno));
       return -1;
    }

    if(pid>0) //父进程
        exit(0);
    fd=open("/dev/null", O_RDWR);
    if(fd<0){
        //perror("open()");
        syslog(LOG_WARNING, "open() 失败:", strerror(errno));
        return -2;
    }
    else{
    //重定向
    dup2(fd, 0);
    dup2(fd, 1);//输出重定向到该文件
    dup2(fd, 2);
    if(fd>2)
        close(fd);
}
  

    //改变当前进程工作路径  已经脱离控制终端的守护进程放到一个合适的位置
    chdir("/");
    umask(0);//不会进行任何文件权限的屏蔽操作
      setsid();
      return 0;
}

static void printhelp(void ){
    printf("-M 指定多播组\n");
    printf("-P 指定接收端口\n");
    printf("-F 前台运行\n");
    printf("-D 指定媒体库位置\n");
    printf("-I Interface name 指定网络设备 （网卡）\n");
    printf("-H 显示帮助\n");
}

int main(int argc,char** argv){
    /*命令行分析*/

    struct sigaction sa;

    sa.sa_handler = daemon_exit;
    sigemptyset(&sa.sa_mask);
    sigaddset(&sa.sa_mask, SIGINT);
    sigaddset(&sa.sa_mask, SIGTERM);
    sigaddset(&sa.sa_mask, SIGQUIT);

    sigaction(SIGTERM,&sa,NULL);
    sigaction(SIGINT,&sa,NULL);
    sigaction(SIGQUIT,&sa,NULL);

    //日志文件
    openlog("netradio", LOG_PID | LOG_PERROR, LOG_DAEMON);//打开日志文件

    int c;
    while(1){
        c = getopt(argc, argv, "M:P:FD:I:H");
        if (c < 0)
            break;
            switch(c){
            case 'M':
            server_conf.mgroup = optarg;
            break;
            case 'P':
            server_conf.rcvport = optarg;
            break;
            case 'F':
         //   server_conf.runmode = RUN_FOREGROUND;
            break;
            case 'D':
            server_conf.media_dir = optarg;
            break;
            case 'I':
            server_conf.ifname = optarg;
            break;
            case 'H':
            printhelp();
            exit(0);
            break;
            default:
            abort();//call file  终止当前线程
            break;
            }
    }
    /*把当前进程作为守护进程*/
    if(server_conf.runmode==RUN_DAEMON)
       if(daemonize()!=0)
            exit(0);
    else if(server_conf.runmode==RUN_FOREGROUND){
        /*前台：do nothing*/
    }
    else{
       //fprintf(stderr, "Error :  EINVAL\n");
            syslog(LOG_ERR, "EINVAL server_conf.runmode.");
            exit(1);
    }
	
    /*SOCKKET初始化*/
    socket_init();
    /*获取频道信息*/ //从medialib中获取，medialib保存的是最全的数据结构   获取频道信息的目的是为了组织节目单往外发出   get_channel_list

    int list_size,err;
    err=mlib_getchnlist(&list,&list_size); //有几个频道，频道信息是什么
    if(err<0)
        syslog(LOG_ERR, "mlib_getchnlist() failed");

    /*创建节目单线程 */ //thread_list    假设100个频道，1个线程发送节目单，100个线程发送频道的数据，一共是101个线程
    thr_list_create(list, list_size);

    /*创建频道线程*/
    //害怕的不是延迟，而是延迟抖动

    int i;
    for (i = 0; i < list_size;++i){
        err=thr_channer_create(list+i);//频道与线程是一对一的关系
        if(err){
            fprintf(stderr, "thr_channer_create():%s", strerror(err));
			exit(1);
        }
    }


    syslog(LOG_DEBUG, "%d channel threads created.", i);

    while (1)
        pause(); // 让程序别结束

    //执行不到下面因为上面pause

    return 0;
}
