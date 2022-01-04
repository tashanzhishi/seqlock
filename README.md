实现一种reader会超时退出的seqlock，防止writer被switch或者异常退出的时候，reader线程被饿死。
但是需要用户处理reader读失败。


x86是TSO内存模型，wmb/rmb相当于nop
https://www.zhihu.com/question/29465982
https://www.cnblogs.com/sunddenly/articles/15389917.html


NUMA也满足cache一致性，因此可以使用smp内存屏障
https://stackoverflow.com/questions/54652663/is-mov-mfence-safe-on-numa
