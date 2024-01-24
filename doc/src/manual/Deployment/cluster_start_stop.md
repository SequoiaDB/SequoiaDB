[^_^]:
    集群启停
    作者：杨垚
    时间：20190312
    评审意见
    王涛：
    许建辉：时间：
    市场部：时间：20190911

本文档主要介绍 SequoiaDB 巨杉数据库集群的启动和停止操作。

集群管理服务
----
sdbcm 是数据库的集群管理服务，作为守护进程常驻于系统后台，负责执行远程的集群管理命令和监控本地的 SequoiaDB 数据库。sdbcm 服务处于运行状态时，会自动启动数据库中已有的节点，用户可以使用 `ps –elf | grep sequoiadb ` 查看已启动的节点。

> **Note:**
>
> 关于 sdbcm 的详细说明可参考[集群管理节点][cm_node]章节。


集群启动
----

数据库集群可通过操作系统命令和 SDB Shell 命令两种方式启动。

###操作系统命令###

用户可以通过 sdbstart 命令启动集群。

```lang-bash
$ sdbstart --help
Command options:
  -h [ --help ]          help
  --version              version
  -c [ --confpath ] arg  configure file path
  -p [ --svcname ] arg   service name, separated by comma (',')
  -t [ --type ] arg      node type: db/om/all, default: db
  -r [ --role ] arg      role type: coord/data/catalog/om
  --force                force start when the config not exist
  -o [ --options ] arg   SequoiaDB start arguments, but not use 
                         '-c/--confpath/-p/--svcname'
  -i [ --ignoreulimit ]  skip checking ulimit
```

- 启动当前服务器的数据节点、编目节点和协调节点

 ```lang-bash
 $ sdbstart
 ```

- 启动当前服务器的所有节点

 ```lang-bash
 $ sdbstart -t all
 ```
 
- 启动指定节点

 ```lang-bash
 sdbstart -p 11830,11840
 ``` 

- 启动所有数据节点

 ```lang-bash
 sdbstart -r data
 ```

 > **Note:**
 >
 > 用户可以通过 -r 参数指定所启动的服务角色。coord 为协调节点，data 为数据节点，catalog 为编目节点，om 为 Sequoiadb 数据库管理服务。

###SDB Shell 命令###

1. 启动 SDB Shell

 ```
 $ /opt/sequoiadb/bin/sdb
 ```

2. 连接至协调节点

 ```lang-javascript
 > var db = new Sdb( "localhost", 11810 )
 ```

3. 启动复制组或节点

   - 启动复制组

     ```lang-javascript
     > rg = db.getRG("group1")
     > rg.start()
     ```

     > **Note:**
     >
     > group1：复制组名

   - 启动复制组中的节点

     ```lang-javascript
     > rg = db.getRG("group1")
     > rg.getNode("sdbserver1",11830).start()
     ```

     > **Note:**
     >
     > - sdbserver1：节点所在主机名
     > - 11830：节点端口号


集群停止
----

数据库集群可通过操作系统命令和 SDB Shell 命令两种方式停止。

###操作系统命令###

用户可以通过 sdbstop 命令停止集群。

```lang-bash
$ sdbstop --help
Command options:
  -h [ --help ]         help
  --version             version
  -a [ --all ]          stop all nodes include db and om
  -t [ --type ] arg     node type: db/om/all
  -r [ --role ] arg     role type: coord/data/catalog/om
  -p [ --svcname ] arg  service name, separated by comma (',')
  --force               force stop when the node can't stop normally
```

- 停止当前服务器的数据节点、编目节点和协调节点

 ```lang-bash
 $ sdbstop
 ```

- 停止当前服务器的所有节点

 ```lang-bash
 $ sdbstop -t all
 ```

- 停止指定节点

 ```lang-bash
 $ sdbstop -p 11830,11840
 ```

- 停止所有数据节点

 ```lang-bash
 $ sdbstop -r data
 ```

 > **Note:**
 >
 > 用户可以通过 -r 参数指定所停止的服务角色。coord 为协调节点，data 为数据节点，catalog 为编目节点，om 为 Sequoiadb 数据库管理服务。

###SDB Shell 命令###

1. 启动 SDB Shell

 ```lang-bash
 $ /opt/sequoiadb/bin/sdb
 ```

2. 连接至协调节点

 ```lang-javascript
 > var db = new Sdb( "localhost", 11810 )
 ```

3. 停止复制组或节点

   - 停止复制组

     ```lang-javascript
     > rg = db.getRG("group1")
     > rg.stop()
     ```

     > **Note:**
     >
     > - group1：复制组名
     > - 协调节点组停止后，将不能通过 SDB Shell 操作数据库。

   - 停止复制组中的节点

     ```lang-javascript
     > rg = db.getRG("group1")
     > rg.getNode("sdbserver1",11830).start()
     ```

     > **Note:**
     >
     > - sdbserver1：节点所在主机名
     > - 11830：节点端口号



[^_^]:
    本文使用的所有引用和链接
[cm_node]:manual/Distributed_Engine/Architecture/Node/cm_node.md





