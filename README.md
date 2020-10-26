<p align="center"><img src="https://user-images.githubusercontent.com/16631264/90947085-177fac00-e46e-11ea-8ccc-3f14e214d39a.png"/></p>

<p align="center">
  <a href="https://www.codacy.com/gh/I-O-Benchmark-On-Container/ContainerTracer?utm_source=github.com&amp;utm_medium=referral&amp;utm_content=I-O-Benchmark-On-Container/ContainerTracer&amp;utm_campaign=Badge_Grade"><img src="https://app.codacy.com/project/badge/Grade/4994a1d576a54a9a9a7b2e0f0619e8f0"/></a>
  <a href="https://travis-ci.org/I-O-Benchmark-On-Container/ContainerTracer"><img src="https://travis-ci.org/I-O-Benchmark-On-Container/ContainerTracer.svg?branch=master"/></a>
</p>

<p align="center">
  <a href="https://github.com/I-O-Benchmark-On-Container/ContainerTracer/blob/master/documentation/README-KOR.md">한국어 링크</a>
</p>

# Introduction

Container Tracer is a tool that measures I/O performance per container.
This program complements lacks of cgroup and container building sequence
the existing I/O performance measurement program like fio and filebench.

Currently, Container Tracer uses [trace-replay](https://github.com/yongseokoh/trace-replay)
to measure the I/O performance of each container.

![Execution Sample](https://user-images.githubusercontent.com/16631264/91653592-28b76100-eadd-11ea-9b66-5773bcfc5845.gif)

# Recommended system requirements

This program can run on the system which follows the POSIX standards.

But in these documents and our testing was based on the system
which is described in the following table.

So, when you build and run this program you have to care about this information.

|      item       |   content    |
| :-------------: | :----------: |
| Operting System | Ubuntu 18.04 |
|     Kernel      |  linux 4.19  |

# Project Components

The composition of the project is as follows.

- **trace-replay:** [trace-replay](https://github.com/yongseokoh/trace-replay)
  which reconstructed to produce the formatted data to message queue or shared memory.
  (language: C)
- **runner:** Assign benchmark program to each container. (language: C)
- **web:** Developed based on Flask, which output the value from runner produces.
  (language: Python)
- **setting**: The directory which contains Scons' build global configuration
- **include**: The directory has C program's header.
  Almost all C program's header must be located in this place.

`runner` directory subdirectory `test` has unit-test program of each driver of runner.

## Implementation

You have to check [this link](https://i-o-benchmark-on-container.github.io/ContainerTracerDoxygen/)

# Building

In common, you must do the following.

```bash
sudo pip3 install black flake8
sudo apt install doxygen
```

## web

Since we are using Flask, you must install Flask and
install the related package first.

```bash
sudo pip3 install flask flask_restful flask_socketio pynput
```

And you can run this program with the following command
in the project root directory.

```bash
sudo python web/run.py
```

You can get more information from this [link](https://github.com/I-O-Benchmark-On-Container/ContainerTracer/wiki/8.-How-to-run-the-%60web%60-program).

## trace-replay and runner

You download the source code and install necessary programs
and libraries following.

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

If you want to build by `clang` compiler then you must change the
`SConstruct` file's `CC` section' `gcc` to `clang` and follow like below.

### Debian

```bash
sudo apt install llvm-6.0 clang-6.0 libclang-6.0-dev
sudo ln -s /usr/bin/clang-6.0 /usr/bin/clang
```

### Redhat

```bash
sudo yum install llvm6.0 clang7.0-devel clang7.0-libs
```

Based on the following commands you do the build and unit testing.
This execution results are stored in `./build/debug`.

```bash
sudo scons -c
sudo scons test DEBUG=True TARGET_DEVICE=<your device>
```

If you want to build the release mode then you do the following.
This execution results are stored in `./build/release`

```bash
sudo scons -c
scons
```

Moreover, you can install the release mode of runner libraries by following commands.
If you want to install the debug mode then just change the
`sudo scons install` to `sudo scons DEBUG=True install`

```bash
sudo scons install
sudo ldconfig
```

> Under the redhat distribution, you must care about two things.
>
> First, you have to check `/etc/ld.so.conf` file has the line `/usr/local/lib`.
> If not you add that line.
>
> Second, if you encounter an error that related to the `jemalloc`
> you must build the `jemalloc` library based on this [link](https://stackoverflow.com/questions/50839284/how-to-dlopen-jemalloc-dynamic-library).

Additionally, you can get information about
how to make the driver program of the runner from [this link](https://github.com/I-O-Benchmark-On-Container/ContainerTracer/wiki/4.-How-to-add-the-driver-to-Runner)

### Notes

- `DEBUG=True` enables us to print all information about debugging.
  This expects to be made you to easily debug our program.
- Because `trace-replay` needs the superuser privileges,
  you must run this program with superuser privileges.
- `sudo scons test` doesn't make a `compile_commands.json` file.
  So, if you want to generate that file then must only do `scons`.

### Warning

1. Because the `trace-replay` does direct-IO, this may be destructing
   your disk's file system. So, you must run this program under an
   empty disk or virtual disk.
2. You must follow the init JSON contents
   must be `{"driver": "<DRIVER-NAME>", "setting": {...}}`.
   And `setting` field has driver dependant contents.

# Libraries

1. pthread: POSIX thread library for trace-replay
2. AIO: asynchronous I/O library
3. rt: Real-Time library
4. [json-c](https://github.com/json-c/json-c):  Json library for C language
5. [unity](https://github.com/ThrowTheSwitch/Unity): Unit test library for C language
6. [jemalloc](https://github.com/jemalloc/jemalloc): A library which does the efficient memory dynamic allocation.

# Contributing

You must follow the contributing rules in [this link](https://github.com/I-O-Benchmark-On-Container/ContainerTracer/blob/master/CONTRIBUTING.md).
