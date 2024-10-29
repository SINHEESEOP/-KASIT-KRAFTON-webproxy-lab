//
// Created by gsg04 on 2024-10-29.
//

#include "csapp.h"

/*
세그먼트 오류가 발생하는 이유는 서버 코드의 echo 함수에서 Rio_readlineb 함수를 잘못 사용했기 때문입니다.
현재 Rio_readlineb 함수에 연결 파일 디스크립터(connfd)를 직접 전달하고 있지만, 이 함수는 rio_t 구조체에 대한 포인터를 필요로 합니다.
파일 디스크립터를 그대로 전달하면 예기치 않은 동작이 발생하여 세그먼트 오류가 발생하게 됩니다.

##다음은 서버 코드의 echo 함수에 문제가 있는 부분입니다.##
Rio_readlineb 함수는 csapp 라이브러리의 일부분으로, 강력한 I/O(ROBUST I/O) 버퍼 입력 시스템을 사용합니다.
이 함수는 호출마다 내부 상태를 유지하기 위해 rio_t 구조체에 대한 포인터를 필요로 합니다.
정수로 직접 전달하면 정의되지 않은 동작이 발생하여 세그먼트 오류를 유발합니다.

##해결 방법##
- echo 함수 내에 rio_t 구조체를 선언합니다.
- Rio_readinitb를 사용해 rio_t 구조체를 초기화합니다.
- Rio_readlineb에 rio_t 구조체의 주소를 전달합니다.
*/

// void echo(int connfd) {
//     size_t n;
//     char buf[MAXLINE];
//
//     while ((n = Rio_readlineb(connfd, buf, MAXLINE)) != 0) {
//         printf("server received %d bytes\n", (int)n);
//         Rio_writen(connfd, buf, n);
//     }
// }

/*
##설명##
- rio_t rio;: 이 부분은 버퍼링된 I/O 상태를 유지할 rio_t 구조체를 선언합니다.
- Rio_readinitb(&rio, connfd);: 연결 파일 디스크립터로 rio_t 구조체를 초기화합니다.
- Rio_readlineb(&rio, buf, MAXLINE): 버퍼링된 I/O 메커니즘을 사용하여 클라이언트로부터 한 줄을 읽습니다.

##포트 번호에 관한 추가 참고 사항##
현재 사용 중인 포트 번호 666566은 허용 가능한 최대 포트 번호인 65535를 초과하고 있습니다.
포트 번호는 16비트의 부호 없는 정수이므로 유효한 범위는 0에서 65535까지입니다.

유효한 포트 번호를 사용해 서버와 클라이언트를 실행해야 합니다. 예를 들어 12345 같은 포트를 사용할 수 있습니다:
- 서버 실행: ./echo_server 12345
- 클라이언트 실행: ./echo_client localhost 12345

##요약##
- echo 함수에서 rio_t 구조체를 사용하도록 수정하고 Rio_readinitb와 Rio_readlineb를 올바르게 초기화 및 사용합니다.
- 유효한 범위 내의 포트 번호를 사용합니다.

이 변경을 통해 서버에서 더 이상 세그먼트 오류가 발생하지 않고, 에코 기능이 예상대로 동작할 것입니다.
*/

void echo(int connfd) {
    size_t n;
    char buf[MAXLINE];
    rio_t rio;  // rio_t 구조체 선언

    Rio_readinitb(&rio, connfd);  // RIO 버퍼 초기화
    while ((n = Rio_readlineb(&rio, buf, MAXLINE)) != 0) {  // &rio 사용
        printf("server received %d bytes\n", (int)n);
        Rio_writen(connfd, buf, n);
    }
}


int main(int argc, char **argv) {
    int listenfd, connfd;
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;
    char client_hostname[MAXLINE], client_port[MAXLINE];

    if (argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(0);
    }

    listenfd = Open_listenfd(argv[1]);
    while (1) {
        clientlen = sizeof(struct sockaddr_storage);
        connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
        Getnameinfo((SA *) &clientaddr, clientlen, client_hostname, MAXLINE, client_port, MAXLINE, 0);
        printf("Connected to (%s, %s)\n", client_hostname, client_port);
        echo(connfd);
        Close(connfd);
    }
}


/*
void echo(int connfd) 함수 문제로, 연결 시 Segmentation fault 오류가 뜨던 이유.

문제의 원인: Rio_readlineb 함수 사용 방식
기존 코드에서 echo 함수는 단순히 connfd (파일 디스크립터)를 전달했습니다:
while ((n = Rio_readlineb(connfd, buf, MAXLINE)) != 0)

문제점: Rio_readlineb 함수는 csapp 라이브러리의 특수 함수로, connfd 정수형 파일 디스크립터가 아닌
rio_t 구조체를 필요로 합니다. rio_t 구조체는 버퍼된 I/O를 위한 구조체로, I/O 작업의 상태를 관리합니다.
이 구조체가 없으면 함수가 정상적으로 작동하지 않고, 세그먼트 오류가 발생할 수 있습니다.

Rio_readlineb 함수와 rio_t 구조체의 역할
Rio_readlineb 함수는 단순 파일 디스크립터로 데이터를 읽어오지 않고, 버퍼링된 I/O 방식을 사용합니다.
이 방식에서는 rio_t 구조체가 I/O 작업의 버퍼와 상태 정보를 유지해 주므로, 이를 함수에 전달하지 않으면
잘못된 메모리 접근이 발생해 세그먼트 오류가 발생합니다.

수정한 코드가 작동하는 이유
수정된 코드에서는 echo 함수 안에 rio_t 구조체를 선언하고, 이를 Rio_readinitb 함수로 초기화했습니다:

void echo(int connfd) {
    size_t n;
    char buf[MAXLINE];
    rio_t rio;  // rio_t 구조체 선언

    Rio_readinitb(&rio, connfd);  // RIO 버퍼 초기화
    while ((n = Rio_readlineb(&rio, buf, MAXLINE)) != 0) {  // &rio 사용
        printf("server received %d bytes\n", (int)n);
        Rio_writen(connfd, buf, n);
    }
}

작동하는 이유:
- rio_t rio;를 선언하여 I/O 버퍼와 상태를 관리할 수 있는 공간을 확보합니다.
- Rio_readinitb(&rio, connfd);를 통해 rio 구조체를 connfd 파일 디스크립터와 연결해 초기화합니다.
  이를 통해 rio 구조체는 connfd의 데이터를 버퍼에 읽어들이기 위한 준비가 완료됩니다.
- 이제 Rio_readlineb(&rio, buf, MAXLINE)로 rio 구조체를 통해 안정적으로 데이터를 읽을 수 있습니다.
  이 함수는 rio 구조체의 상태를 참고해 여러 줄의 데이터를 읽을 때 위치와 상태를 유지합니다.

요약:
Rio_readlineb 함수는 버퍼링된 I/O 시스템에서 작동하도록 설계되었으며, rio_t 구조체를 필요로 합니다.
이전 코드에서는 connfd를 직접 전달해 오류가 발생했습니다. 수정된 코드에서는 rio_t 구조체를 올바르게
선언하고 초기화하여 함수가 예상대로 동작하게 되었습니다.
*/