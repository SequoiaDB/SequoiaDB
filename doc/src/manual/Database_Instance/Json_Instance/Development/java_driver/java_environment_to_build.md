本文档主要介绍如何获取驱动开发包和配置 Eclipse 开发环境。

##环境需求##

Java 驱动环境需求如下：

 * JDK 版本：1.8+


##获取驱动开发包##

用户可以从 [SequoiaDB 巨杉数据库官网][download]下载对应操作系统版本的 SequoiaDB 驱动开发包。

##配置 Eclipse 开发环境##

1. 解压驱动开发包，从压缩包中的 `driver/java/` 目录获取 `sequoiadb.jar` 文件

2. 将 `sequoiadb.jar` 文件拷贝到工程文件目录下（建议将其放置在其他所有依赖库目录，如 `lib` 目录）

3. 在 Eclipse 界面中，创建并打开开发工程

4. 在 Eclipse 主窗口左侧的【Package Explore】窗口中，选择开发工程，并点击鼠标右键

5. 在菜单中选择【properties】菜单项

6. 在弹出的【property for project …】窗口中，选择【Java Build Path】->【Libraries】，如下图所示：

 ![property][eclipse]

7. 点击 **Add External JARs..** 按钮，选择添加 `sequoiadb.jar` 到工程

8. 点击 **OK** 完成环境配置


[^_^]:
     本文使用的所有引用和链接
[download]:http://download.sequoiadb.com/cn/index-cat_id-2
[eclipse]:images/Database_Instance/Json_Instance/Development/java_driver/eclipse.png