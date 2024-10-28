/*
 * csapp.c - CS:APP3판 책을 위한 함수들
 *
 * 2016년 10월 업데이트 reb:
 *   - 음수를 처리하지 못하던 sio_ltoa의 버그 수정
 *
 * 2016년 2월 업데이트 droh:
 *   - open_clientfd와 open_listenfd를 더 우아하게 실패하도록 업데이트
 *
 * 2014년 8월 업데이트 droh:
 *   - open_clientfd와 open_listenfd의 새로운 버전은 재진입 가능하고
 *     프로토콜에 독립적입니다.
 *
 *   - 프로토콜에 독립적인 inet_ntop과 inet_pton 함수를 추가했습니다.
 *     inet_ntoa와 inet_aton 함수는 구식입니다.
 *
 * 2014년 7월 업데이트 droh:
 *   - 재진입 가능한 sio (signal-safe I/O) 루틴 추가
 *
 * 2013년 4월 업데이트 droh:
 *   - rio_readlineb: 경계 조건 버그 수정
 *   - rio_readnb: 중복된 EINTR 검사 제거
 */
/* $begin csapp.c */
#include "csapp.h"

/**************************
 * 오류 처리 함수들
 **************************/
/* $begin errorfuns */
/* $begin unixerror */
void unix_error(char *msg) /* 유닉스 스타일 오류 */
{
    fprintf(stderr, "%s: %s\n", msg, strerror(errno));
    exit(0);
}
/* $end unixerror */

void posix_error(int code, char *msg) /* POSIX 스타일 오류 */
{
    fprintf(stderr, "%s: %s\n", msg, strerror(code));
    exit(0);
}

void gai_error(int code, char *msg) /* Getaddrinfo 스타일 오류 */
{
    fprintf(stderr, "%s: %s\n", msg, gai_strerror(code));
    exit(0);
}

void app_error(char *msg) /* 애플리케이션 오류 */
{
    fprintf(stderr, "%s\n", msg);
    exit(0);
}
/* $end errorfuns */

void dns_error(char *msg) /* 구식 gethostbyname 오류 */
{
    fprintf(stderr, "%s\n", msg);
    exit(0);
}


/*********************************************
 * 유닉스 프로세스 제어 함수에 대한 래퍼
 ********************************************/

/* $begin forkwrapper */
pid_t Fork(void)
{
    pid_t pid;

    if ((pid = fork()) < 0)
	unix_error("Fork 오류");
    return pid;
}
/* $end forkwrapper */

void Execve(const char *filename, char *const argv[], char *const envp[])
{
    if (execve(filename, argv, envp) < 0)
	unix_error("Execve 오류");
}

/* $begin wait */
pid_t Wait(int *status)
{
    pid_t pid;

    if ((pid  = wait(status)) < 0)
	unix_error("Wait 오류");
    return pid;
}
/* $end wait */

pid_t Waitpid(pid_t pid, int *iptr, int options)
{
    pid_t retpid;

    if ((retpid  = waitpid(pid, iptr, options)) < 0)
	unix_error("Waitpid 오류");
    return(retpid);
}

/* $begin kill */
void Kill(pid_t pid, int signum)
{
    int rc;

    if ((rc = kill(pid, signum)) < 0)
	unix_error("Kill 오류");
}
/* $end kill */

void Pause()
{
    (void)pause();
    return;
}

unsigned int Sleep(unsigned int secs)
{
    unsigned int rc;

    if ((rc = sleep(secs)) < 0)
	unix_error("Sleep 오류");
    return rc;
}

unsigned int Alarm(unsigned int seconds) {
    return alarm(seconds);
}

void Setpgid(pid_t pid, pid_t pgid) {
    int rc;

    if ((rc = setpgid(pid, pgid)) < 0)
	unix_error("Setpgid 오류");
    return;
}

pid_t Getpgrp(void) {
    return getpgrp();
}

/************************************
 * 유닉스 시그널 함수에 대한 래퍼
 ***********************************/

/* $begin sigaction */
handler_t *Signal(int signum, handler_t *handler)
{
    struct sigaction action, old_action;

    action.sa_handler = handler;
    sigemptyset(&action.sa_mask); /* 처리 중인 시그널 유형의 시그널 블록 */
    action.sa_flags = SA_RESTART; /* 가능한 경우 시스템 호출 재시작 */

    if (sigaction(signum, &action, &old_action) < 0)
	unix_error("Signal 오류");
    return (old_action.sa_handler);
}
/* $end sigaction */

void Sigprocmask(int how, const sigset_t *set, sigset_t *oldset)
{
    if (sigprocmask(how, set, oldset) < 0)
	unix_error("Sigprocmask 오류");
    return;
}

void Sigemptyset(sigset_t *set)
{
    if (sigemptyset(set) < 0)
	unix_error("Sigemptyset 오류");
    return;
}

void Sigfillset(sigset_t *set)
{
    if (sigfillset(set) < 0)
	unix_error("Sigfillset 오류");
    return;
}

void Sigaddset(sigset_t *set, int signum)
{
    if (sigaddset(set, signum) < 0)
	unix_error("Sigaddset 오류");
    return;
}

void Sigdelset(sigset_t *set, int signum)
{
    if (sigdelset(set, signum) < 0)
	unix_error("Sigdelset 오류");
    return;
}

int Sigismember(const sigset_t *set, int signum)
{
    int rc;
    if ((rc = sigismember(set, signum)) < 0)
	unix_error("Sigismember 오류");
    return rc;
}

int Sigsuspend(const sigset_t *set)
{
    int rc = sigsuspend(set); /* 항상 -1 반환 */
    if (errno != EINTR)
        unix_error("Sigsuspend 오류");
    return rc;
}

/*************************************************************
 * Sio (Signal-safe I/O) 패키지 - 시그널 핸들러에서도 안전한 간단한 재진입 출력 함수들.
 *************************************************************/

/* 개인용 sio 함수들 */

/* $begin sioprivate */
/* sio_reverse - 문자열을 뒤집음 (K&R에서 가져옴) */
static void sio_reverse(char s[])
{
    int c, i, j;

    for (i = 0, j = strlen(s)-1; i < j; i++, j--) {
        c = s[i];
        s[i] = s[j];
        s[j] = c;
    }
}

/* sio_ltoa - long 타입을 진수 b의 문자열로 변환 (K&R에서 가져옴) */
static void sio_ltoa(long v, char s[], int b)
{
    int c, i = 0;
    int neg = v < 0;

    if (neg)
	v = -v;

    do {
        s[i++] = ((c = (v % b)) < 10)  ?  c + '0' : c - 10 + 'a';
    } while ((v /= b) > 0);

    if (neg)
	s[i++] = '-';

    s[i] = '\0';
    sio_reverse(s);
}

/* sio_strlen - 문자열의 길이 반환 (K&R에서 가져옴) */
static size_t sio_strlen(char s[])
{
    int i = 0;

    while (s[i] != '\0')
        ++i;
    return i;
}
/* $end sioprivate */

/* 공개된 Sio 함수들 */
/* $begin siopublic */

ssize_t sio_puts(char s[]) /* 문자열 출력 */
{
    return write(STDOUT_FILENO, s, sio_strlen(s)); //line:csapp:siostrlen
}

ssize_t sio_putl(long v) /* long 타입 출력 */
{
    char s[128];

    sio_ltoa(v, s, 10); /* K&R itoa() 기반 */  //line:csapp:sioltoa
    return sio_puts(s);
}

void sio_error(char s[]) /* 오류 메시지 출력 후 종료 */
{
    sio_puts(s);
    _exit(1);                                      //line:csapp:sioexit
}
/* $end siopublic */

/*******************************
 * SIO 루틴에 대한 래퍼들
 ******************************/
ssize_t Sio_putl(long v)
{
    ssize_t n;

    if ((n = sio_putl(v)) < 0)
	sio_error("Sio_putl 오류");
    return n;
}

ssize_t Sio_puts(char s[])
{
    ssize_t n;

    if ((n = sio_puts(s)) < 0)
	sio_error("Sio_puts 오류");
    return n;
}

void Sio_error(char s[])
{
    sio_error(s);
}

/********************************
 * 유닉스 I/O 루틴에 대한 래퍼
 ********************************/

int Open(const char *pathname, int flags, mode_t mode)
{
    int rc;

    if ((rc = open(pathname, flags, mode))  < 0)
	unix_error("Open 오류");
    return rc;
}

ssize_t Read(int fd, void *buf, size_t count)
{
    ssize_t rc;

    if ((rc = read(fd, buf, count)) < 0)
	unix_error("Read 오류");
    return rc;
}

ssize_t Write(int fd, const void *buf, size_t count)
{
    ssize_t rc;

    if ((rc = write(fd, buf, count)) < 0)
	unix_error("Write 오류");
    return rc;
}

off_t Lseek(int fildes, off_t offset, int whence)
{
    off_t rc;

    if ((rc = lseek(fildes, offset, whence)) < 0)
	unix_error("Lseek 오류");
    return rc;
}

void Close(int fd)
{
    int rc;

    if ((rc = close(fd)) < 0)
	unix_error("Close 오류");
}

int Select(int  n, fd_set *readfds, fd_set *writefds,
	   fd_set *exceptfds, struct timeval *timeout)
{
    int rc;

    if ((rc = select(n, readfds, writefds, exceptfds, timeout)) < 0)
	unix_error("Select 오류");
    return rc;
}

int Dup2(int fd1, int fd2)
{
    int rc;

    if ((rc = dup2(fd1, fd2)) < 0)
	unix_error("Dup2 오류");
    return rc;
}

void Stat(const char *filename, struct stat *buf)
{
    if (stat(filename, buf) < 0)
	unix_error("Stat 오류");
}

void Fstat(int fd, struct stat *buf)
{
    if (fstat(fd, buf) < 0)
	unix_error("Fstat 오류");
}

/*********************************
 * 디렉토리 함수에 대한 래퍼
 *********************************/

DIR *Opendir(const char *name)
{
    DIR *dirp = opendir(name);

    if (!dirp)
        unix_error("opendir 오류");
    return dirp;
}

struct dirent *Readdir(DIR *dirp)
{
    struct dirent *dep;

    errno = 0;
    dep = readdir(dirp);
    if ((dep == NULL) && (errno != 0))
        unix_error("readdir 오류");
    return dep;
}

int Closedir(DIR *dirp)
{
    int rc;

    if ((rc = closedir(dirp)) < 0)
        unix_error("closedir 오류");
    return rc;
}

/***************************************
 * 메모리 매핑 함수에 대한 래퍼들
 ***************************************/
void *Mmap(void *addr, size_t len, int prot, int flags, int fd, off_t offset)
{
    void *ptr;

    if ((ptr = mmap(addr, len, prot, flags, fd, offset)) == ((void *) -1))
	unix_error("mmap 오류");
    return(ptr);
}

void Munmap(void *start, size_t length)
{
    if (munmap(start, length) < 0)
	unix_error("munmap 오류");
}

/***************************************************
 * 동적 메모리 할당 함수에 대한 래퍼들
 ***************************************************/

void *Malloc(size_t size)
{
    void *p;

    if ((p  = malloc(size)) == NULL)
	unix_error("Malloc 오류");
    return p;
}

void *Realloc(void *ptr, size_t size)
{
    void *p;

    if ((p  = realloc(ptr, size)) == NULL)
	unix_error("Realloc 오류");
    return p;
}

void *Calloc(size_t nmemb, size_t size)
{
    void *p;

    if ((p = calloc(nmemb, size)) == NULL)
	unix_error("Calloc 오류");
    return p;
}

void Free(void *ptr)
{
    free(ptr);
}

/******************************************
 * 표준 I/O 함수에 대한 래퍼들.
 ******************************************/
void Fclose(FILE *fp)
{
    if (fclose(fp) != 0)
	unix_error("Fclose 오류");
}

FILE *Fdopen(int fd, const char *type)
{
    FILE *fp;

    if ((fp = fdopen(fd, type)) == NULL)
	unix_error("Fdopen 오류");

    return fp;
}

char *Fgets(char *ptr, int n, FILE *stream)
{
    char *rptr;

    if (((rptr = fgets(ptr, n, stream)) == NULL) && ferror(stream))
	app_error("Fgets 오류");

    return rptr;
}

FILE *Fopen(const char *filename, const char *mode)
{
    FILE *fp;

    if ((fp = fopen(filename, mode)) == NULL)
	unix_error("Fopen 오류");

    return fp;
}

void Fputs(const char *ptr, FILE *stream)
{
    if (fputs(ptr, stream) == EOF)
	unix_error("Fputs 오류");
}

size_t Fread(void *ptr, size_t size, size_t nmemb, FILE *stream)
{
    size_t n;

    if (((n = fread(ptr, size, nmemb, stream)) < nmemb) && ferror(stream))
	unix_error("Fread 오류");
    return n;
}

void Fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream)
{
    if (fwrite(ptr, size, nmemb, stream) < nmemb)
	unix_error("Fwrite 오류");
}


/****************************
 * 소켓 인터페이스 래퍼들
 ****************************/

int Socket(int domain, int type, int protocol)
{
    int rc;

    if ((rc = socket(domain, type, protocol)) < 0)
	unix_error("Socket 오류");
    return rc;
}

void Setsockopt(int s, int level, int optname, const void *optval, int optlen)
{
    int rc;

    if ((rc = setsockopt(s, level, optname, optval, optlen)) < 0)
	unix_error("Setsockopt 오류");
}

void Bind(int sockfd, struct sockaddr *my_addr, int addrlen)
{
    int rc;

    if ((rc = bind(sockfd, my_addr, addrlen)) < 0)
	unix_error("Bind 오류");
}

void Listen(int s, int backlog)
{
    int rc;

    if ((rc = listen(s,  backlog)) < 0)
	unix_error("Listen 오류");
}

int Accept(int s, struct sockaddr *addr, socklen_t *addrlen)
{
    int rc;

    if ((rc = accept(s, addr, addrlen)) < 0)
	unix_error("Accept 오류");
    return rc;
}

void Connect(int sockfd, struct sockaddr *serv_addr, int addrlen)
{
    int rc;

    if ((rc = connect(sockfd, serv_addr, addrlen)) < 0)
	unix_error("Connect 오류");
}

/*******************************
 * 프로토콜 독립적인 래퍼들
 *******************************/
/* $begin getaddrinfo */
void Getaddrinfo(const char *node, const char *service,
                 const struct addrinfo *hints, struct addrinfo **res)
{
    int rc;

    if ((rc = getaddrinfo(node, service, hints, res)) != 0)
        gai_error(rc, "Getaddrinfo 오류");
}
/* $end getaddrinfo */

void Getnameinfo(const struct sockaddr *sa, socklen_t salen, char *host,
                 size_t hostlen, char *serv, size_t servlen, int flags)
{
    int rc;

    if ((rc = getnameinfo(sa, salen, host, hostlen, serv,
                          servlen, flags)) != 0)
        gai_error(rc, "Getnameinfo 오류");
}

void Freeaddrinfo(struct addrinfo *res)
{
    freeaddrinfo(res);
}

void Inet_ntop(int af, const void *src, char *dst, socklen_t size)
{
    if (!inet_ntop(af, src, dst, size))
        unix_error("Inet_ntop 오류");
}

void Inet_pton(int af, const char *src, void *dst)
{
    int rc;

    rc = inet_pton(af, src, dst);
    if (rc == 0)
	app_error("inet_pton 오류: 잘못된 점-십진수 주소");
    else if (rc < 0)
        unix_error("Inet_pton 오류");
}

/*******************************************
 * DNS 인터페이스 래퍼들.
 *
 * 참고: 이들은 스레드 안전하지 않으므로 구식입니다.
 * 대신 getaddrinfo와 getnameinfo를 사용하세요.
 ***********************************/

/* $begin gethostbyname */
struct hostent *Gethostbyname(const char *name)
{
    struct hostent *p;

    if ((p = gethostbyname(name)) == NULL)
	dns_error("Gethostbyname 오류");
    return p;
}
/* $end gethostbyname */

struct hostent *Gethostbyaddr(const char *addr, int len, int type)
{
    struct hostent *p;

    if ((p = gethostbyaddr(addr, len, type)) == NULL)
	dns_error("Gethostbyaddr 오류");
    return p;
}

/************************************************
 * Pthreads 스레드 제어 함수에 대한 래퍼들
 ************************************************/

void Pthread_create(pthread_t *tidp, pthread_attr_t *attrp,
		    void * (*routine)(void *), void *argp)
{
    int rc;

    if ((rc = pthread_create(tidp, attrp, routine, argp)) != 0)
	posix_error(rc, "Pthread_create 오류");
}

void Pthread_cancel(pthread_t tid) {
    int rc;

    if ((rc = pthread_cancel(tid)) != 0)
	posix_error(rc, "Pthread_cancel 오류");
}

void Pthread_join(pthread_t tid, void **thread_return) {
    int rc;

    if ((rc = pthread_join(tid, thread_return)) != 0)
	posix_error(rc, "Pthread_join 오류");
}

/* $begin detach */
void Pthread_detach(pthread_t tid) {
    int rc;

    if ((rc = pthread_detach(tid)) != 0)
	posix_error(rc, "Pthread_detach 오류");
}
/* $end detach */

void Pthread_exit(void *retval) {
    pthread_exit(retval);
}

pthread_t Pthread_self(void) {
    return pthread_self();
}

void Pthread_once(pthread_once_t *once_control, void (*init_function)()) {
    pthread_once(once_control, init_function);
}

/*******************************
 * POSIX 세마포어에 대한 래퍼들
 *******************************/

void Sem_init(sem_t *sem, int pshared, unsigned int value)
{
    if (sem_init(sem, pshared, value) < 0)
	unix_error("Sem_init 오류");
}

void P(sem_t *sem)
{
    if (sem_wait(sem) < 0)
	unix_error("P 오류");
}

void V(sem_t *sem)
{
    if (sem_post(sem) < 0)
	unix_error("V 오류");
}

/****************************************
 * Rio 패키지 - 견고한 I/O 함수들
 ****************************************/

/*
 * rio_readn - 견고하게 n 바이트 읽기 (버퍼링되지 않음)
 */
/* $begin rio_readn */
ssize_t rio_readn(int fd, void *usrbuf, size_t n)
{
    size_t nleft = n;
    ssize_t nread;
    char *bufp = usrbuf;

    while (nleft > 0) {
	if ((nread = read(fd, bufp, nleft)) < 0) {
	    if (errno == EINTR) /* 시그널 핸들러에 의해 인터럽트됨 */
		nread = 0;      /* 그리고 다시 read() 호출 */
	    else
		return -1;      /* read()에 의해 errno 설정 */
	}
	else if (nread == 0)
	    break;              /* EOF */
	nleft -= nread;
	bufp += nread;
    }
    return (n - nleft);         /* 0 이상 반환 */
}
/* $end rio_readn */

/*
 * rio_writen - 견고하게 n 바이트 쓰기 (버퍼링되지 않음)
 */
/* $begin rio_writen */
ssize_t rio_writen(int fd, void *usrbuf, size_t n)
{
    size_t nleft = n;
    ssize_t nwritten;
    char *bufp = usrbuf;

    while (nleft > 0) {
	if ((nwritten = write(fd, bufp, nleft)) <= 0) {
	    if (errno == EINTR)  /* 시그널 핸들러에 의해 인터럽트됨 */
		nwritten = 0;    /* 그리고 다시 write() 호출 */
	    else
		return -1;       /* write()에 의해 errno 설정 */
	}
	nleft -= nwritten;
	bufp += nwritten;
    }
    return n;
}
/* $end rio_writen */


/*
 * rio_read - Unix read() 함수의 래퍼로, 내부 버퍼에서 사용자 버퍼로 min(n, rio_cnt) 바이트를 전송합니다.
 * n은 사용자가 요청한 바이트 수이고, rio_cnt는 내부 버퍼에 남은 바이트 수입니다.
 * 입력 시, 내부 버퍼가 비어 있으면 read() 호출을 통해 내부 버퍼를 다시 채웁니다.
 */
/* $begin rio_read */
static ssize_t rio_read(rio_t *rp, char *usrbuf, size_t n)
{
    int cnt;

    while (rp->rio_cnt <= 0) {  /* 버퍼가 비어 있으면 다시 채움 */
	rp->rio_cnt = read(rp->rio_fd, rp->rio_buf,
			   sizeof(rp->rio_buf));
	if (rp->rio_cnt < 0) {
	    if (errno != EINTR) /* 시그널 핸들러에 의해 인터럽트되지 않음 */
		return -1;
	}
	else if (rp->rio_cnt == 0)  /* EOF */
	    return 0;
	else
	    rp->rio_bufptr = rp->rio_buf; /* 버퍼 포인터 재설정 */
    }

    /* 내부 버퍼에서 사용자 버퍼로 min(n, rp->rio_cnt) 바이트 복사 */
    cnt = n;
    if (rp->rio_cnt < n)
	cnt = rp->rio_cnt;
    memcpy(usrbuf, rp->rio_bufptr, cnt);
    rp->rio_bufptr += cnt;
    rp->rio_cnt -= cnt;
    return cnt;
}
/* $end rio_read */

/*
 * rio_readinitb - 디스크립터를 읽기 버퍼와 연결하고 버퍼를 재설정
 */
/* $begin rio_readinitb */
void rio_readinitb(rio_t *rp, int fd)
{
    rp->rio_fd = fd;
    rp->rio_cnt = 0;
    rp->rio_bufptr = rp->rio_buf;
}
/* $end rio_readinitb */

/*
 * rio_readnb - 견고하게 n 바이트 읽기 (버퍼링됨)
 */
/* $begin rio_readnb */
ssize_t rio_readnb(rio_t *rp, void *usrbuf, size_t n)
{
    size_t nleft = n;
    ssize_t nread;
    char *bufp = usrbuf;

    while (nleft > 0) {
	if ((nread = rio_read(rp, bufp, nleft)) < 0)
            return -1;          /* read()에 의해 errno 설정 */
	else if (nread == 0)
	    break;              /* EOF */
	nleft -= nread;
	bufp += nread;
    }
    return (n - nleft);         /* 0 이상 반환 */
}
/* $end rio_readnb */

/*
 * rio_readlineb - 견고하게 텍스트 라인 읽기 (버퍼링됨)
 */
/* $begin rio_readlineb */
ssize_t rio_readlineb(rio_t *rp, void *usrbuf, size_t maxlen)
{
    int n, rc;
    char c, *bufp = usrbuf;

    for (n = 1; n < maxlen; n++) {
        if ((rc = rio_read(rp, &c, 1)) == 1) {
	    *bufp++ = c;
	    if (c == '\n') {
                n++;
     		break;
            }
	} else if (rc == 0) {
	    if (n == 1)
		return 0; /* EOF, 읽은 데이터 없음 */
	    else
		break;    /* EOF, 일부 데이터는 읽었음 */
	} else
	    return -1;	  /* 오류 */
    }
    *bufp = 0;
    return n-1;
}
/* $end rio_readlineb */

/**********************************
 * 견고한 I/O 루틴에 대한 래퍼들
 **********************************/
ssize_t Rio_readn(int fd, void *ptr, size_t nbytes)
{
    ssize_t n;

    if ((n = rio_readn(fd, ptr, nbytes)) < 0)
	unix_error("Rio_readn 오류");
    return n;
}

void Rio_writen(int fd, void *usrbuf, size_t n)
{
    if (rio_writen(fd, usrbuf, n) != n)
	unix_error("Rio_writen 오류");
}

void Rio_readinitb(rio_t *rp, int fd)
{
    rio_readinitb(rp, fd);
}

ssize_t Rio_readnb(rio_t *rp, void *usrbuf, size_t n)
{
    ssize_t rc;

    if ((rc = rio_readnb(rp, usrbuf, n)) < 0)
	unix_error("Rio_readnb 오류");
    return rc;
}

ssize_t Rio_readlineb(rio_t *rp, void *usrbuf, size_t maxlen)
{
    ssize_t rc;

    if ((rc = rio_readlineb(rp, usrbuf, maxlen)) < 0)
	unix_error("Rio_readlineb 오류");
    return rc;
}

/********************************
 * 클라이언트/서버 헬퍼 함수들
 ********************************/
/*
 * open_clientfd - <hostname, port>의 서버에 연결을 열고 읽기 및 쓰기가 가능한 소켓 디스크립터를 반환합니다.
 * 이 함수는 재진입 가능하고 프로토콜에 독립적입니다.
 *
 * 오류 시, 다음을 반환합니다:
 *   - getaddrinfo 오류의 경우 -2 반환
 *   - 다른 오류의 경우 errno가 설정된 상태로 -1 반환
 */
/* $begin open_clientfd */
int open_clientfd(char *hostname, char *port) {
    int clientfd, rc;
    struct addrinfo hints, *listp, *p;

    /* 가능한 서버 주소의 리스트를 얻음 */
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_socktype = SOCK_STREAM;  /* 연결 열기 */
    hints.ai_flags = AI_NUMERICSERV;  /* 숫자 포트 인자 사용 */
    hints.ai_flags |= AI_ADDRCONFIG;  /* 연결에 권장됨 */
    if ((rc = getaddrinfo(hostname, port, &hints, &listp)) != 0) {
        fprintf(stderr, "getaddrinfo 실패 (%s:%s): %s\n", hostname, port, gai_strerror(rc));
        return -2;
    }

    /* 성공적으로 연결할 수 있는 주소를 찾기 위해 리스트를 탐색 */
    for (p = listp; p; p = p->ai_next) {
        /* 소켓 디스크립터 생성 */
        if ((clientfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0)
            continue; /* 소켓 실패, 다음으로 시도 */

        /* 서버에 연결 */
        if (connect(clientfd, p->ai_addr, p->ai_addrlen) != -1)
            break; /* 성공 */
        if (close(clientfd) < 0) { /* 연결 실패, 다른 주소 시도 */  //line:netp:openclientfd:closefd
            fprintf(stderr, "open_clientfd: close 실패: %s\n", strerror(errno));
            return -1;
        }
    }

    /* 정리 */
    freeaddrinfo(listp);
    if (!p) /* 모든 연결 실패 */
        return -1;
    else    /* 마지막 연결 성공 */
        return clientfd;
}
/* $end open_clientfd */

/*
 * open_listenfd - 지정된 포트에서 수신 대기 소켓을 열고 반환합니다.
 * 이 함수는 재진입 가능하고 프로토콜에 독립적입니다.
 *
 * 오류 시, 다음을 반환합니다:
 *   - getaddrinfo 오류의 경우 -2 반환
 *   - 다른 오류의 경우 errno가 설정된 상태로 -1 반환
 */
/* $begin open_listenfd */
int open_listenfd(char *port)
{
    struct addrinfo hints, *listp, *p;
    int listenfd, rc, optval=1;

    /* 가능한 서버 주소의 리스트를 얻음 */
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_socktype = SOCK_STREAM;             /* 연결 수락 */
    hints.ai_flags = AI_PASSIVE | AI_ADDRCONFIG; /* 모든 IP 주소에서 수락 */
    hints.ai_flags |= AI_NUMERICSERV;            /* 포트 번호 사용 */
    if ((rc = getaddrinfo(NULL, port, &hints, &listp)) != 0) {
        fprintf(stderr, "getaddrinfo 실패 (포트 %s): %s\n", port, gai_strerror(rc));
        return -2;
    }

    /* 바인딩할 수 있는 주소를 찾기 위해 리스트를 탐색 */
    for (p = listp; p; p = p->ai_next) {
        /* 소켓 디스크립터 생성 */
        if ((listenfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0)
            continue;  /* 소켓 실패, 다음으로 시도 */

        /* bind의 "이미 사용 중인 주소" 오류 제거 */
        setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR,    //line:netp:csapp:setsockopt
                   (const void *)&optval , sizeof(int));

        /* 디스크립터를 주소에 바인딩 */
        if (bind(listenfd, p->ai_addr, p->ai_addrlen) == 0)
            break; /* 성공 */
        if (close(listenfd) < 0) { /* 바인딩 실패, 다음으로 시도 */
            fprintf(stderr, "open_listenfd close 실패: %s\n", strerror(errno));
            return -1;
        }
    }


    /* 정리 */
    freeaddrinfo(listp);
    if (!p) /* 사용할 수 있는 주소 없음 */
        return -1;

    /* 연결 요청을 수락할 준비가 된 수신 대기 소켓으로 만듦 */
    if (listen(listenfd, LISTENQ) < 0) {
        close(listenfd);
	return -1;
    }
    return listenfd;
}
/* $end open_listenfd */

/****************************************************
 * 재진입 가능하고 프로토콜 독립적인 헬퍼에 대한 래퍼들
 ****************************************************/
int Open_clientfd(char *hostname, char *port)
{
    int rc;

    if ((rc = open_clientfd(hostname, port)) < 0)
	unix_error("Open_clientfd 오류");
    return rc;
}

int Open_listenfd(char *port)
{
    int rc;

    if ((rc = open_listenfd(port)) < 0)
	unix_error("Open_listenfd 오류");
    return rc;
}

/* $end csapp.c */
