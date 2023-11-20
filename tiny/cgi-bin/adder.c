/*
 * adder.c - a minimal CGI program that adds two numbers together
 */
// 두 인자를 더하고 클라이언트에게 결과와 html 파일 리턴
/* $begin adder */
#include "csapp.h"
// #include "../webproxy-lab/tiny/csapp.h"


int main(void)
{
  char *buf, *p, *method;
  char arg1[MAXLINE], arg2[MAXLINE], content[MAXLINE];
  int n1 = 0, n2 = 0;

  /* Extract the two arguments */
  if ((buf = getenv("QUERY_STRING")) != NULL)
  {
    p = strchr(buf, '&'); //buf에 &문자가 존재하는지 확인해 존재하면 해당 문자열 포인터 반환
    *p = '\0';
    strcpy(arg1, buf);
    strcpy(arg2, p + 1);
    n1 = atoi(strchr(arg1, '=') + 1); //atoi 문자 스트링을 정수값으로 변환
    n2 = atoi(strchr(arg2, '=') + 1);
  }
  method = getenv("REQUEST_METHOD");
  /* Make the response body */
  // content 인자에 html body 를 담는다.
  sprintf(content, "QUERY_STRING=%s", buf);
  sprintf(content, "Welcome to add.com: ");
  sprintf(content, "%sTHE Internet addition portal.\r\n<p>", content);
  sprintf(content, "%sThe answer is: %d + %d = %d\r\n<p>",
          content, n1, n2, n1 + n2);
  sprintf(content, "%sThanks for visiting!\r\n", content);

  /* Generate the HTTP response */
  printf("Connection: close\r\n");
  printf("Content-length: %d\r\n", (int)strlen(content));
  printf("Content-type: text/html\r\n\r\n");
  // 여기까지 가 헤더

  if(strcasecmp(method, "GET")==0){ //method가 get일때만 바디 출력
    // 여기부터 바디 출력
    printf("%s", content);
  }
  
  fflush(stdout);

  exit(0);
}
/* $end adder */