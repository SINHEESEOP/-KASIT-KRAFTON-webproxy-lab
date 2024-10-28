이제 Makefile의 명령어들을 하나씩 터미널에 입력하여 직접 빌드를 실행해보겠습니다. 각 명령어가 하는 역할을 설명드리겠습니다.

### 1. 컴파일 단계 (객체 파일 생성)

```bash
gcc -g -Wall -c csapp.c
```
- `csapp.c`를 컴파일하여 `csapp.o` 객체 파일을 생성합니다.
- `-c` 옵션은 컴파일만 수행하고 링크는 하지 않습니다.
- `-g`는 디버그 정보를 포함하고, `-Wall`은 모든 경고를 표시합니다.

```bash
gcc -g -Wall -c proxy.c
```
- `proxy.c` 파일을 컴파일하여 `proxy.o` 객체 파일을 생성합니다.

```bash
gcc -g -Wall -c echo.c
```
- `echo.c` 파일을 컴파일하여 `echo.o` 객체 파일을 생성합니다.

### 2. 링크 단계 (실행 파일 생성)

```bash
gcc -g -Wall proxy.o csapp.o -o proxy -lpthread
```
- `proxy.o`와 `csapp.o` 객체 파일을 링크하여 `proxy`라는 실행 파일을 생성합니다.
- `-lpthread` 옵션은 POSIX 스레드 라이브러리를 링크하는 옵션입니다.

```bash
gcc -g -Wall echo.o csapp.o -o echo -lpthread
```
- `echo.o`와 `csapp.o` 객체 파일을 링크하여 `echo`라는 실행 파일을 생성합니다.

### 3. 청소 단계 (임시 파일 삭제)

```bash
rm -f *~ *.o proxy echo core *.tar *.zip *.gzip *.bzip *.gz
```
- 이 명령어는 빌드 과정에서 생성된 임시 파일들을 삭제합니다.
    - `*~`: 일반적으로 백업 파일
    - `*.o`: 객체 파일
    - `proxy`, `echo`: 각각 `proxy`와 `echo` 실행 파일
    - `core`: 프로그램 충돌 시 생성되는 덤프 파일
    - `*.tar`, `*.zip`, `*.gzip`, `*.bzip`, `*.gz`: 압축 파일

### 4. 제출용 아카이브 생성

```bash
make clean; cd ..; tar cvf sinheeseop-proxylab-handin.tar proxylab-handout --exclude tiny --exclude nop-server.py --exclude proxy --exclude echo --exclude driver.sh --exclude port-for-user.pl --exclude free-port.sh --exclude ".*"
```
- `make clean`을 통해 불필요한 파일들을 삭제한 후,
- `cd ..`로 상위 디렉토리로 이동하여 `tar` 명령어로 아카이브를 생성합니다.
- `tar cvf sinheeseop-proxylab-handin.tar proxylab-handout`은 `proxylab-handin.tar`라는 이름의 아카이브 파일을 생성하는데, 특정 파일과 디렉토리는 `--exclude` 옵션으로 제외됩니다.

### 순서대로 실행하기

위 명령어들을 순서대로 실행하여 빌드 및 클린업 과정을 수행하면 됩니다.