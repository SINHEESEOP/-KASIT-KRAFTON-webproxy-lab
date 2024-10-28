####################################################################
# CS:APP Proxy Lab
#
# 학생 소스 파일
####################################################################

이 디렉토리에는 CS:APP Proxy Lab에 필요한 파일들이 포함되어 있습니다.

**proxy.c**
**csapp.h**
**csapp.c**
- 이 파일들은 시작 파일입니다. `csapp.c`와 `csapp.h`는 여러분의 교과서에 설명되어 있습니다.
- 여러분은 이 파일들에 원하는 대로 변경을 가할 수 있으며, 원하는 추가 파일을 생성하고 제출할 수 있습니다.
- 프록시나 tiny 서버를 위한 고유한 포트를 생성하려면 `port-for-user.pl` 또는 `free-port.sh`를 사용하세요.

**Makefile**
- 이 파일은 프록시 프로그램을 빌드하는 메이크파일입니다. "make" 명령어로 여러분의 솔루션을 빌드하거나, "make clean" 후 "make"로 새로 빌드할 수 있습니다.
- "make handin" 명령어로 제출할 tar 파일을 생성합니다. 자유롭게 수정할 수 있으며, 여러분의 교수는 이 Makefile을 사용하여 소스에서 프록시를 빌드할 것입니다.

**port-for-user.pl**
- 특정 사용자에게 랜덤 포트를 생성해줍니다.
- 사용법: `./port-for-user.pl <userID>`

**free-port.sh**
- 프록시나 tiny 서버에서 사용할 수 있는 미사용 TCP 포트를 찾아주는 스크립트입니다.
- 사용법: `./free-port.sh`

**driver.sh**
- Basic, Concurrency, 그리고 Cache에 대한 자동 채점 도구입니다.
- 사용법: `./driver.sh`

**nop-server.py**
- 자동 채점 도구의 헬퍼 스크립트입니다.

**tiny**
- CS:APP 교재에 나오는 Tiny 웹 서버입니다.

-----------------------------------------------------------------------------
####################################################################
# CS:APP Proxy Lab
#
# Student Source Files
####################################################################

This directory contains the files you will need for the CS:APP Proxy
Lab.

proxy.c
csapp.h
csapp.c
    These are starter files.  csapp.c and csapp.h are described in
    your textbook. 

    You may make any changes you like to these files.  And you may
    create and handin any additional files you like.

    Please use `port-for-user.pl' or 'free-port.sh' to generate
    unique ports for your proxy or tiny server. 

Makefile
    This is the makefile that builds the proxy program.  Type "make"
    to build your solution, or "make clean" followed by "make" for a
    fresh build. 

    Type "make handin" to create the tarfile that you will be handing
    in. You can modify it any way you like. Your instructor will use your
    Makefile to build your proxy from source.

port-for-user.pl
    Generates a random port for a particular user
    usage: ./port-for-user.pl <userID>

free-port.sh
    Handy script that identifies an unused TCP port that you can use
    for your proxy or tiny. 
    usage: ./free-port.sh

driver.sh
    The autograder for Basic, Concurrency, and Cache.        
    usage: ./driver.sh

nop-server.py
     helper for the autograder.         

tiny
    Tiny Web server from the CS:APP text

