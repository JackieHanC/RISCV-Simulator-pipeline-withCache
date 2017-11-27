## Lab3.1 riscv simulator pipeline with cache

本文件夹包含：

```
Simulation.cpp
Simulation.h
Makefile
Read_Elf.cpp
Read_Elf.h
README.md
cache.cc
cache.h
memory.h
memory.cc
Storage.h
```
************************
本README文档主要叙述使用方法：
在安装make的linux机器上，解压压缩包，进入文件夹，执行

```
$ make
```

生成可执行文件`main`,该可执行程序有3个输入参数，样例执行如下

```
$ ./main qsort qsort_elf.txt -a 
```
其中`qsort`为riscv工具链编译的elf可执行文件，`qsort_elf.txt`为你想要输出elf信息到的文本文件,第3个参数为`-a`或`-p`,分别表示整体执行和单步执行模式，需要注意的是本程序有大量输出，所以一般使用

```
$ ./main qsort qsort_elf.txt -a > log
```
通过查看log文件来查找结果.   
在单步模式下，有若干指令：`p`单步执行,`c`终止执行,`reg`查看寄存器,`mem`输入并查看内存
