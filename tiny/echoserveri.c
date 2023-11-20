#include "csapp.h"

void echo(int connfd)
{
    size_t n;
    char buf[MAXLINE];
    rio_t rio;
    rio_readinitb(&rio, connfd);
    while((n = rio_readlineb(&rio, buf, MAXLINE)) != 0) 
	{
        printf("server received %d bytes\n", (int)n);
        rio_writen(connfd, buf, n);
    }
}

int main(int argc, char **argv)
{
    int listenfd, connfd;
    socklen_t clientlen;
    struct sockaddr_storage clientaddr; //accept로 보내지는 소켓 주소 구조체
    char client_hostname[MAXLINE], client_port[MAXLINE];

    if (argc != 2) 
	{
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(0);
    }

    listenfd = open_listenfd(argv[1]);
    while (1) 
	{
        //클라이언트 연결요청을 기다려서
        clientlen = sizeof(struct sockaddr_storage);
        connfd = Accept(listenfd, (SA*)&clientaddr, &clientlen);
        Getnameinfo((SA *) &clientaddr, clientlen, client_hostname, MAXLINE, client_port, MAXLINE, 0);
        printf("Connected to (%s, %s)\n", client_hostname, client_port);
        //클라이언트를 서비스하는 echo 함수 호출
        echo(connfd);
        //자신들의 식별자를 닫고 연결 종료
        Close(connfd);
    }
    exit(0);
    
}