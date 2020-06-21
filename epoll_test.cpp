#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <iostream>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#define MAXLINE		10
#define OPEN_MAX	100
#define LISTENQ		20
#define SERV_PORT	5555
#define INFTIM		1000
#define SOCKET int
#define INVALID_SOCKET (SOCKET)(-0)
#define SOCKET_ERROR           (-1)
void NON_BLOCK(int sock)
{
    int opts;
    opts = fcntl(sock, F_GETFL);

    if(opts < 0)
    {
        perror("fcntl(sock, GETFL)");
        exit(1);
    }

    opts = opts | O_NONBLOCK;

    if(fcntl(sock, F_SETFL, opts) < 0)
    {
        perror("fcntl(sock, SETFL, opts)");
        exit(1);
    }
}

int main()
{
    printf("epoll socket begins.\n");
    int i, maxi, listenfd, connfd, sockfd, epfd, nfds;

    sockaddr_in clientaddr;
    sockaddr_in serveraddr;
    listenfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (INVALID_SOCKET == listenfd)
    {
        std::cout << "Socket creation failed " << "socket = " << listenfd << std::endl;
        exit(1);
        //#ifdef _WIN32
        //		Sleep(3000);
        //#endif
    }
    else
    {
        std::cout << "Socket creation success " << "socket = " << listenfd << std::endl;
    }
    NON_BLOCK(listenfd);


    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = INADDR_ANY;
    serveraddr.sin_port = htons(SERV_PORT);
    if (SOCKET_ERROR == bind(listenfd, (sockaddr*)&serveraddr, sizeof(serveraddr)))
    {
        printf("Failed to bind  port <%d>\n", SERV_PORT);
        exit(1);
    }
    else
    {
        printf("Success to bind  port <%d>\n", SERV_PORT);
    }
    if (SOCKET_ERROR == listen(listenfd, LISTENQ))
    {
        printf("Failed to listen port sock = <%d>\n", listenfd);
        exit(1);
    }
    else
    {
        printf("Success to listen port sock = <%d>\n", listenfd);
    }

    ssize_t n;
    char line[MAXLINE];
    socklen_t clilen;
    epoll_event ev, events[20];
    epfd = epoll_create(256);
    ev.data.fd = listenfd;
    ev.events = EPOLLIN | EPOLLET;
    epoll_ctl(epfd, EPOLL_CTL_ADD, listenfd, &ev);
    maxi = 0;

    while(1)
    {
        nfds = epoll_wait(epfd, events, 20, 5);
        for(i = 0; i < nfds; ++i)
        {   printf("accept connection, fd is %d\n", listenfd);
            if(events[i].data.fd == listenfd)
            {
                printf("accept connection, fd is %d\n", listenfd);
                connfd = accept(listenfd, (struct sockaddr *)&clientaddr, &clilen);
                if(connfd < 0)
                {
                    perror("connfd < 0");
                    exit(1);
                }
                NON_BLOCK(connfd);
                char *str = inet_ntoa(clientaddr.sin_addr);
                printf("connect from %s\n", str);

                ev.data.fd = connfd;
                ev.events = EPOLLIN | EPOLLET;
                epoll_ctl(epfd, EPOLL_CTL_ADD, connfd, &ev);
            }
            else if(events[i].events & EPOLLIN)
            {
                if((sockfd = events[i].data.fd) < 0)
                    continue;
                if((n = read(sockfd, line, MAXLINE)) < 0)
                {
                    if(errno == ECONNRESET)
                    {
                        close(sockfd);
                        events[i].data.fd = -1;
                    }
                    else
                    {
                        printf("readline error");
                    }
                }
                else if(n == 0)
                {
                    close(sockfd);
                    events[i].data.fd = -1;
                }

                printf("received data: %s\n", line);
                ev.data.fd = sockfd;
                ev.events = EPOLLOUT | EPOLLET;
                epoll_ctl(epfd, EPOLL_CTL_MOD, sockfd, &ev);
            }
            else if(events[i].events & EPOLLOUT)
            {
                sockfd = events[i].data.fd;
                write(sockfd, line, n);
                printf("written data: %s\n", line);
                ev.data.fd = sockfd;
                ev.events = EPOLLIN | EPOLLET;
                epoll_ctl(epfd, EPOLL_CTL_MOD, sockfd, &ev);
            }
        }
    }
}
