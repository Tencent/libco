//
// Created by dell-pc on 2018/4/5.
//

#include <sys/epoll.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <strings.h>
#include <asm/byteorder.h>
#include <cygwin/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <stdio.h>

#include <iostream>
#include <sys/unistd.h>
#include <stdlib.h>

#include <map>

using namespace std;

#define __LOG_PLACE  __FILE__<<":"<<__LINE__<<" at "<<__FUNCTION__

static int SetNonBlock(int iSock)
{
    int iFlags;

    iFlags = fcntl(iSock, F_GETFL, 0);
    iFlags |= O_NONBLOCK;
    iFlags |= O_NDELAY;
    int ret = fcntl(iSock, F_SETFL, iFlags);
    return ret;
}

static void SetAddr(const char *pszIP,const unsigned short shPort,struct sockaddr_in &addr)
{
    bzero(&addr,sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(shPort);
    int nIP = 0;
    if( !pszIP || '\0' == *pszIP
        || 0 == strcmp(pszIP,"0") || 0 == strcmp(pszIP,"0.0.0.0")
        || 0 == strcmp(pszIP,"*")
            )
    {
        nIP = htonl(INADDR_ANY);
    }
    else
    {
        nIP = inet_addr(pszIP);
    }
    addr.sin_addr.s_addr = (in_addr_t) nIP;

}

static int CreateTcpSocket(const unsigned short shPort /* = 0 */,const char *pszIP /* = "*" */,bool bReuse /* = false */)
{
    int fd = socket(AF_INET,SOCK_STREAM, IPPROTO_TCP);
    if( fd >= 0 )
    {
        if(shPort != 0)
        {
            if(bReuse)
            {
                int nReuseAddr = 1;
                setsockopt(fd,SOL_SOCKET,SO_REUSEADDR,&nReuseAddr,sizeof(nReuseAddr));
            }
            struct sockaddr_in addr ;
            SetAddr(pszIP,shPort,addr);
            int ret = bind(fd,(struct sockaddr*)&addr,sizeof(addr));
            if( ret != 0)
            {
                close(fd);
                return -1;
            }
        }
    }
    return fd;
}

int main()
{
    int port = 8900;
    char *ip = (char *) "0.0.0.0";
    int listen_fd = CreateTcpSocket((const unsigned short) port, ip, true );
    listen(listen_fd, 1024 );
    if(listen_fd == -1){
        printf("Port %d is in use\n", port);
        return -1;
    }
    printf("listen %d %s:%d\n", listen_fd, ip, port);

    SetNonBlock(listen_fd);

    int epollfd = epoll_create(0);
    if (epollfd < 0) {
        cout << __LOG_PLACE << " epoll_create err ret=" << epollfd << " err=" << strerror(errno) << endl;
        return epollfd;
    }

#define MAX_EVENTS 10
    struct epoll_event ev, events[MAX_EVENTS];

    ev.events = EPOLLIN;
    ev.data.fd = listen_fd;
    int ret = epoll_ctl(epollfd, EPOLL_CTL_ADD, listen_fd, &ev);
    if (ret < 0) {
        cout << __LOG_PLACE << " epoll_ctl err ret=" << ret << " err=" << strerror(errno) << endl;
    }

    map<int, string> fdBuffMap;

    while (true) {
        int nfds = epoll_wait(epollfd, events, MAX_EVENTS, 1000*10);
        if (nfds == -1) {
            perror("epoll_wait");
            exit(nfds);
        }

        for (int n = 0; n < nfds; ++n) {
            if (events[n].data.fd == listen_fd) {
                struct sockaddr addr;
                int addrlen = 0;
                int conn_sock = accept(listen_fd,
                                       (struct sockaddr *) &addr, &addrlen);
                if (conn_sock == -1) {
                    perror("accept");
                    exit(conn_sock);
                }

                cout << __LOG_PLACE << "new conn=" << conn_sock << endl;

                SetNonBlock(conn_sock);
                ev.events = EPOLLIN | EPOLLET;
                ev.data.fd = conn_sock;
                if (epoll_ctl(epollfd, EPOLL_CTL_ADD, conn_sock,
                              &ev) == -1) {
                    perror("epoll_ctl: conn_sock");
                    exit(-2);
                }
            } else {
                int fd = events[n].data.fd;
                int evv = events[n].events;
                if (evv & EPOLLIN) {
                    static char buff[1024*10];
                    int n = recv(fd, buff, 1024*10, 0);
                    if (n < 0) {
                        if (errno == EAGAIN) {
                            continue;
                        }
                        cout << __LOG_PLACE << " recv err ret=" << n << " err=" << strerror(errno) << endl;
                        epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, 0);
                        close(fd);
                        continue;
                    }
                    if (fdBuffMap.find(fd) == fdBuffMap.end()) {
                        fdBuffMap.insert(make_pair(fd, string(buff, n)));
                    } else {
                        fdBuffMap.find(fd)->second = string(buff, n);
                    }
                    ev.events = EPOLLOUT | EPOLLET;
                    ev.data.fd = fd;
                    epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &ev);
                    continue;
                }
                if (evv & EPOLLOUT) {
                    auto it = fdBuffMap.find(fd);
                    if (it == fdBuffMap.end()) {
                        char *s = (char *) "buff err";
                        send(fd, s, strlen(s), 0);
                    } else {
                        send(fd, it->second.c_str(), it->second.length(), 0);
                    }
                    ev.events = EPOLLIN | EPOLLET;
                    ev.data.fd = fd;
                    epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &ev);
                    continue;
                }
            }
        }
    }
}
