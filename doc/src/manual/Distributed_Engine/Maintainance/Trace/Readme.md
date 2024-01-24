
trace 是 SequoiaDB 巨杉数据库提供的流程跟踪工具。

- 开启 trace 后，会将当前操作流程涉及的函数间的调用关系组织成特定格式写入内存缓冲区中
- 关闭 trace 将 dump 内存缓冲区中的信息为二进制格式的跟踪文件，可以通过格式化工具转换二进制输出文件为可读形式

通常 SequoiaDB 巨杉数据库的支持团队和开发团队使用 trace 完成以下任务：
- 诊断重复出现且可重现的问题
- 获取有关正在调查的问题的信息

> **Note**：

> - 运行 trace 会增加开销，因此开启 trace 会影响系统性能
> - 使用 trace 分析问题需要熟悉源代码

通过学习本章，可以了解trace的基本概念和原理，熟悉trace的分析方法。本章内容如下：

+ [trace 的原理][intro]
+ [trace 的输出及格式化][dump_format]
+ [trace 的分析方法][analyze]


[^_^]:
    本文使用到的所有链接及引用。
[intro]:manual/Distributed_Engine/Maintainance/Trace/intro.md
[dump_format]:manual/Distributed_Engine/Maintainance/Trace/dump_format.md
[analyze]:manual/Distributed_Engine/Maintainance/Trace/analyze.md


