[^_^]:
    元数据映射

元数据映射功能用于增大 MariaDB 实例在单库建表时的数量限制。由于 SequoiaDB 集合空间的[限制][limit]，MariaDB 实例的单库建表数量不能超过 4096。因此，在 v3.6/5.0.3 及以上版本的 MariaDB 实例组件中，引入元数据映射功能。开启该功能后，用户可将表与不同集合空间的集合建立映射关系，以突破建表数量的限制。

##使用##

元数据映射功能默认为关闭状态，用户需手动开启。下述以实例 my_inst1 为例，介绍元数据映射功能的使用。

1. 在 SDB Shell 创建一个名为“SysMetaGroup”的数据复制组，用于存储映射关系
  
    ```lang-javascript
    > var rg = db.createRG("SysMetaGroup")
    > rg.createNode("sdbserver1", 11850, "/opt/sequoiadb/database/data/11850")
    > rg.createNode("sdbserver2", 11850, "/opt/sequoiadb/database/data/11850")
    > rg.createNode("sdbserver3", 11850, "/opt/sequoiadb/database/data/11850")
    > rg.start()
    ```

2. 连接实例 my_inst1，该实例所属实例组为 sql_group

    ```lang-bash
    $ mysql --socket=/opt/sequoiasql/mariadb/database/3206/mysqld.sock -u sdbadmin
    ```

3. 创建实例用户

    ```lang-sql
    MariaDB [(none)]> grant all privileges on *.* to sdbadmin@'%' identified by 'sdbadmin' with grant option;
    ```

4. 开启元数据映射功能

    ```lang-bash
    $ sql_enable_mapping --instance=my_inst1 -u sdbadmin --password=sdbadmin
    ```

    >**Note:**
    >
    > - 如果实例不属于任何实例组，开启元数据映射功能时需指定参数 --standalone。
    > - 工具 sql_enable_mapping 的详细参数可参考[启用工具][sql_enable_mapping]。

5. 创建表并建立映射关系

    以分区表 tb1 为例，将分区 p0、p1、p2、p3 分别映射至集合 CS1.cl1、CS2.cl1、CS3.cl2、CS4.cl2

    ```lang-sql
    MariaDB [company]> CREATE TABLE `tbl` (
      `id` int(11) NOT NULL,
      `produced_date` date DEFAULT NULL,
      `name` varchar(100) COLLATE utf8mb4_bin DEFAULT NULL,
      `company` varchar(100) COLLATE utf8mb4_bin DEFAULT NULL
    ) COMMENT='sequoiadb: { mapping: "dbx.tx"}'
     PARTITION BY RANGE (year(`produced_date`)) (
        PARTITION `p0` VALUES LESS THAN (2021) COMMENT = 'sequoiadb: { mapping: "CS1.cl1"}' ENGINE = SequoiaDB,
        PARTITION `p1` VALUES LESS THAN (2022) COMMENT = 'sequoiadb: { mapping: "CS2.cl1"}' ENGINE = SequoiaDB,
        PARTITION `p2` VALUES LESS THAN (2023) COMMENT = 'sequoiadb: { mapping: "CS3.cl2"}' ENGINE = SequoiaDB,
        PARTITION `p3` VALUES LESS THAN (2024) COMMENT = 'sequoiadb: { mapping: "CS4.cl2"}' ENGINE = SequoiaDB
    );
    ```

6. 查询映射关系

    ```lang-bash
    $ sql_get_mapping --instance my_inst1 --table company.tbl --long
    ```

    输出结果如下：

    ```lang-text
    DBName     TableName    CSName    CLName    
    company    tbl          dbx       tx        
    company    tbl#P#p0     CS1       cl1       
    company    tbl#P#p1     CS2       cl1       
    company    tbl#P#p2     CS3       cl2       
    company    tbl#P#p3     CS4       cl2      
    ```

    >**Note:**
    >
    > 工具 sql_get_mapping 的详细参数可参考[查询工具][sql_get_mapping]。

##启用工具##

sql_enable_mapping 工具用于开启元数据映射功能。

###语法规则###

```lang-text
sql_enable_mapping [--instance=NAME] [-uNAME|--user=NAME] [-p|--password=PASSWORD] [--standalone] [--manual-restart]
```

###参数说明###

| 参数 | 描述 | 是否必填 |
| ---- | ---- | -------- |
| --instance | 指定实例名 | 否 |
| -u，--user | 指定实例用户的用户名 | 否 |
| -p，--password | 指定实例用户的密码 | 否 |
| --standalone | 单实例模式 | 否 |
| --manual-restart | 重启 MariaDB 实例<br>当 sdbcm 服务不可用时，需指定该参数 | 否 |
| -h，--help | 获取帮助信息 | 否 |
| -v，--version | 获取版本信息 | 否 |

##查询工具##

sql_get_mapping 工具用于查询表与集合的映射关系。

###语法规则###

```lang-text
sql_get_mapping [--instance=NAME] [--table=NAME] [--collection=NAME] [-l|--long]
```

###参数说明###

| 参数 | 描述 | 是否必填 |
| ---- | ---- | -------- |
| --instance | 指定实例名 | 否 |
| --table | 指定集合所映射的表名 | 否 |
| --collection | 指定表所映射的集合名 | 否 |
| -l，--long | 显示详细的映射信息 | 否 |
| -h，--help | 获取帮助信息 | 否 |
| -v，--version | 获取版本信息 | 否 |
| --verbose | 输出工具的日志信息 | 否 |

##配置##

元数据映射功能开启后，支持将表映射至 10 个不同的集合空间，单个集合空间可创建的集合数量为 1024。如果上述配置仍无法满足业务需求，用户可参考 [SQL 实例配置][config]进行修改。

[^_^]:
     本文使用的所有引用和链接
[config]:manual/Database_Instance/Relational_Instance/MariaDB_Instance/Maintainance/config.md###配置元数据映射功能
[limit]:manual/Manual/sequoiadb_limitation.md
[sql_enable_mapping]:manual/Database_Instance/Relational_Instance/MariaDB_Instance/Maintainance/metadata_mapping_management.md##启用工具
[sql_get_mapping]:manual/Database_Instance/Relational_Instance/MariaDB_Instance/Maintainance/metadata_mapping_management.md##查询工具