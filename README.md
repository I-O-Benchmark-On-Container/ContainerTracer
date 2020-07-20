# Container Tracer
Container Tracer는 각 **장치의 컨테이너 별 I/O 성능**을 측정하는 도구로 기존의 fio나 filebench의 부족한 cgroup 별 I/O 성능 측정 기능을 보완하는 프로그램입니다.

Container Tracer는 장치의 컨테이너 별 I/O 성능을 측정하기 위해서 [trace-replay](https://github.com/yongseokoh/trace-replay)를 사용합니다.

## 프로젝트의 구성

프로젝트의 구성은 아래와 같습니다.

- **trace-replay:** [trace-replay](https://github.com/yongseokoh/trace-replay)의 코드로 기존의 비정형 데이터가 아닌 정형 데이터로 출력하게 변경되어 있습니다.
- **runner:** container를 생성한 후에 정형 데이터를 출력하는 trace-replay를 각각의 container에 동작시키는 프로그램입니다.
- **web:** 파이썬의 Flask로 개발되어 있으며, runner에서 받은 데이터를 웹으로 실시간 출력을 하는 역할을 합니다.
- **setting**: Scons 빌드의 전역 환경 설정이 들어있는 폴더입니다.
- **include**: C로 작성된 소스 코드의 헤더가 들어가는 위치입니다. 모든 헤더는 해당 위치에 있어야 합니다.

각 폴더의 하위 항목으로 있는 **test**에는 단위 테스트 파일이 들어가게 됩니다.

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

소스 코드를 다운 받은 후에 아래를 그대로 수행하면 됩니다.

```bash
sudo pip3 install scons
sudo pip3 install clang-format
sudo apt install cppcheck libaio-dev
```

## 사용 라이브러리

1. pthread: trace-replay 동작용 POSIX 라이브러리
2. AIO: asynchronous I/O 라이브러리
3. rt: Real-Time 라이브러리
4. [json-c](https://github.com/json-c/json-c):  C용 json 라이브러리
5. [unity](https://github.com/ThrowTheSwitch/Unity): C용 unit test 라이브러리

## 코드 기여 규칙

trace-replay를 제외한 모든 코드는 [Doxygen](https://www.doxygen.nl/index.html)으로 문서화를 진행하셔야 합니다.

###  web 디렉터리의 경우

가능하면 모듈에 대해서 테스트 파일을 만들어서 `web/test` 위치에 넣어주시길 바라며, 반드시 해당 테스트 파일은 **`pytest` 규격으로 작성**하셔서 테스트를 진행해주시길 바랍니다.

black 포맷팅을 진행한 후에 flake8 검사를 실시한 후에 문제가 없는 경우 PR을 진행합니다. 기본적으로, `pre-commit`으로 등록되어 있기 때문에 별도로 처리할 필요는 없습니다. 만약, 동작하지 않는 경우에는 아래의 조치를 수행해주시길 바랍니다.

1. `pip3 install pre-commit`을 수행해줍니다.
2. `pre-commit install`을 수행해주시길 바랍니다.

### trace-replay나 runner의 경우

trace-replay나 runner의 경우에 반드시 아래 절차에 따라서 수행한 후에 PR을 진행해야 합니다.

1. `scons test`를 반드시 수행한 후에 `build/log` 또는 출력에서 발생하는 모든 내용은 오류를 발생 시켜서는 안됩니다. 단, 경고의 경우에는 PR을 올려도 되나 반드시 경고 내용을 함께 동봉해주시길 바랍니다.

[clang-format](https://clang.llvm.org/docs/ClangFormat.html)은 `pip install clang-format`으로 설치할 수 있고, [cppcheck](http://cppcheck.sourceforge.net/)의 경우에는 `sudo apt install cppcheck`를 통해 설치 가능합니다.