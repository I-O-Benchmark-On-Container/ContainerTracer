# Common Rules

You must comment the all source code except `trace-replay` based on `Doxygen` style.

## `web` Contributing Rules

Before you do the pull request, you must format all source codes by using [black](https://github.com/psf/black)
and check the source codes with [bandit](https://github.com/PyCQA/bandit), [PyLint](https://www.pylint.org/).

Basically, `pre-commit` does the automatic format and check.
However, this doesn't work correctly then do following.

```bash
pip3 install pre-commit
pre-commit install
```

## `trace-replay` and `runner` Contributing Rules

Before you do the pull request, you must follow the below sequences.

1. After executes the `sudo scons test`, you have to remove the all errors that appeared in the output.
2. You must check the memory leakage by using command
   `sudo valgrind --leak-check=full --show-leak-kinds=all ./build/(debug|release)/<PROGRAM>`
3. When you write down the comparison, follow the form of `if (constant == variable)`

You can install [clang-format](https://clang.llvm.org/docs/ClangFormat.html) by using `sudo pip3 install clang-format`.
And install [cppcheck](http://cppcheck.sourceforge.net/), [flawfinder](https://dwheeler.com/flawfinder/) from
`sudo apt install cppcheck`, `sudo pip3 install flawfinder`.
