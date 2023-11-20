#include <stdio.h>
#include "csapp.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

void doit(int fd);
void read_requesthdrs(rio_t *rp);
int parse_uri(char *uri, char *host, char *port, char *path);
void serve_static(int fd, char *filename, int filesize, char *method);
void get_filetype(char *filename, char *filetype);
void serve_dynamic(int fd, char *filename, char *cgiargs, char *method);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg,
		char *longmsg);
void *thread(void *vargp);

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr =
    "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 "
    "Firefox/10.0.3\r\n";

/*
          fd           target_fd
client <------> proxy <---------> tiny
*/
int main(int argc, char **argv) {
	int listenfd, connfd;
	char hostname[MAXLINE], port[MAXLINE];
	socklen_t clientlen;
	struct sockaddr_storage clientaddr;
  pthread_t tid;
	/* Check command line args */
	if (argc != 2) {
		fprintf(stderr, "usage: %s <port>\n", argv[0]);
		exit(1);
	}

	listenfd = Open_listenfd(argv[1]);
	while (1) {
		clientlen = sizeof(clientaddr);
		connfd = Accept(listenfd, (SA *)&clientaddr,
				&clientlen);  // line:netp:tiny:accept
		Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE,
				0);
		printf("Accepted connection from (%s, %s)\n", hostname, port);
		// doit(connfd);   // line:netp:tiny:doit
    Pthread_create(&tid, NULL, thread, (void *)connfd);
		// Close(connfd);  // line:netp:tiny:close
	}
}

void doit(int fd){
	int target_serverfd; 
	ssize_t n;
	struct stat sbuf;
	char buf[MAXLINE];
	char buf_res[MAXLINE];
	char version[MAXLINE];
	//URI parse value
	char method[MAXLINE], uri[MAXLINE], host[MAXLINE], path[MAXLINE], port[MAXLINE];
	char filename[MAXLINE], cgiargs[MAXLINE];
	rio_t rio_req, rio_res;

	Rio_readinitb(&rio_res, fd);
	Rio_readlineb(&rio_res, buf, MAXLINE);
	printf("Request headers:\n");
	printf("%s", buf);
	sscanf(buf, "%s %s %s", method, uri, version);

	parse_uri(uri, host, port, path);
	printf("-----------------------------\n");	
	printf("\nClient Request Info : \n");
	printf("mothed : %s\n", method);
	printf("URI : %s\n", uri);
	printf("hostName : %s\n", host);
	printf("port : %s\n", port);
	printf("path : %s\n", path);
	printf("-----------------------------\n");

  // client가 요청한 페이지로 client file descriptor 생성
	target_serverfd = Open_clientfd(host, port);

  //요청을 target_fd로 보내고 응답을 받아옴
	request(target_serverfd, host, path);
  //응답을 보여줌
	response(target_serverfd, fd);
	Close(target_serverfd);
}

void request(int target_fd, char *host, char *path){
	char *version = "HTTP/1.0";
	char buf[MAXLINE];

  //header 내용을 buf에 작성
	sprintf(buf, "GET %s %s\r\n", path, version);
	/*Set header*/
	sprintf(buf, "%sHost: %s\r\n", buf, host);    
  sprintf(buf, "%s%s", buf, user_agent_hdr);
  sprintf(buf, "%sConnections: close\r\n", buf);
  sprintf(buf, "%sProxy-Connection: close\r\n\r\n", buf);

  //buf에 작성한 header 내용을 targer_fd를 통해 tiny에 전송해 응답을 받아옴
	Rio_writen(target_fd, buf, (size_t)strlen(buf));	//proxy 요청-> tiny 응답 -> proxy

}

void response(int target_fd, int fd){
	char buf[MAX_CACHE_SIZE];
	rio_t rio;
	int content_length;
	char *ptr;

	Rio_readinitb(&rio, target_fd); //&rio와 target_fd와 연결
  //header를 buf에 넣어줌
	while (strcmp(buf, "\r\n")){ //헤더 뒤에는 \r\n이 있으니 이걸 만날때 까지
    Rio_readlineb(&rio, buf, MAX_CACHE_SIZE); //rio(target_fd)를 buf에 작성
    if (strstr(buf, "Content-length"))  //content-length = 바디 길이
      content_length = atoi(strchr(buf, ':') + 1);
    Rio_writen(fd, buf, strlen(buf)); //header를 fd에 작성
  }

  //body를 buf에 넣어줌
	ptr = malloc(content_length); //바디 길이 만큼 malloc
	Rio_readnb(&rio, ptr, content_length); //바디 길이만큼을 가진 ptr에 rio 작성
	printf("server received %d bytes\n", content_length); 
	Rio_writen(fd, ptr, content_length); //body를 fd에 작성
}

void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg){
	char buf[MAXLINE], body[MAXBUF];

	sprintf(body, "<html><title>Tiny Error</title>");
	sprintf(body, "%s<body bgcolor=""ffffff"">\r\n", body);
	sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
	sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
	sprintf(body, "%s<hr><em>The Tiny Web server</em>\r\n", body);

	sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
	Rio_writen(fd, buf, strlen(buf));
	sprintf(buf, "Content-type: text/html\r\n");
	Rio_writen(fd, buf, strlen(buf));
	sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
	Rio_writen(fd, buf, strlen(buf));
	Rio_writen(fd, body, strlen(body));
}

void read_requesthdrs(rio_t *rp){
	char buf[MAXLINE];

	Rio_readlineb(rp, buf, MAXLINE);
	while(strcmp(buf, "\r\n")){
		Rio_readlineb(rp, buf, MAXLINE);
		printf("%s", buf);
	}
	return;
}

int parse_uri(char *uri, char *host, char *port, char *path){
	char *parse_ptr = strstr(uri, "//") ? strstr(uri, "//") + 2 : uri;

	//aws page ip:port/filename
	strcpy(host, parse_ptr);

	strcpy(path, "/"); //path = /

	parse_ptr = strchr(host, '/');
	if(parse_ptr){
		//path = /filename
		*parse_ptr = '\0';
		parse_ptr +=1;
		strcat(path, parse_ptr);
	}

	//aws page ip:port
	parse_ptr = strchr(host, ':');
	if(parse_ptr){
		//port = port
		*parse_ptr = '\0';
		parse_ptr +=1;
		strcpy(port, parse_ptr);
	} else {
		strcpy(port, "80");
	}
	return 0;
}

void *thread(void *vargp){
  int connfd = (int )vargp;
  //thread 분리하기 
  //메모리 누수를 막기 위해 각각의 연결 가능 thread는 다른 thread에 의해 
  //명시적으로 소거되거나 pthread_detach함수를 호출해 분리해야 한다
  Pthread_detach(pthread_self()); 
  // Free(vargp);
  doit(connfd);
  Close(connfd);
  return NULL;
}