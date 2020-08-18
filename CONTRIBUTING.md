# 코드 기여 규칙

trace-replay를 제외한 모든 코드는 [Doxygen](https://www.doxygen.nl/index.html)으로
문서화를 진행하셔야 합니다.

## web 코드 기여 규칙

가능하면 모듈에 대해서 테스트 파일을 만들어서 `web/test` 위치에 넣어주시길 바라며,
반드시 해당 테스트 파일은 **`pytest` 규격으로 작성**하셔서 테스트를 진행해주시길 바랍니다.

black 포맷팅을 진행한 후에 flake8 검사를 실시한 후에 문제가 없는 경우 PR을 진행합니다.
기본적으로, `pre-commit`으로 등록되어 있기 때문에 별도로 처리할 필요는 없습니다.
만약, 동작하지 않는 경우에는 아래의 조치를 수행해주시길 바랍니다.

1. `pip3 install pre-commit`을 수행해줍니다.
2. `pre-commit install`을 수행해주시길 바랍니다.

## trace-replay나 runner의 코드 기여 규칙

trace-replay나 runner의 경우에 반드시 아래 절차를 확인한 후에 PR을 진행해야 합니다.

1. `sudo scons test`를 반드시 수행한 후에 `build/log` 또는 출력에서 발생하는 모든 치명적인 오류는 반드시 수정되어야 합니다.
2. 프로젝트 최상위 폴더에서 `sudo valgrind --leak-check=full --show-leak-kinds=all ./build/(debug|release)/<PROGRAM>`를
   수행하여 메모리 누수를 확인하셔야 합니다.
3. 되도록 비교 구문 작성 시에는 `if (상수 == 변수)`의 형태로 작성해주시길 바랍니다.

[clang-format](https://clang.llvm.org/docs/ClangFormat.html)은 `pip install clang-format`으로
설치할 수 있고, [cppcheck](http://cppcheck.sourceforge.net/)의 경우에는
`sudo apt install cppcheck`를 통해 설치 가능합니다.
