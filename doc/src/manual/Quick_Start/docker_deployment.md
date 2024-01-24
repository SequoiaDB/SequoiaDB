[^_^]:
    容器模式部署
    作者：秦志强
    时间：
    评审意见
    王涛：
    许建辉：时间：
    市场部：时间：20190321

SequoiaDB 巨杉数据库支持容器模式部署。目前，推荐使用 Docker 容器。

Docker 是一个开源的应用容器引擎，让开发者可以打包应用以及依赖包到一个可移植的容器中，便于发布到任何流行的 Linux 机器上，以实现虚拟化。容器是完全使用沙箱机制，相互之间不会有任何接口。Docker 通过 LXC 来实现类似 VM 的功能，节省硬件资源的同时提供给用户更多的计算资源。

SequoiaDB 巨杉数据库为用户提供了 Docker 镜像，用户可以通过镜像快速部署集群进行开发和测试工作。本文主要讲解如何在 Linux 系统上安装 Docker 应用并拉取 SequoiaDB 巨杉数据库的镜像进行安装部署，同时也将展示如何在部署后的环境中进行 MySQL 实例的 CRUD 操作。


集群配置
----

用户可以在五个容器中部署一个多节点高可用 SequoiaDB 集群。集群包含一个协调节点、一个编目节点、三个三副本数据节点和一个 MySQL 实例节点。

| 主机名 | IP | 复制组 | 软件版本 |
| ------ | ------ | ------ |------ |
| Coord 协调节点 | 172.17.0.2:11810 | SYSCoord | SequoiaDB |
| Catalog编目节点 | 172.17.0.2:11800 | SYSCatalogGroup | SequoiaDB |
| Data1数据节点1 | 	172.17.0.3:11820 | group1 | SequoiaDB |
| Data2数据节点2 | 	172.17.0.4:11820 | group1 | SequoiaDB |
| Data3数据节点3 | 172.17.0.5:11820 | group1 | SequoiaDB |
| Data1数据节点2 | 172.17.0.4:11830 | group2 | SequoiaDB |
| Data2数据节点3 | 172.17.0.5:11830 | group2 | SequoiaDB |
| Data3数据节点1 | 172.17.0.3:11830 | group2 | SequoiaDB |
| Data1数据节点3 | 172.17.0.5:11840 | group3 | SequoiaDB |
| Data2数据节点1 | 172.17.0.3:11840| group3 | SequoiaDB |
| Data3数据节点2 | 172.17.0.4:11840 | group3 | SequoiaDB |
| MySQL实例 | 172.17.0.6:3306 | - | SequoiaSQL-MySQL |

Docker环境部署SequoiaDB
----
1. 下载镜像并上传至服务器

   ```lang-bash
   wget http://cdn.sequoiadb.com/images/tools/sequoiadb_docker_image.tar.gz
   ```

2. 解压 `sequoiadb_docker_image.gz` 包

   ```lang-bash
   tar -zxvf sequoiadb_docker_image.tar.gz
   ```

3. 恢复镜像 `sequoiadb.tar`

   ```lang-bash
   docker load -i sequoiadb.tar
   ```

4. 恢复镜像 `sequoiasql-mysql.tar`

   ```lang-bash
   docker load -i sequoiasql-mysql.tar
   ```

5. 启动四个 SequoiaDB 容器

   ```lang-bash
   docker run -it -d --name coord_catalog sequoiadb/sequoiadb:latest
   docker run -it -d --name sdb_data1 sequoiadb/sequoiadb:latest
   docker run -it -d --name sdb_data2 sequoiadb/sequoiadb:latest
   docker run -it -d --name sdb_data3 sequoiadb/sequoiadb:latest
   ```

6. 查看四个容器的容器 ID

   ```lang-bash
   docker ps -a | awk '{print $NF}'
   ```

7. 查看四个容器对应的 IP 地址
   
   ```lang-bash
   docker inspect coord_catalog | grep IPAddress |awk 'NR==2 {print $0}'
   docker inspect sdb_data1 | grep IPAddress |awk 'NR==2 {print $0}'
   docker inspect sdb_data2 | grep IPAddress |awk 'NR==2 {print $0}'
   docker inspect sdb_data3 | grep IPAddress |awk 'NR==2 {print $0}'
   ```

8. 部署 SequoiaDB 集群，根据集群规划以及各个容器的 IP 地址，在对应参数填入各自的地址与端口号（建议存储空间在 30G 以上）：

   ```lang-bash
   docker exec coord_catalog "/init.sh" \
   --coord='172.17.0.2:11810' \
   --catalog='172.17.0.2:11800'
   --data='group1=172.17.0.3:11820,172.17.0.4:11820,172.17.0.5:11820; group2=172.17.0.4:11830,172.17.0.5:11830,172.17.0.3:11830;group3=172.17.0.5:11840,172.17.0.3:11840,172.17.0.4:11840'
   ```

9. 启动一个 MySQL 实例容器，并查看启动容器的 ID

   ```lang-bash
   docker run -it -d -p 3306:3306 --name mysql sequoiadb/sequoiasql-mysql:latest
   ```

10. 查看容器 IP 地址

   ```lang-bash
   docker inspect mysql | grep IPAddress | awk 'NR==2 {print $0}'
   ```

11. 将 MySQL 实例注册入协调节点

   ```lang-bash
   docker exec mysql "/init.sh" --port=3306 --coord='172.17.0.2:11810'
   ```

12. 进入 mysql 容器

   ```lang-bash
   docker exec -it mysql /bin/bash
   ```

13. 查看 mysql 实例状态

   ```lang-bash
   /opt/sequoiasql/mysql/bin/sdb_mysql_ctl status
   ```

14. 进入 coord_catalog 容器，查看 SequoiaDB 存储引擎节点列

 - 查看 sequoiadb 容器名称

   ```lang-bash
   docker ps -a | awk '{print $NF}'
   ```

 - 进入 coord_catalog 容器查看编目节点和协调节点

   ```lang-bash
   docker exec -it coord_catalog /bin/bash
   ```

 - 切换 sdbamdin 用户（sdbadmin 用户密码为 Admin@1024）

   ```lang-bash
   su - sdbadmin
   ```

 - 查看编目节点和协调节点列表

   ```lang-bash
   sdblist –t all –l
   ```

 - 退出容器
 
   ```lang-bash
   exit
   ```

数据库对接开发
----

**MySQL Shell**

1. 进入 mysql 容器中

   ```lang-bash
   docker exec -it mysql /bin/bash
   ```

2. 启动 mysql（如未启动）

   ```lang-bash
   /opt/sequoiasql/mysql/bin/sdb_mysql_ctl start MySQLInstance
   ```

3. 登录到 MySQL Shell

   ```lang-bash
   /opt/sequoiasql/mysql/bin/mysql -h 127.0.0.1 -P 3306 -u root
   ```

4. 创建新数据库 company，并切换到 company

   ```lang-sql
   CREATE DATABASE company;
   USE company;
   ```

5. 在 company 数据库中创建数据表 employee

   ```lang-sql
   CREATE TABLE employee
   (
   empno INT AUTO_INCREMENT PRIMARY KEY,
   ename VARCHAR(128),
   age INT
   );
   ```

6. 在表 employee 中插入如下数据：

   ```lang-sql
   INSERT INTO employee (ename, age) VALUES ("Jacky", 36);
   INSERT INTO employee (ename, age) VALUES ("Alice", 18);
   ```

7. 查询 employee 表中的数据

   ```lang-sql
   SELECT * FROM employee;
   ```

8. 退出 mysql 容器

   ```lang-sql
   quit
   ```

**SDB Shell**

1. 进入 coord_catalog 容器中，并进入 SDB Shell 交互式界面，使用 JavaScript 连接协调节点，获取数据库连接

   ```lang-javascript
   docker exec -it coord_catalog /bin/bash
   su sdbadmin
   sdb
   var db = new Sdb("localhost", 11810);
   ```

2. 使用 insert() 向 SequoiaDB 集合中写入记录

   ```lang-javascript
   db.sample.employee.insert( { ename: "Abe", age: 20 } );
   ```

3. 使用 find() 方法从集合中查询数据记录

   ```lang-javascript
   db.sample.employee.find( { ename: "Abe" } );
   ```

4. 使用 update() 方法将集合中的记录进行修改

   ```lang-javascript
   db.sample.employee.update( { $set: { ename: "Ben" } }, { ename: "Abe" } );
   ```

5. 使用 remove() 方法从集合中删除数据

   ```lang-javascript
   db.sample.employee.remove( { ename: "Ben" } );
   ```



