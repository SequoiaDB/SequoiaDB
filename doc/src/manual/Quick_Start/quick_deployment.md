本入门教程使用 SequoiaDB v3.4 及 MySQL 实例组件 v3.4 在 Ubuntu 16.04 上搭建一个基础运行环境，以快速了解 SequoiaDB 巨杉数据库及 MySQL 实例组件的基本功能。

SequoiaDB 可以选择部署在单台机器上，也可以部署在多台机器上。

## 部署SequoiaDB及MySQL实例

用户在部署 SequoiaDB 及 MySQL 实例之前应先完成 [SequoiaDB 安装][engine_install] 和 [MySQL 实例组件安装][install_deploy]。SequoiaDB 部署方案可以选择在单台机器上进行伪集群部署，或者在多台机器上进行集群部署。

**部署前准备**

在进行集群部署之前，需要使用 root 用户或者管理员用户登录主机，在每台主机上执行以下命令查看 11800 端口是否被占用：

```lang-bash
# netstat -anp | grep 11800
```

   > **Note：**
   >
   > SequoiaDB 默认需要的端口号为 11800、11810、11820、11830、11840 及 18800，MySQL 实例默认需要的端口号为 3306，需确保这些端口没有被占用。

**部署工具说明**

* 以下介绍的伪集群部署和集群部署均使用快速部署工具进行操作，快速部署工具的使用与配置可参考 [quickDeploy.sh][quickdeploy]。 

* 如果用户在执行快速部署的过程中发生异常，则需要解决异常后再次执行 `quickDeploy.sh` 命令。如果依然失败，应在软件包解压路径下执行 `./setup.sh --clean` 命令进行环境清理，然后按照快速入门指南重新操作一次。

### 伪集群部署

部署 SequoiaDB 到本机上，创建三个数据组，每个数据组单副本，并创建一个 MySQL 实例。

![伪集群部署][quickstart_1]

1. 切换到 sdbadmin 用户

   ```lang-bash
   # su - sdbadmin
   ``` 

2. 切换到 SequoiaDB 安装目录下

   ```lang-bash
   $ cd /opt/sequoiadb 
   ```

3. 执行快速部署工具

   ```lang-bash
   $ ./tools/deploy/quickDeploy.sh
   ```
  
   输出以下信息则表示部署成功：

   ```lang-text
   ************ Deploy SequoiaDB ************************
   Create catalog: sdbserver1:11800
   Create coord:   sdbserver1:11810
   Create data:    sdbserver1:11820
   Create data:    sdbserver1:11830
   Create data:    sdbserver1:11840
  
   ************ Deploy SequoiaSQL-MySQL *****************
   Create instance: [name: myinst, port: 3306]
   ```

### 集群部署

部署 SequoiaDB 到三台机器上，主机名分别为 sdbserver1/sdbserver2/sdbserver3，创建三个数据组，每个数据组三副本，并在 sdbserver1 上创建一个 MySQL 实例。

![集群部署][quickstart_2]

1. 切换到 sdbadmin 用户

   ```lang-bash
   # su - sdbadmin
   ```

2. 切换到 SequoiaDB 安装目录下

   ```lang-bash
   $ cd /opt/sequoiadb
   ```

3. 修改第一台主机 sdbserver1 上的配置文件 `tools/deploy/sequoiadb.conf`（
  主机名 sdbserver1/sdbserver2/sdbserver3 可根据实际需要修改），修改内容如下：

  ```lang-ini
  role,groupName,hostName,serviceName,dbPath
  
  catalog,SYSCatalogGroup,sdbserver1,11800,[installPath]/database/catalog/11800
  catalog,SYSCatalogGroup,sdbserver2,11800,[installPath]/database/catalog/11800
  catalog,SYSCatalogGroup,sdbserver3,11800,[installPath]/database/catalog/11800
  
  coord,SYSCoord,sdbserver1,11810,[installPath]/database/coord/11810
  coord,SYSCoord,sdbserver2,11810,[installPath]/database/coord/11810
  coord,SYSCoord,sdbserver3,11810,[installPath]/database/coord/11810
  
  data,group1,sdbserver1,11820,[installPath]/database/data/11820
  data,group1,sdbserver2,11820,[installPath]/database/data/11820
  data,group1,sdbserver3,11820,[installPath]/database/data/11820
  
  data,group2,sdbserver1,11830,[installPath]/database/data/11830
  data,group2,sdbserver2,11830,[installPath]/database/data/11830
  data,group2,sdbserver3,11830,[installPath]/database/data/11830
  
  data,group3,sdbserver1,11840,[installPath]/database/data/11840
  data,group3,sdbserver2,11840,[installPath]/database/data/11840
  data,group3,sdbserver3,11840,[installPath]/database/data/11840
  ```

4. 在主机 sdbserver1 上执行快速部署工具

   ```lang-bash
   $ ./tools/deploy/quickDeploy.sh
   ```
   输出以下信息则表示部署成功：

   ```lang-text
  ************ Deploy SequoiaDB ************************
  Create catalog: sdbserver1:11800
  Create catalog: sdbserver2:11800
  Create catalog: sdbserver3:11800
  Create coord:   sdbserver1:11810
  Create coord:   sdbserver2:11810
  Create coord:   sdbserver3:11810
  Create data:    sdbserver1:11820
  Create data:    sdbserver2:11820
  Create data:    sdbserver3:11820
  Create data:    sdbserver1:11830
  Create data:    sdbserver2:11830
  Create data:    sdbserver3:11830
  Create data:    sdbserver1:11840
  Create data:    sdbserver2:11840
  Create data:    sdbserver3:11840
  
  ************ Deploy SequoiaSQL-MySQL *****************
  Create instance: [name: myinst, port: 3306]
  ```

##基本操作##

**MySQL Shell**

- 登录 MySQL Shell

  ```lang-bash
  $ /opt/sequoiasql/mysql/bin/mysql -h 127.0.0.1 -P 3306 -u root
  ```

- 创建数据库实例

  ```lang-sql
  mysql> create database cs;
  Query OK, 1 row affected (0.00 sec)
  
  mysql> use cs;
  Database changed
  ```

- 创建表

  ```lang-sql
  mysql> create table cl(a int, b int, c text, primary key(a, b) ) ;
  Query OK, 0 rows affected (0.66 sec)
  ```

- 使用 SQL 语句进行增删改查操作

  ```lang-sql
  mysql> insert into cl values(1, 101, "SequoiaDB test");
  Query OK, 1 row affected (0.05 sec)
  
  mysql> insert into cl values(2, 102, "SequoiaDB test");
  Query OK, 1 row affected (0.01 sec)
  
  mysql> select * from cl order by b asc;
  +---+-----+----------------+
  | a | b   | c              |
  +---+-----+----------------+
  | 1 | 101 | SequoiaDB test |
  | 2 | 102 | SequoiaDB test |
  +---+-----+----------------+
  2 rows in set (0.00 sec)
  
  mysql> update cl set c="My test" where a=1;
  Query OK, 1 row affected (0.01 sec)
  Rows matched: 1  Changed: 1  Warnings: 0
  
  mysql> delete from cl where b=102;
  Query OK, 1 row affected (0.02 sec)
  
  mysql> select * from cl order by b asc;
  +---+-----+---------+
  | a | b   | c       |
  +---+-----+---------+
  | 1 | 101 | My test |
  +---+-----+---------+
  1 row in set (0.00 sec)
  ```

**SDB Shell**

* 登录 SDB Shell

   ```lang-bash
   $ sdb
   ```

* 使用 JavaScript 连接协调节点

   ```lang-json
   > var db = new Sdb("localhost", 11810)
   ```

* 创建集合空间

   ```lang-json
   db.createCS("sample")
   ```

* 创建集合

   ```lang-json
   db.sample.createCL("employee")
   ```

* 向集合 sample.employee 中插入两条数据

   ```lang-json
   > db.sample.employee.insert({"id":1,"name":"xiaoli","phone":5553})
   > db.sample.employee.insert({"id":2,"name":"xiaozhang","phone":1371})
   ```

* 修改字段"phone"为 5553 的记录，将"name"的值修改为"xiaolili"

   ```lang-json
   > db.sample.employee.update({$set:{"name":"xiaolili"}},{"phone":5553})
   ```

* 查询集合 sample.employee 中的记录

   ```lang-json
   > db.sample.employee.find()
   ```

   输出结果如下：
   
   ```lang-json
   {
     "_id": {
       "$oid": "5c98d499ee15aef104e88722"
     },
     "id": 1,
     "name": "xiaolili",
     "phone": 5553
   }
   {
     "_id": {
       "$oid": "5c98d499ee15aef104e88723"
     },
     "id": 2,
     "name": "xiaozhang",
     "phone": 1371
   Return 2 row(s).
   ```

* 删除字段"phone"为 5553 的记录
   
   ```lang-json
   > db.sample.employee.remove({"phone":5553})
   ```


[^_^]:
    本文所用到的所有链接和引用

[engine_install]:manual/Deployment/engine_install.md
[install_deploy]:manual/Database_Instance/Relational_Instance/MySQL_Instance/Installation/install_deploy.md
[quickdeploy]:manual/Distributed_Engine/Maintainance/Mgmt_Tools/quickdeploy.md
[setup]:manual/Quick_Start/quick_deployment.md#清除SequoiaDB及MySQL实例组件
[quickstart_1]:images/Quick_Start/quickstart_1.png
[quickstart_2]:images/Quick_Start/quickstart_2.png