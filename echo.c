//
// Created by gsg04 on 2024-10-27.
//

#include <stdio.h>
#include "csapp.h"

/* 권장 최대 캐시 및 객체 크기 */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/* 이 긴 줄을 코드에 포함해도 스타일 점수를 잃지 않습니다 */
static const char *user_agent_hdr =
    "User-Agent: Mozilla/5.0 "
    "(X11; Linux x86_64; rv:10.0.3) "
    "Gecko/20120305 Firefox/10.0.3\r\n";

int main(int argc, char **argv) {
    struct addrinfo *p, *listp, hints;
    char buf[MAXLINE];
    int rc, flags;

    // 입력 유효성 검사
    if (argc != 2) {
        fprintf(stderr, "usage: %s <domain name>\n", argv[0]);
        exit(0);
    }

    // addrinfo 리스트 가져오기
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;            // IPv4만
    hints.ai_socktype = SOCK_STREAM;      // 연결만
    if ((rc = getaddrinfo(argv[1], NULL, &hints, &listp)) != 0) {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(rc));
        exit(1);
    }

    // 각 IP 주소를 순회하여 출력
    flags = NI_NUMERICHOST;               // 숫자 주소 문자열로 출력
    for (p = listp; p; p = p->ai_next) {
        Getnameinfo(p->ai_addr, p->ai_addrlen, buf, MAXLINE, NULL, 0, flags);
        printf("%s\n", buf);
    }

    // 메모리 해제
    Freeaddrinfo(listp);
    exit(0);
}
