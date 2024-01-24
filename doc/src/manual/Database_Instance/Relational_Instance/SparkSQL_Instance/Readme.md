[^_^]:
    SparkSQL 实例-概述

Spark 是一个可扩展的数据分析平台，该平台集成了原生的内存计算。相比 Hadoop 的集群存储，在使用上有较大的性能优势。

Apache Spark 提供了高级的 Java、Scala 和 Python APIs ，同时拥有优化的引擎来支持常用的执行图。Spark 还支持多样化的高级工具，其中包括处理结构化数据和 SQL 的 SparkSQL、处理机器学习的 MLlib 、图形处理的 GraphX 和 SparkStreaming。

##Spark 组成##

在集群中， Spark 应用以独立进程集合的方式运行，并由主程序（driver program）中的 SparkContext 对象进行统一调度。当需要在集群上运行时， SparkContext 会连接到几个不同类的 ClusterManager（集群管理器）上（ Spark 的 Standalone/Mesos/YARN ），集群管理器将给各个应用分配资源，连接成功后 Spark 会请求集群各个节点的 Executor（执行器）。然后，Spark 会将应用提供的代码（应用已经提交给 SparkContext 的 JAR 或 Python文件）交给 executor。最后，由 SparkContext 发送 tasks 提供给其执行。

![Spark组件图][spark_components]

关于这个架构的几点介绍：

- 每一个应用有其独立的 Executor 进程，这些进程将会在应用整个生命周期内为应用服务，并且会在多个线程中执行任务 tasks。这种做法能有效地隔离不同的应用，在调度和执行端都能很好地隔离（每个驱动调度自己的任务，不同的任务在不同的JVM中执行）。而如果不写入外部的存储设备，那数据就不能在不同的 Spark 应用（SparkContext 实例）之间共享。
    
- Spark 对于下列的集群管理者是不可知的：只要 Spark 能请求 executor 进程，且这些进程之间能互相通信，那能相对容易地去运行支持其他应用的集群管理器（如 Mesos/YARN）。
    
- 由于驱动在集群中负责调度任务，所以运行于 worker nodes（工作节点）附近，而 worker nodes 最好是在相同的局域网当中。如果用户不是从远程向集群发送请求，则需要为驱动打开一个 RPC 并让其在附近提交操作，而不是在远离 worker nodes 处运行驱动。

##Spark-SequoiaDB 连接组件##

通过使用 Spark-SequoiaDB 连接组件，SequoiaDB 巨杉数据库可以作为 Spark 的数据源，从而使用 SparkSQL 实例对 SequoiaDB 数据存储引擎的数据进行查询、统计操作。



[^_^]:
    本文使用到的所有链接及引用
[spark_components]:images/Database_Instance/Relational_Instance/SparkSQL_Instance/spark_components.jpg