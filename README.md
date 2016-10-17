#Libco
Libco is a c/c++ coroutine library that is widely used in WeChat services. It has been running on tens of thousands of machines since 2013.*

Author: sunnyxu(sunnyxu@tencent.com), leiffyli(leiffyli@tencent.com), dengoswei@gmail.com(dengoswei@tencent.com), sarlmolchen(sarlmolchen@tencent.com)

By linking with libco, you can easily transform synchronous back-end service into coroutine service. The coroutine service will provide out-standing concurrency compare to multi-thread approach. With the system hook, You can easily coding in synchronous way but asynchronous executed.

You can also use co_create/co_resume/co_yield interfaces to create asynchronous back-end service. These interface will give you more control of coroutines.

By libco copy-stack mode, you can easily build a back-end service support tens of millions of tcp connection.
***
### 简介
libco是微信后台大规模使用的c/c++协程库，2013年至今稳定运行在微信后台的数万台机器上。  

libco通过仅有的几个函数接口 co_create/co_resume/co_yield 再配合 co_poll，可以支持同步或者异步的写法，如线程库一样轻松。同时库里面提供了socket族函数的hook，使得后台逻辑服务几乎不用修改逻辑代码就可以完成异步化改造。

作者: sunnyxu(sunnyxu@tencent.com), leiffyli(leiffyli@tencent.com), dengoswei@gmail.com(dengoswei@tencent.com), sarlmolchen(sarlmolchen@tencent.com)

### libco的特性
- 类pthread接口设计，通过**co_create、co_resume**等简单清晰接口即可完成协程的创建与恢复;
- 完善的协程环境编程接口：类 __thread的**协程私有变量**、协程间通信的**协程信号量co_signal**等;
- 支持**socket族函数hook**，第三方同步网络库一般不需任何修改即可完成异步化改造;
- 支持常用于cgi的**getenv/setenv**方法hook，现网cig程序也可轻松完成协程化改造;
- 支持glibc的**gethostbyname/gethostbyname_r接口hook**，简单安全的异步查询dns;
- 支持自定义的**stackless的协程共享栈**，轻松创建千万级协程;
- **非语言级别的lambda实现**，结合协程原地编写并执行后台异步任务;
- 基于epoll/kqueue实现的小而轻的网络框架，基于时间轮盘实现的高性能定时器;


