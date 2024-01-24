本文档主要介绍如何获取驱动开发包和配置开发环境。

##获取驱动开发包##

用户可以从 [SequoiaDB 巨杉数据库官网][download]下载对应操作系统版本的 SequoiaDB 驱动开发包。

##配置开发环境##

* Linux

   1. 解压下载的驱动开发包；

   2. 将压缩包中的 `driver` 目录，拷贝到开发工程目录中（建议放在第三方库目录下），并命名为"sdbdriver"；

   3. 将 `sdbdriver/include` 目录加入到编译头目录，并将 `sdbdriver/lib` 目录加入连接目录，链接方式如下：

     **动态链接：**

     使用 `lib` 目录下的 `libsdbc.so` 动态库，gcc 编译参数形式如：

     ```lang-bash
     $ gcc testClient.c -o testClientC -I < PATH >/sdbdriver/include -L < PATH >/sdbdriver/lib -lsdbc
     ```

     PATH 为 sdbdriver 放置路径；运行程序时，用户需要将 LD_LIBRARY_PATH 路径指定为包含 `libsdbc.so` 动态库的路径

     ```lang-bash
     $ export LD_LIBRARY_PATH=< PATH >/sdbdriver/lib
     ```

     >**Note:**
     >
     >如果运行程序时会出现错误提示：
     >
     >```lang-bash
     >error while loading shared libraries: libsdbc.so: cannot open shared object file: No such file or directory
     >```
     >
     > 表示没有正确设置 LD_LIBRARY_PATH 和 LD_LIBRARY_PATH 环境变量，建议设置到 `/etc/profile` 或者应用程序的启动脚本中，避免每次新开终端都需要重新设置。
    
     **静态链接：**
    
     使用 `lib` 目录下的 `libstaticsdbc.a` 静态库，gcc 编译参数形式如：
    
     ```lang-bash
     $ gcc testClient.c -o testClientC -I < PATH >/sdbdriver/include -L < PATH >/sdbdriver/lib/ -lstaticsdbc -lm -lpthread -ldl 
     ```

* Windows

  暂未推出 Windows 驱动开发包



[^_^]:
    本文使用的所有引用和链接
[download]:http://download.sequoiadb.com/cn/driver
