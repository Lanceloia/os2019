# OS2019代码导览

建议从一个问题走完一条路径，再看其他模块。下面的说明基于现有源码与历史，方便源码阅读与面试讨论。

## 1. 协程怎样交还控制权

入口：[co.c](../os-workbench/libco/co.c)、[co.h](../os-workbench/libco/co.h)、
[原课程测试](../os-workbench/libco/tests/main.c)。

```text
co_init → co_start → 独立栈上执行入口
                     co_yield → 保存上下文 → 选择可运行协程 → 恢复上下文
                     返回 → 标记结束
co_wait → 等待目标结束
```

代码使用`setjmp/longjmp`保存和恢复执行上下文，首次启动时通过x86内联汇编切换栈。
测试包含交替计数与生产者／消费者两类场景。
可讨论栈寿命、寄存器保存、栈对齐、可运行集合为空时的行为，以及协程与抢占线程的区别。
这些是阅读问题，原测试在当前工具链是否通过需实际复测。

## 2. 中断怎样触发调度和同步

入口：[os.c](../os-workbench/kernel/src/os.c)、[kthreads.c](../os-workbench/kernel/src/kthreads.c)。

`os_trap`遍历注册的事件处理器，调度器保存当前任务上下文并选择下一个可运行任务。
`kmt_spin_lock`结合原子交换与中断状态管理；信号量以计数、等待队列和任务状态组织阻塞／唤醒。

可讨论：为什么拿锁时要考虑中断？睡眠前怎样交还锁？任务在多个CPU上切换时有哪些竞态？
教学接口与源码存在，是本轮可以核对的事实；并发正确性需要独立的测试与审查。

## 3. 文件名怎样找到实际数据

入口：[vfs.c](../os-workbench/kernel/src/fs/vfs.c)、
[ext2fs.c](../os-workbench/kernel/src/fs/ext2fs.c)、
[procfs.c](../os-workbench/kernel/src/fs/procfs.c)。

VFS持有虚拟节点、文件系统与文件句柄；路径查找逐段解析目录，后端提供目录和数据操作。
ext2与procfs接入同一接口，link/unlink在历史中有独立增量。
可讨论：路径解析与后端读取的边界、节点身份和链接计数、文件关闭与删除如何影响状态。

## 4. KV实验怎样保存与重放操作

入口：[kvdb.c](../os-workbench/libkvdb/kvdb.c)、[kvdb.h](../os-workbench/libkvdb/kvdb.h)。

实现包含分块数据、索引、备份文件和操作日志；`open`选择新建、正常打开或日志恢复路径，
`put/get`使用pthread互斥，文件访问另使用`fcntl`锁。

**读接口时留意历史语义：** 此版`kvdb_get`读出值后调用`kvdb_free`删除条目，并写入`get`日志。
它具有取出语义，不能直接当作普通数据库的无副作用查询使用。
日志能重放的代码也不自动证明断电持久性或任意并发场景正确。
这些适合进一步讨论：接口命名、返回缓冲区寿命、锁范围、写入顺序和崩溃恢复边界。

## 5. C REPL怎样使用编译器

入口：[crepl.c](../os-workbench/crepl/crepl.c)。

实现接收函数或表达式，生成临时C文件，`fork/execve`调用GCC，使用管道检查编译信息，
再通过动态加载接口取得函数并求值。这条路径适合解释进程、文件描述符和动态链接。

## 归属与本轮验证

上述实验保留2019年课程实现；平台和接口来自课程框架。
2026年此次维护只增加README和导览，未修改C实现、原测试、Makefile或提交历史。
本轮证据是源码与历史核对，运行环境、测试结果和性能应分别通过实际命令建立。
