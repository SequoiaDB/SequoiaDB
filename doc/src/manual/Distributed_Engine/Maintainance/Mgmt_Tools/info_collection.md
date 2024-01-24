[^_^]:
    数据库信息收集工具
    作者：赵玉静
    时间：20190329
    评审意见
    王涛：
    许建辉：
    市场部：20190522


sdbsupport 是 SequoiaDB 巨杉数据库用于收集数据库相关信息的工具。该工具收集的信息包括：数据库配置信息、数据库日志信息、数据库快照信息、数据库所在主机的硬件信息和操作系统信息。

运行需求
----

该工具位于安装目录 `/opt/sequoiadb/tools` 下，是一个可执行脚本，执行该工具需要数据库用户（sdbadmin）权限。

语法规则
----

```lang-text
./sdbsupport.sh
./sdbsupport.sh [--all]
./sdbsupport.sh [--hostname | -s arg] [--svcname | -p arg] [--snapshot | -n] [--log]
```

参数说明
----

| 参数              | 缩写 | 描述 |
| ----              | ---- | ---- |
| --help            | -H   | 帮助选项|
| --hostname        | -s   | 指定要收集信息的主机名称，当不指定主机名称也没有指定 --all 参数时，收集本机的信息 |
| --svcname         | -p   | 指定特定端口收集其配置、日志及快照信息 |
| --user            | -u   | 指定数据库用户名     |
| --password        | -w   | 指定数据库用户密码   |
| --snapshot        | -n   | 指定收集快照信息     |
| --osinfo          | -o   | 指定收集操作系统信息 |
| --hardware        | -h   | 指定收集硬件信息     |
| --all             |      | 指定收集数据库所有集群的所有信息 |
| --conf            |      | 指定收集数据库节点的配置文件信息 |
| --log             |      | 指定收集数据库节点的日志文件信息 |
| --cm              |      | 指定收集 CM 配置和日志信息 |
| --cpu             |      | 指定收集 CPU 信息  |
| --memory          |      | 指定收集内存信息 |
| --disk            |      | 指定收集硬盘信息 |
| --netcard         |      | 指定收集网卡信息 |
| --mainboard       |      | 指定收集主板信息 |
| --catalog         |      | 指定收集编目节点快照     |
| --group           |      | 指定收集数据复制组的信息 |
| --context         |      | 指定收集上下文快照信息   |
| --session         |      | 指定收集会话快照信息     |
| --conllection     |      | 指定收集集合快照信息     |
| --collectionspace |      | 指定收集集合空间快照信息 |
| --database        |      | 指定收集数据库快照信息   |
| --system          |      | 指定收集系统快照信息     |
| --diskmanage      |      | 指定收集操作系统硬盘管理信息 |
| --basicsys        |      | 指定收集操作系统基本信息 |
| --module          |      | 指定收集内核加载模块信息 |
| --env             |      | 指定收集操作系统环境变量信息 |
| --network         |      | 指定收集 IP 地址等网络信息   |
| --process         |      | 指定收集操作系统进程信息     |
| --login           |      | 指定收集用户登录该主机执行过的历史操作信息 |
| --limit           |      | 指定收集操作系统用户限制信息 |
| --vmstat          |      | 指定收集给定时间间隔内的服务状态值信息 |

示例
----

- 不带任何参数表示收集本机的所有信息（包括配置、日志、硬件、操作系统及快照信息）
```lang-bash
$ /opt/sequoiadb/tools/sdbsupport/sdbsupport.sh
```

- 收集整个数据库集群信息
```lang-bash
$ /opt/sequoiadb/tools/sdbsupport/sdbsupport.sh --all
```

- 收集特定主机特定端口的日志信息和快照信息
```lang-bash
$ /opt/sequoiadb/tools/sdbsupport/sdbsupport.sh -s hostname -p 11810 -n --log
```

* 指定 --hostname 收集其他主机信息或指定 --all 收集数据库集群内的信息，且主机之间没有配置信任关系时，需要输入密码，以 --all 为例：

   收集整个数据库集群的信息

   ```lang-bash
   $ /opt/sequoiadb/tools/sdbsupport/sdbsupport.sh --all
   ```

   提示输入密码

   ```lang-text
   ************************************Sdbsupport***************************
   * This program run mode will collect all configuration and
   * system environment information.Please make sure whether
   * you need !
   * Begin .....
   *************************************************************************
   
   check over environment, correct!
   complete database cluster
   The host sdbadmin@hostname2's password :
   correct password for hostname2
   The host sdbadmin@hostname3's password :
   correct password for hostname3
   
   Begin to Collect information...
   success to collect information from hostname3
   success to collect information from hostname2
   success to collect information from hostname1
   ```

检查报告解析
----

收集的信息会放在与 `sdbsupport.sh` 文件同级的 `log` 目录中。收集的信息以主机为单位打包成压缩包存放，名称以“主机名-年月日-时分秒”的格式命名。将压缩包解压后会得到四个文件夹，分别为 SDBNODES、SDBSNAPS、OSINFO 和 HARDINFO，当携带的参数指定收集的信息不包含四类中的某一类时，压缩包中不包含该文件夹。
  
+ SDBNODES：存放收集的数据库配置（包括CM配置和节点配置）和日志信息
+ SDBSNAPS：存放收集的数据库快照信息
+ OSINFO：存放收集的操作系统信息
+ HARDINFO：存放收集的硬件信息

