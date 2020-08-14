# Container Tracer

[![Codacy Badge](https://app.codacy.com/project/badge/Grade/4994a1d576a54a9a9a7b2e0f0619e8f0)](https://www.codacy.com/gh/I-O-Benchmark-On-Container/ContainerTracer?utm_source=github.com&amp;utm_medium=referral&amp;utm_content=I-O-Benchmark-On-Container/ContainerTracer&amp;utm_campaign=Badge_Grade)
[![Build Status](https://travis-ci.org/I-O-Benchmark-On-Container/ContainerTracer.svg?branch=master)](https://travis-ci.org/I-O-Benchmark-On-Container/ContainerTracer)

Container Tracer는 컨테이너 별 I/O 성능을 측정하는 도구로 기존의 fio나 filebench의
부족한 cgroup 별 I/O 성능 측정 기능을 보완하는 프로그램입니다.

Container Tracer는 장치의 컨테이너 별 I/O 성능을 측정하기 위해서 [trace-replay](https://github.com/yongseokoh/trace-replay)를
사용합니다.

## 프로젝트의 구성

프로젝트의 구성은 아래와 같습니다.

- **trace-replay:** [trace-replay](https://github.com/yongseokoh/trace-replay)의 코드로
  기존의 비정형 데이터가 아닌 정형 데이터로 출력하게 변경되어 있습니다.
- **runner:** container를 생성한 후에 정형 데이터를 출력하는 trace-replay를 각각의 container에 동작시키는 프로그램입니다.
- **web:** 파이썬의 Flask로 개발되어 있으며, runner에서 받은 데이터를 웹으로 실시간 출력을 하는 역할을 합니다.
- **setting**: Scons 빌드의 전역 환경 설정이 들어있는 폴더입니다.
- **include**: C로 작성된 소스 코드의 헤더가 들어가는 위치입니다. 모든 헤더는 해당 위치에 있어야 합니다.

각 폴더의 하위 항목으로 있는 **test**에는 단위 테스트 파일이 들어가게 됩니다.

### 구현 내용

[링크](https://i-o-benchmark-on-container.github.io/ContainerTracerDoxygen/)를 확인해주시길
바랍니다.

## 빌드 방법

공통적으로는 아래를 수행해주셔야 합니다.

```bash
sudo pip3 install black flake8
sudo apt install doxygen
```

### web의 경우

Flask를 사용하기 때문에 반드시 아래를 수행해주셔야 합니다.

```bash
sudo pip3 install flask flask_restful
```

### trace-replay나 runner의 경우

소스 코드를 다운 받은 후에 빌드에 필요한 프로그램을 설치하도록 합니다.

```bash
sudo pip3 install scons scons-compiledb
sudo pip3 install clang-format
sudo apt install cppcheck libaio-dev libjson-c-dev libjemalloc-dev
```

추가로 본 프로젝트는 clang을 기반으로 하기 때문에 clang도 설치해주셔야 합니다.
만약에 \[1\]을 수행한 후에 `clang --version` 했을 때, 문제가 발생하지 않으면 \[2\] 과정은 무시해도 됩니다.

```bash
sudo apt install llvm-6.0 clang-6.0 libclang-6.0-dev # [1]
sudo ln -s /usr/bin/clang-6.0 /usr/bin/clang # [2]
```

그리고 아래의 명령을 통해서 빌드 및 테스트를 수행합니다. 결과물은 `./build/debug`에 저장됩니다.

```bash
sudo scons -c
sudo scons test DEBUG=True
```

배포판을 만드는 경우에는 아래와 같은 방법으로 진행해주시길 바랍니다. 결과물은 `./build/release`에 저장됩니다.

```
sudo scons -c
scons
```

드라이버 추가 방법 관련해서는 [링크](https://github.com/I-O-Benchmark-On-Container/ContainerTracer.wiki.git)를
확인해주시길 바랍니다.

#### 참고 사항

- `DEBUG=True`를 주게되면 모든 로그가 다 출력되어 좀 더 수월하게 디버깅이 가능해집니다.
- `sudo`를 주는 이유는 `trace-replay`가 실행되기 위해서는 `sudo` 권한을 필요로 하기 때문입니다.
- `sudo scons test`를 하면 `compile_commands.json`이 생성되지 않으므로 생성을 원하는
  경우 `scons`를 수행해주시길 바랍니다.

#### 유의 사항

1. trace replay는 Direct-IO를 수행하므로 동작 과정에서 **사용자 디스크의 파일 시스템을 붕괴** 시킵니다.
   따라서, 파일 시스템이 구축되지 않은 디스크 및 백업이 간편한 가상 환경에서 수행해주시길 바랍니다
2. Runner의 init에 JSON을 보낼 때에는 반드시 `{"driver": "<DRIVER-NAME>", "setting": {...}}`과
   같은 형식으로 보내야 합니다. "setting"에는 드라이버에 종속적인 내용이 들어갑니다.

## 사용 라이브러리

1. pthread: trace-replay 동작용 POSIX 라이브러리
2. AIO: asynchronous I/O 라이브러리
3. rt: Real-Time 라이브러리
4. [json-c](https://github.com/json-c/json-c):  C용 json 라이브러리
5. [unity](https://github.com/ThrowTheSwitch/Unity): C용 unit test 라이브러리
6. [jemalloc](https://github.com/jemalloc/jemalloc): 효율적인 메모리 동적 할당을 해주는 라이브러리

## 코드 기여 규칙

trace-replay를 제외한 모든 코드는 [Doxygen](https://www.doxygen.nl/index.html)으로
문서화를 진행하셔야 합니다.

### web 코드 기여 규칙

가능하면 모듈에 대해서 테스트 파일을 만들어서 `web/test` 위치에 넣어주시길 바라며,
반드시 해당 테스트 파일은 **`pytest` 규격으로 작성**하셔서 테스트를 진행해주시길 바랍니다.

black 포맷팅을 진행한 후에 flake8 검사를 실시한 후에 문제가 없는 경우 PR을 진행합니다.
기본적으로, `pre-commit`으로 등록되어 있기 때문에 별도로 처리할 필요는 없습니다.
만약, 동작하지 않는 경우에는 아래의 조치를 수행해주시길 바랍니다.

1. `pip3 install pre-commit`을 수행해줍니다.
2. `pre-commit install`을 수행해주시길 바랍니다.

### trace-replay나 runner의 코드 기여 규칙

trace-replay나 runner의 경우에 반드시 아래 절차를 확인한 후에 PR을 진행해야 합니다.

1. `sudo scons test`를 반드시 수행한 후에 `build/log` 또는 출력에서 발생하는 모든 치명적인 오류는 반드시 수정되어야 합니다.
2. 프로젝트 최상위 폴더에서 `sudo valgrind --leak-check=full --show-leak-kinds=all ./build/(debug|release)/<PROGRAM>`를
   수행하여 메모리 누수를 확인하셔야 합니다.
3. 되도록 비교 구문 작성 시에는 `if (상수 == 변수)`의 형태로 작성해주시길 바랍니다.

[clang-format](https://clang.llvm.org/docs/ClangFormat.html)은 `pip install clang-format`으로
설치할 수 있고, [cppcheck](http://cppcheck.sourceforge.net/)의 경우에는
`sudo apt install cppcheck`를 통해 설치 가능합니다.
