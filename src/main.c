
/*
 * Copyright (C) Zhu Jiashun
 * Copyright (C) Zaver
 */

#include <stdint.h>
#include <getopt.h>
#include <signal.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>

#include <zlog.h>

#include "util.h"
#include "timer.h"
#include "http.h"
#include "epoll.h"
#include "threadpool.h"
#include "memory_pool.h"

#define CONF "zaver.conf"
#define PROGRAM_VERSION "0.1"

extern struct epoll_event *events;

static const struct option long_options[]=
{
    {"help",no_argument,NULL,'?'},
    {"version",no_argument,NULL,'V'},
    {"conf",required_argument,NULL,'c'},
    {NULL,0,NULL,0}
};

/**
 * @brief 提示错误信息
 * 
 */
static void usage() {
   fprintf(stderr,
	"zaver [option]... \n"
	"  -c|--conf <config file>  Specify config file. Default ./zaver.conf.\n"
	"  -?|-h|--help             This information.\n"
	"  -V|--version             Display program version.\n"
	);
}

zlog_category_t *g_zc;

int main(int argc, char* argv[]) {
    int rc;
    int opt = 0;
    int options_index = 0;
    char *conf_file = CONF;

    // zlog 初始化
    int zlogrc;
	zlogrc = zlog_init("conf/zlog.conf");
	if (zlogrc) {
		printf("init failed\n");
		exit(-1);
	}

	g_zc = zlog_get_category("zlogdemo");
	if (!g_zc) {
		printf("get category fail\n");
		zlog_fini();
		exit(-2);
	}

    /*
    * parse argv 
    * more detail visit: http://www.gnu.org/software/libc/manual/html_node/Getopt.html
    */

    if (argc == 1) {
        usage();
        return 0;
    }

    while ((opt=getopt_long(argc, argv,"Vc:?h",long_options,&options_index)) != EOF) {
        switch (opt) {
            case  0 : break;
            case 'c':
                conf_file = optarg;
                break;
            case 'V':
                printf(PROGRAM_VERSION"\n");
                return 0;
            case ':':
            case 'h':
            case '?':
                usage();
                return 0;
        }
    }

    debug("conffile = %s", conf_file);

    debug("%d", sizeof(zv_http_request_t));

    if (optind < argc) {
        log_err("non-option ARGV-elements: ");
        while (optind < argc)
            log_err("%s ", argv[optind++]);
        return 0;
    }

    /*
    * read confile file
    */
    char conf_buf[BUFLEN];
    zv_conf_t cf;
    rc = read_conf(conf_file, &cf, conf_buf, BUFLEN);
    check(rc == ZV_CONF_OK, "read conf err");

    memorypool_create(64);
    /*
    *   install signal handle for SIGPIPE
    *   when a fd is closed by remote, writing to this fd will cause system send
    *   SIGPIPE to this process, which exit the program
    */
    struct sigaction sa;
    memset(&sa, '\0', sizeof(sa));
    sa.sa_handler = SIG_IGN;
    sa.sa_flags = 0;
    if (sigaction(SIGPIPE, &sa, NULL)) {
        log_err("install sigal handler for SIGPIPE failed");
        return 0;
    }

    /*
    * initialize listening socket
    */
    int listenfd;
    struct sockaddr_in clientaddr;
    // initialize clientaddr and inlen to solve "accept Invalid argument" bug
    socklen_t inlen = 1;
    memset(&clientaddr, 0, sizeof(struct sockaddr_in));  
    
    listenfd = open_listenfd(cf.port);
    rc = make_socket_non_blocking(listenfd);
    check(rc == 0, "make_socket_non_blocking");

    /*
    * create epoll and add listenfd to ep
    */
    int epfd = zv_epoll_create(0);
    struct epoll_event event;
    
    // 创建一个http请求对象
    zv_http_request_t *request = (zv_http_request_t *)malloc(sizeof(zv_http_request_t));
    zv_init_request_t(request, listenfd, epfd, &cf);

    // 将listen fd 设置为水平触发，监听可读事件
    event.data.ptr = (void *)request;
    event.events = EPOLLIN | EPOLLET;
    zv_epoll_add(epfd, listenfd, &event);

    /*
    * create thread pool
    */
    
    zv_threadpool_t *tp = threadpool_init(cf.thread_num);
    check(tp != NULL, "threadpool_init error");
    
    
    /*
     * initialize timer
     */
    zv_timer_init();

    log_info("zaver started.");
    int n;
    int i, fd;
    int time;

    /* epoll_wait loop */
    while (1) {
        time = zv_find_timer();
        zlog_debug(g_zc,"wait time = %d", time);
        n = zv_epoll_wait(epfd, events, MAXEVENTS, time);
        // 处理超时链接
        zv_handle_expire_timers();
        
        for (i = 0; i < n; i++) {
            zv_http_request_t *r = (zv_http_request_t *)events[i].data.ptr;
            fd = r->fd;
            
            if (listenfd == fd) {
                /* we hava one or more incoming connections */

                int infd;
                while(1) {
                    infd = accept(listenfd, (struct sockaddr *)&clientaddr, &inlen);
                    if (infd < 0) {
                        if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) {
                            /* we have processed all incoming connections */
                            break;
                        } else {
                            log_err("accept");
                            break;
                        }
                    }

                    rc = make_socket_non_blocking(infd);
                    check(rc == 0, "make_socket_non_blocking");
                    zlog_info(g_zc,"new connection fd %d", infd);
                    
                    // 内存池TODO
                    // zv_http_request_t *request = (zv_http_request_t *)malloc(sizeof(zv_http_request_t));
                    zv_http_request_t *request = (zv_http_request_t *)Allocate(MEMPOOL_HTTP_REQUESET_T);
                    if (request == NULL) {
                        log_err("malloc(sizeof(zv_http_request_t))");
                        break;
                    }

                    zv_init_request_t(request, infd, epfd, &cf);
                    printf("request ========= %p ", request);
                    printf("**** &(r->list) = %p\n",  &(request->list));
                    printf("(r->list).next %p\n", (request->list).next);
                    printf("(r->list).prev %p\n", (request->list).prev);
                    event.data.ptr = (void *)request;
                    event.events = EPOLLIN | EPOLLET | EPOLLONESHOT;

                    zv_epoll_add(epfd, infd, &event);
                    // zv_add_timer(request, TIMEOUT_DEFAULT, zv_http_close_conn);
                }   // end of while of accept

            } else {
                if ((events[i].events & EPOLLERR) ||
                    (events[i].events & EPOLLHUP) ||
                    (!(events[i].events & EPOLLIN))) {
                    log_err("epoll error fd: %d", r->fd);
                    close(fd);
                    continue;
                }

                zlog_info(g_zc,"new data from fd %d", fd);

                zv_http_request_t *request = (zv_http_request_t *)(events[i].data.ptr);

                // log_info("--- &(r->list) = %p\n----",  &(request->list));
                // printf("(r->list).next %p\n", (request->list).next);
                // printf("(r->list).prev %p\n", (request->list).prev);
                (request->list).next =  &(request->list);
                // printf("--- &(r->list) = %p\n----",  &(request->list));
                // printf("(r->list).next %p\n", (request->list).next);
                // printf("(r->list).prev %p\n", (request->list).prev);
                
                rc = threadpool_add(tp, do_request, events[i].data.ptr);
                check(rc == 0, "threadpool_add");

                do_request(events[i].data.ptr);
            }
        }   //end of for
    }   // end of while(1)
    

    /*
    if (threadpool_destroy(tp, 1) < 0) {
        log_err("destroy threadpool failed");
    }
    */

    return 0;
}
