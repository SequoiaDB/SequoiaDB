[^_^]:
    文件系统实例概述


SequoiaDB 除了支持创建关系型数据库实例和 JSON 实例外，同样支持创建 S3 对象存储实例。

S3 对象存储实例适用于对象存储类的联机业务与归档类场景，SequoiaDB 与 S3 保持 100% 兼容。SequoiaS3 系统实现通过 AWS S3 接口访问 SequoiaDB 的能力。

通过本章文档，用户可以了解 S3 对象存储实例的操作与开发。主要内容如下：

**操作**

 - [安装与配置][s3_setup]
 - [Rest 接口][restapi]
 - [Java 接口][javaapi]

**开发**

 - [下载 S3 开发工具包][s3_engine_download]
 - [Java 程序样例][s3_java_sample]
 - [C++ 程序样例][C++_sample]
 - [通过 s3cmd 进行连接][s3_connection]

[^_^]:
     本文使用的所有链接及引用
[s3_setup]:manual/Database_Instance/Object_Instance/S3_Instance/Operation/setup.md
[restapi]:manual/Database_Instance/Object_Instance/S3_Instance/Operation/restapi.md
[javaapi]:manual/Database_Instance/Object_Instance/S3_Instance/Operation/javaapi.md
[s3_engine_download]:manual/Database_Instance/Object_Instance/S3_Instance/Development/engine_download.md
[s3_connection]:manual/Database_Instance/Object_Instance/S3_Instance/Development/connection.md
[s3_java_sample]:manual/Database_Instance/Object_Instance/S3_Instance/Development/java_sample.md
[C++_sample]:manual/Database_Instance/Object_Instance/S3_Instance/Development/C++_sample.md
