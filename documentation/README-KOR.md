<p align="center"><img src="https://user-images.githubusercontent.com/16631264/90947085-177fac00-e46e-11ea-8ccc-3f14e214d39a.png"/></p>

<p align="center">
  <a href="https://www.codacy.com/gh/I-O-Benchmark-On-Container/ContainerTracer?utm_source=github.com&amp;utm_medium=referral&amp;utm_content=I-O-Benchmark-On-Container/ContainerTracer&amp;utm_campaign=Badge_Grade"><img src="https://app.codacy.com/project/badge/Grade/4994a1d576a54a9a9a7b2e0f0619e8f0"/></a>
  <a href="https://travis-ci.org/I-O-Benchmark-On-Container/ContainerTracer"><img src="https://travis-ci.org/I-O-Benchmark-On-Container/ContainerTracer.svg?branch=master"/></a>
</p>

# 소개

Container Tracer는 컨테이너 별 I/O 성능을 측정하는 도구로 기존의 fio나 filebench의
부족한 cgroup 별 I/O 성능 측정 기능을 보완하는 프로그램입니다.

Container Tracer는 장치의 컨테이너 별 I/O 성능을 측정하기 위해서 [trace-replay](https://github.com/yongseokoh/trace-replay)를
사용합니다.

![Execution Sample](https://user-images.githubusercontent.com/16631264/91653592-28b76100-eadd-11ea-9b66-5773bcfc5845.gif)

# 권장 시스템 조건

이 프로그램은 POSIX 표준을 따르는 시스템에서는 다 동작하게 개발되었습니다.

그러나 이 문서 및 테스트는 이하 표에서 제공하는 시스템에서 수행을 했습니다.

그러므로 반드시 빌드 또는 프로그램을 수행할 때에는 이 정보에 주의해야 합니다.

|   항목    |     내용     |
| :-------: | :----------: |
| 운영 체제 | Ubuntu 18.04 |
|   커널    |  linux 4.19  |

# 프로젝트의 구성

프로젝트의 구성은 아래와 같습니다.

- **trace-replay:** [trace-replay](https://github.com/yongseokoh/trace-replay)의 코드로
  기존의 비정형 데이터가 아닌 정형 데이터로 출력하게 변경되어 있습니다.
- **runner:** container를 생성한 후에 정형 데이터를 출력하는 trace-replay를 각각의 container에 동작시키는 프로그램입니다.
- **web:** 파이썬의 Flask로 개발되어 있으며, runner에서 받은 데이터를 웹으로 실시간 출력을 하는 역할을 합니다.
- **setting**: Scons 빌드의 전역 환경 설정이 들어있는 폴더입니다.
- **include**: C로 작성된 소스 코드의 헤더가 들어가는 위치입니다. 모든 헤더는 해당 위치에 있어야 합니다.

각 폴더의 하위 항목으로 있는 **test**에는 단위 테스트 파일이 들어가게 됩니다.

## 구현 내용

[링크](https://i-o-benchmark-on-container.github.io/ContainerTracerDoxygen/)를 확인해주시길
바랍니다.

# 빌드 방법

공통적으로는 아래를 수행해주셔야 합니다.

```bash
sudo pip3 install black flake8
sudo apt install doxygen
```

## web의 경우

Flask를 사용하기 때문에 반드시 아래를 수행해주셔야 합니다.

```bash
sudo pip3 install flask flask_restful flask_socketio pynput
```

그리고 프로젝트 루트 디렉터리에서 아래의 명령을 통해서
이 프로그램을 실행할 수 있습니다.

```bash
sudo python web/run.py
```

자세한 내용은 [링크](https://github.com/I-O-Benchmark-On-Container/ContainerTracer/wiki/8.-How-to-run-the-%60web%60-program)
를 확인해주시길 바랍니다.

## trace-replay나 runner의 경우

소스 코드를 다운 받은 후에 빌드에 필요한 프로그램을 설치하도록 합니다.

### Debian

```bash
sudo pip3 install scons scons-compiledb
sudo pip3 install clang-format
sudo apt install cppcheck libaio-dev libjson-c-dev libjemalloc-dev
```

### Redhat

```bash
sudo yum install cppcheck libaio-devel json-c-devel jemalloc-devel
```

추가로 본 프로젝트는 clang을 기반으로 하기 때문에 clang도 설치해주셔야 합니다.
만약에 \[1\]을 수행한 후에 `clang --version` 했을 때, 문제가 발생하지 않으면 \[2\] 과정은 무시해도 됩니다.

### Debian

```bash
sudo apt install llvm-6.0 clang-6.0 libclang-6.0-dev # [1]
sudo ln -s /usr/bin/clang-6.0 /usr/bin/clang # [2]
```

### Redhat

```bash
sudo yum install llvm6.0 clang7.0-devel clang7.0-libs
```

그리고 아래의 명령을 통해서 빌드 및 테스트를 수행합니다. 결과물은 `./build/debug`에 저장됩니다.

```bash
sudo scons -c
sudo scons test DEBUG=True TARGET_DEVICE=<당신의 장치>
```

배포판을 만드는 경우에는 아래와 같은 방법으로 진행해주시길 바랍니다. 결과물은 `./build/release`에 저장됩니다.

```bash
sudo scons -c
scons
```

그리고 빌드된 내용을 설치하기 위해서 release 모드의 경우에는 아래와 같이 진행하면 설치를 할 수 있습니다.
만약에 디버깅 모드로 설치하고 싶으신 경우에는 `sudo scons DEBUG=True install`로 해주시면 됩니다.

```bash
sudo scons install
sudo ldconfig
```
> 레드헷 환경에서는 반드시 아래 2가지를 유념하셔야 합니다.
>
> 첫번째, `/etc/ld.so.conf` 파일에 `/usr/local/lib` 줄이 있는지 확인해야 합니다.
> 만약 해당 줄이 없는 경우에는 해당 라인을 추가해줘야 합니다.
>
> 두번째, 만약 `jemalloc`과 관계된 에러가 확인되면 `jemalloc` 라이브러리를
> [링크](https://stackoverflow.com/questions/50839284/how-to-dlopen-jemalloc-dynamic-library)에 따라서 설치해야 합니다.

추가적으로 드라이버 제작 방법 관련해서는 [링크](https://github.com/I-O-Benchmark-On-Container/ContainerTracer/wiki/4.-How-to-add-the-driver-to-Runner)를
확인해주시길 바랍니다.

### 참고 사항

- `DEBUG=True`를 주게되면 모든 로그가 다 출력되어 좀 더 수월하게 디버깅이 가능해집니다.
- `sudo`를 주는 이유는 `trace-replay`가 실행되기 위해서는 `sudo` 권한을 필요로 하기 때문입니다.
- `sudo scons test`를 하면 `compile_commands.json`이 생성되지 않으므로 생성을 원하는
  경우 `scons`를 수행해주시길 바랍니다.

### 유의 사항

1. trace replay는 Direct-IO를 수행하므로 동작 과정에서 **사용자 디스크의 파일 시스템을 붕괴** 시킵니다.
   따라서, 파일 시스템이 구축되지 않은 디스크 및 백업이 간편한 가상 환경에서 수행해주시길 바랍니다
2. Runner의 init에 JSON을 보낼 때에는 반드시 `{"driver": "<DRIVER-NAME>", "setting": {...}}`과
   같은 형식으로 보내야 합니다. "setting"에는 드라이버에 종속적인 내용이 들어갑니다.

# 사용 라이브러리

1. pthread: trace-replay 동작용 POSIX 라이브러리
2. AIO: asynchronous I/O 라이브러리
3. rt: Real-Time 라이브러리
4. [json-c](https://github.com/json-c/json-c):  C용 json 라이브러리
5. [unity](https://github.com/ThrowTheSwitch/Unity): C용 unit test 라이브러리
6. [jemalloc](https://github.com/jemalloc/jemalloc): 효율적인 메모리 동적 할당을 해주는 라이브러리

# 코드 기여 규칙

기여 규칙은 [관련 링크](https://github.com/I-O-Benchmark-On-Container/ContainerTracer/blob/master/CONTRIBUTING.md)를 확인해주시길 바랍니다.

