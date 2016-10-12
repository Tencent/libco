##libco is a c/c++ coroutine library that widely used in WeChat services.

Author: sunnyxu(sunnyxu@tencent.com), leiffyli(leiffyli@tencent.com), dengoswei@gmail.com(dengoswei@tencent.com), sarlmolchen(sarlmolchen@tencent.com)

By linking with libco, you can easily transform synchronous back-end service into co-routine service, which provide out-standing concurrency compare to multi-thread approach. You get both happy coding in synchronous way and performance improve.

You can also use co_create/co_resume/co_yield function for asynchronous back-end service, which give you more control of when should I switch between co-routine.

libco has been used in WeChat since 2013, running on tens of thousands of machines. With libco, you can easily build a backend service suport tens of millions tcp connection.
***
##libco是微信后台大规模使用的c/c++协程库，2013年至今稳定运行在微信后台的数万台机器上。
作者: sunnyxu(sunnyxu@tencent.com), leiffyli(leiffyli@tencent.com), dengoswei@gmail.com(dengoswei@tencent.com), sarlmolchen(sarlmolchen@tencent.com)

libco通过仅有的几个函数接口 co_create/co_resume/co_yield 再配合 co_poll，可以支持同步或者异步的写法，如线程库一样轻松。同时库里面提供了socket族函数的hook，使得后台逻辑服务几乎不用修改逻辑代码就可以完成异步化改造。

同时提供协程变量机制辅助代码快速修改，并提供env族函数的hook以支持cgi轻松切换到协程模式，而新的共享栈模式（可选）轻松构建单机千万连接
