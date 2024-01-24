[^_^]:
    卸载
    作者：陈思琴
    时间：20190320
    评审意见
    王涛：
    许建辉：
    市场部：20190530


如果用户不再需要 SequoiaDB 巨杉数据库，可以选择卸载。 

卸载前的检查
----

- 确保 SequoiaDB 未处于使用状态且不再使用
- 卸载过程需要使用 root 用户权限

数据备份
----

如果用户需要保留数据则自行对数据进行备份，如果不需要则彻底删除。

卸载步骤
----

以 SequoiaDB 安装在 `/opt/sequoiadb` 目录下为例，卸载步骤如下：

1. 以 root 身份登陆数据库服务器

2. 执行如下命令卸载 SequoiaDB

   ```lang-bash
   $ /opt/sequoiadb/uninstall
   ```

3. 回退下列系统配置参数

   - 删除系统配置文件 `/etc/security/limits.conf` 中的以下配置参数：

     ```lang-text
     <#domain>     <type>    <item>     <value>  
       *             soft      core       0
       *             soft      data       unlimited
       *             soft      fsize      unlimited
       *             soft      rss        unlimited
       *             soft      as         unlimited
     ```
   
   - 删除系统配置文件 `/etc/sysctl.conf` 中的以下配置参数：

     ```lang-ini
     vm.swappiness = 0
     vm.dirty_ratio = 100
     vm.dirty_background_ratio = 10
     vm.dirty_expire_centisecs = 50000
     vm.vfs_cache_pressure = 200
     vm.min_free_kbytes = <物理内存大小的8%，单位KB>
     ```

> **Note:**
>
> 集群环境需要在每台数据库服务器上执行卸载操作。

卸载检查
----

- 检查 SequoiaDB 相关进程已退出
- 检查 sdbcm 相关进程已退出
- 检查安装路径 `/opt/sequoiadb` 下相关文件已不存在

