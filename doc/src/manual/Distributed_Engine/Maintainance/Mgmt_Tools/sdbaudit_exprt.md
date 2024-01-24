sdbaudit_ctl.py 是 SequoiaDB 巨杉数据库的审计日志管理工具，可以持续归档所监控集群和实例(MySQL/MariaDB)的审计日志，并整理成统一格式导入到指定的集合中。同时，生成日志导出进度的状态文件，保证工具重启后导出任务可以继续执行。

审计日志管理工具以工具套的形式位于 `<INSTALL_DIR>/tools/sdbaudit/` 目录下，目录结构如下：<br>

```lang-text
sdbaudit/
|__ sdbaudit_ctl.py                     管理工具脚本
|__ sdbaudit_exporter.py                日志导出脚本
|__ sdbaudit_daemon.py                  后台守护脚本
|__ conf                             
      |__ sdbaudit.conf                 全局配置文件
      |__ sdbaudit_log.conf             日志配置文件
      |__ sdb       
      |   |__ 11810                     
      |   |   |__ sdbaudit.conf         本地配置文件
      |   |   |__ sdbaudit.status       状态文件
      |   |   |__ sdbaudit.pid          进程 ID 文件
      |   |   |__ sdbaudit.log          日志文件
      |   |
      |   |__ 11820
      |   |__ ...
      |
      |__ mysql                         
          |__ 3306
      ...
```

其中 sdb 和 mysql 目录是用户在添加监控对象后生成的，里面包含了节点的配置信息、状态信息、日志信息等。

运行需求
----

用户需搭建 Python 开发环境，具体可参考 [Python 驱动开发][python_environment_to_build]章节。

参数说明
----

| 参数名      | 缩写  | 描述                                              | 是否必填 |
| ----------- | ----- | ------------------------------------------------- | -------- |
| --help      |  -h   | 返回帮助信息                                      |    否    |
| --version   |  -v   | 返回版本信息                                      |    否    |
| --type      |  -t   | 监控类型，可选参数为 sdb、mysql、mariadb          |    是    |
| --inst      |       | 实例名，监控类型为 mysql 或 mariadb 时可以指定    |    否    |
| --path      |       | 所监控对象的安装路径                              |    否    |

语法规则
----

* 添加监控对象
```lang-text
sdbaudit_ctl.py add -t <sdb|mysql|mariadb> [--inst INSTNAME] [--path INSTALL_DIR]
```

* 启动节点
```lang-text
sdbaudit_ctl.py start
```

* 查看节点状态
```lang-text
sdbaudit_ctl.py list
```

* 停止节点
```lang-text
sdbaudit_ctl.py stop
```

* 删除节点
```lang-text
sdbaudit_ctl.py del -t <sdb|mysql|mariadb> [--inst INSTNAME] [--path INSTALL_DIR]
```

 > **Note:**
 > 
 > * 建议用户通过 `sdbaudit_ctl.py` 工具启动和管理节点。<br>
 > * `sdbaudit_daemon.py` 以守护进程的形式存在。该服务在启动时，会自动启动相关的节点，在节点进程异常退出时，也会自动重启节点。

添加监控对象后，每个节点目录下都有自己的配置文件 `sdbaudit.conf`，用户可以根据需求灵活地配置每一个节点。配置读取遵循本地配置覆盖全局配置的规则，其具体参数有：

|参数名|类型|描述|是否自动生成|
|----  |----|----|----|
|auditpath|string|用于监控的审计日志文件目录|是|
|delete|boolean|是否删除完成导出的审计日志文件（只删除完整日志文件），默认值为 true|否|
|addr|string|指定主机地址（hostname:svcname），默认值为 localhost:11810|否|
|user|string|数据库用户名|否|
|password|string|指定数据库密码，指定值则使用明文输入，不指定值则命令行提示输入|否|
|password_type|string|指定数据库密码类型，0 代表密码为明文，1 代表密码为密文|否|
|ssl|boolean|指定是否使用 SSL 连接，默认 false，不使用 SSL 连接|否|
|clname|string|导出的集合全名|否|
|insertnum|int32|每批次插入的记录数|否|
|nodename|string|监控的节点名，添加时自动生成|是|
|role |string|监控的节点角色，添加时自动生成|是|
|instname|string|实例名，添加监控类型为 mysql 或 mariadb 时自动生成|是|

示例
----

1. 添加 SequoiaDB 各个节点审计日志目录下的文件进行导出

   ```lang-bash
   $ python sdbaudit_ctl.py add -t sdb
   ```
   
2. 添加 MySQL 各个节点审计日志目录下的文件进行导出

   ```lang-bash
   $ python sdbaudit_ctl.py add -t mysql
   ```

3. 启动所有节点

   ```lang-bash
   $ python sdbaudit_ctl.py start
   ```
   
4. 停止所有节点

   ```lang-bash
   $ python sdbaudit_ctl.py stop
   ```
   
5. 删除 SequoiaDB 所有节点

   ```lang-bash
   $ python sdbaudit_ctl.py del -t sdb
   ```
   
6. 删除 MySQL 所有节点
   
   ```lang-bash
   $ python sdbaudit_ctl.py del -t mysql
   ```



[^_^]:
    本文使用的所有引用和链接
[python_environment_to_build]:manual/Database_Instance/Json_Instance/Development/python_driver/python_environment_to_build.md
