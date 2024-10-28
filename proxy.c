/* proxy.c */

#include "csapp.h"

/* 권장 최대 캐시 및 객체 크기 */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/* 이 긴 줄을 코드에 포함해도 스타일 점수를 잃지 않습니다 */
static const char *user_agent_hdr =
    "User-Agent: Mozilla/5.0 "
    "(X11; Linux x86_64; rv:10.0.3) "
    "Gecko/20120305 Firefox/10.0.3\r\n";

/* 함수 프로토타입 */
void *thread(void *vargp);
void doit(int connfd);
void parse_uri(char *uri, char *hostname, char *path, char *port);
void build_requesthdrs(char *http_header, char *hostname, char *path, rio_t *client_rio);

/* 메인 함수 */
int main(int argc, char **argv) {
    int listenfd, *connfdp;
    char port[6];
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;
    pthread_t tid;

    if (argc != 2) {
        fprintf(stderr, "사용법: %s <포트>\n", argv[0]);
        exit(1);
    }

    strcpy(port, argv[1]);
    listenfd = Open_listenfd(port);

    while (1) {
        clientlen = sizeof(clientaddr);
        connfdp = Malloc(sizeof(int));
        *connfdp = Accept(listenfd, (SA *)&clientaddr, &clientlen);

        /* 스레드 생성하여 요청 처리 */
        Pthread_create(&tid, NULL, thread, connfdp);
    }

    return 0;
}

/* 스레드 함수 */
void *thread(void *vargp) {
    int connfd = *((int *)vargp);
    Pthread_detach(pthread_self());
    Free(vargp);
    doit(connfd);
    Close(connfd);
    return NULL;
}

/* 클라이언트 요청 처리 함수 */
void doit(int connfd) {
    int serverfd;
    char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
    char hostname[MAXLINE], path[MAXLINE], port[6];
    rio_t client_rio, server_rio;
    char http_header[MAXLINE];

    /* 클라이언트로부터 요청 라인과 헤더를 읽음 */
    Rio_readinitb(&client_rio, connfd);
    if (!Rio_readlineb(&client_rio, buf, MAXLINE))
        return;

    sscanf(buf, "%s %s %s", method, uri, version);

    /* GET 메서드만 처리 */
    if (strcasecmp(method, "GET")) {
        fprintf(stderr, "501 Not Implemented: %s\n", method);
        return;
    }

    /* URI 파싱 */
    parse_uri(uri, hostname, path, port);

    /* 서버에 보낼 요청 헤더 생성 */
    build_requesthdrs(http_header, hostname, path, &client_rio);

    /* 원격 서버와의 연결 설정 */
    serverfd = Open_clientfd(hostname, port);
    if (serverfd < 0) {
        fprintf(stderr, "연결 실패: %s:%s\n", hostname, port);
        return;
    }

    /* 서버에 요청 전송 */
    Rio_readinitb(&server_rio, serverfd);
    Rio_writen(serverfd, http_header, strlen(http_header));

    /* 서버로부터 응답을 받아 클라이언트에게 전송 */
    size_t n;
    while ((n = Rio_readlineb(&server_rio, buf, MAXLINE)) != 0) {
        Rio_writen(connfd, buf, n);
    }

    Close(serverfd);
}

/* URI 파싱 함수 */
void parse_uri(char *uri, char *hostname, char *path, char *port) {
    char *hostbegin;
    char *hostend;
    char *pathbegin;
    int port_num;

    if (!strstr(uri, "http://")) {
        strcpy(path, uri);
        strcpy(port, "80");
        return;
    }

    /* http:// 부분 건너뜀 */
    uri += 7;
    hostbegin = uri;

    /* 포트 번호 파싱 */
    if ((hostend = strchr(hostbegin, ':')) != NULL) {
        *hostend = '\0';
        strcpy(hostname, hostbegin);
        port_num = atoi(hostend + 1);
        sprintf(port, "%d", port_num);
        pathbegin = strchr(hostend + 1, '/');
    } else {
        hostend = strchr(hostbegin, '/');
        if (hostend != NULL) {
            *hostend = '\0';
            strcpy(hostname, hostbegin);
            strcpy(port, "80");
            pathbegin = hostend;
        } else {
            strcpy(hostname, hostbegin);
            strcpy(port, "80");
            pathbegin = "/";
        }
    }

    strcpy(path, pathbegin);
}

/* 요청 헤더 빌드 함수 */
void build_requesthdrs(char *http_header, char *hostname, char *path, rio_t *client_rio) {
    char buf[MAXLINE], request_line[MAXLINE], host_hdr[MAXLINE], other_hdr[MAXLINE];

    /* 요청 라인 생성 */
    sprintf(request_line, "GET %s HTTP/1.0\r\n", path);

    /* 기본 Host 헤더 설정 */
    sprintf(host_hdr, "Host: %s\r\n", hostname);

    /* 클라이언트로부터 받은 헤더 읽기 */
    while (Rio_readlineb(client_rio, buf, MAXLINE) > 0) {
        if (strcmp(buf, "\r\n") == 0)
            break;

        if (!strncasecmp(buf, "Host:", 5)) {
            strcpy(host_hdr, buf);
            continue;
        }

        if (strncasecmp(buf, "User-Agent:", 11)
            && strncasecmp(buf, "Connection:", 11)
            && strncasecmp(buf, "Proxy-Connection:", 17)) {
            strcat(other_hdr, buf);
        }
    }

    /* 최종 요청 헤더 생성 */
    sprintf(http_header, "%s%s%s%s%s%s\r\n",
            request_line,
            host_hdr,
            user_agent_hdr,
            "Connection: close\r\n",
            "Proxy-Connection: close\r\n",
            other_hdr);
}
