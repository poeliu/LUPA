LUPA User Guide
====

LUPA is a lock usage pattern analysis tool for concurrency programs. the program source code is first fed into the compiler front-end to generate LLVM intermediate representation. Then the LUPA is applied to transform the original IR into the instrumented IR. After that, the code generation part of LLVM is used to generate assembly code of the instrumented program. The whole process can be fully automated by modifying the build scripts of the software.

### Supported OS

Currently, LUPA is only supported on Linux platforms. We recommend to use 64-bit Linux machines. We have tested LUPA on various Linux distributions, including the following.

* Redhat Enterprise Linux 5.4 (x86\_64)
* Ubuntu Desktop 10.10 (x86\_64)
* Ubuntu Desktop 11.04 (x86\_64)
* Ubuntu Desktop 12.04 (x86\_64)


