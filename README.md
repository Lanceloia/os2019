# OS2019：操作系统与系统编程课程实验

在南京大学操作系统课程框架上，用 C 实践**上下文切换、线程同步、内存管理、文件系统与系统工具**。
这是 Lanceloia 2019 年的课程代码，保留当时的实现和增量提交；2026 年补充中文阅读导览。

## 三分钟读代码

| 想看什么 | 建议入口 | 可以追问的实现问题 |
| --- | --- | --- |
| 协程与执行上下文 | [libco/co.c](os-workbench/libco/co.c) | 独立栈、`setjmp/longjmp`、让出与等待如何配合？ |
| 线程与同步 | [kernel/src/kthreads.c](os-workbench/kernel/src/kthreads.c) | 上下文保存、自旋锁、关中断、信号量等待队列如何组织？ |
| 内存分配 | [kernel/src/alloc.c](os-workbench/kernel/src/alloc.c) | 空闲块如何拆分、回收并维护链表约束？ |
| 文件系统 | [kernel/src/fs/](os-workbench/kernel/src/fs) | 路径查找、文件句柄、ext2/procfs及link/unlink如何接到统一接口？ |
| 持久化与恢复 | [libkvdb/kvdb.c](os-workbench/libkvdb/kvdb.c) | 数据块、备份、操作日志、互斥及文件锁各负责什么？ |
| 编译与动态加载 | [crepl/crepl.c](os-workbench/crepl/crepl.c) | 如何用子进程调用GCC，再加载函数求值？ |

继续阅读：[代码导览与面试讨论](docs/CODE_TOUR.md)。

## 仓库地图

```text
os-workbench/
  libco/             用户态协程库与原课程测试
  libkvdb/           持久化键值实验
  kernel/            教学内核：线程、同步、内存、设备与文件系统
  pstree/            进程树工具
  frecov/            文件恢复实验
  crepl/             C表达式／函数的编译与动态加载
  amgame/            Abstract Machine上的小游戏实验
  abstract-machine/  课程提供的抽象机、平台与基础支持
```

## 历史实现证据

| 阶段 | 保存的提交 |
| --- | --- |
| 协程实验 | [85d0ea7](https://github.com/Lanceloia/os2019/commit/85d0ea79408c98b00b9d9e411691a0ce4af4da41)，2019-03-24 |
| KV实验迭代 | [libkvdb文件历史](https://github.com/Lanceloia/os2019/commits/master/os-workbench/libkvdb/kvdb.c) |
| 文件系统增量 | [d40ef3a：ext2](https://github.com/Lanceloia/os2019/commit/d40ef3a)、[575e739：procfs](https://github.com/Lanceloia/os2019/commit/575e739)、[de7cbe3：link/unlink](https://github.com/Lanceloia/os2019/commit/de7cbe3) |

个人实验模块与课程平台分别位于上面的目录。`abstract-machine/`、框架和课程测试保留各自来源；
历史也包含课程工具自动记录的构建提交，阅读功能增量比统计提交数量更有帮助。

## 阅读与构建

本轮整理核对了代码、接口和历史提交，尚未在现代工具链复测整个教学系统。
不同实验的依赖不同：用户态工具使用Linux系统接口，协程含x86栈切换，内核依赖Abstract Machine。

开始构建前先读各模块的Makefile：原课程默认构建会调用`git`目标记录提交；
`submit`目标会联系旧课程平台。代码阅读无需执行这些目标。
现代复现建议从单个用户态模块开始，输出放在临时目录；先明确测试环境，再扩展至教学内核。
