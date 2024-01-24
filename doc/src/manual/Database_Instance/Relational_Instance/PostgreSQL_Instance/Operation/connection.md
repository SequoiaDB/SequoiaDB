[^_^]:
    PostgreSQL 实例-连接

PostgreSQL 实例组件安装后，需要将 PostgreSQL 实例与数据库分布式存储引擎进行连接，使用户可直接通过 PostgreSQL Shell 使用标准的 SQL 语言访问 SequoiaDB 巨杉数据库。

##连接 PostgreSQL 实例组件与存储引擎##

1. 创建 PostgreSQL 的数据库 sample

   ```lang-bash
   $ bin/sdb_pg_ctl createdb sample myinst
   ```

2. 进入 PostgreSQL Shell
 
   - 本地连接

     ```lang-bash
     $ bin/psql -p 5432 sample
     ```

   - 远程连接

     假设 PostgreSQL 服务器地址为 `sdbserver1:5432`，在客户端可以使用如下方式进行远程连接：

     ```lang-bash
     $ bin/psql -h sdbserver1 -p 5432 sample
     ```

     >**Note:**
     >
     > - PostgreSQL 默认未授予远程连接的访问权限，所以需要在服务端对客户端 IP 进行访问授权。
     > - 需确保本地创建的数据库与服务器创建的数据库同名，否则将会连接失败。

3. 加载 SequoiaDB 连接驱动

	```lang-sql
	sample=# create extension sdb_fdw;
	```

4. 配置 SequoiaDB 连接参数

    ```lang-sql
	sample=# create server sdb_server foreign data wrapper sdb_fdw options(address '127.0.0.1', service '11810', user 'sdbUserName', password 'sdbPassword', preferredinstance 'A', transaction 'off');
    ```

    >**Note:**
    >
    > 详细参数说明可参考 SequoiaDB 连接参数说明。

##SequoiaDB 连接参数说明##

| 参数名 | 类型 | 描述 | 是否必填 |
| ------ | ------   | ------ | ------ |
| user   | string   | 数据库用户名 | 否 |
| password | string | 数据库密码 | 否 |
| address | string | 协调节点地址，需要填写多个协调节点地址时，格式为：'ip1:port1,ip2:port2,ip3:port3'，service 字段可填写任意一个非空字符串 | 是 |
| service | string | 协调节点 serviceName | 是 |
| preferredinstance | string | 设置 SequoiaDB 的连接属性，多个属性以逗号分隔，如：preferredinstance '1,2,A'，详细配置可参考 [preferredinstance][preferredinstance] 取值 | 否 |
| preferredinstancemode | string | 设置 SequoiaDB 的连接属性 preferredinstance 的选择模式 | 否 |
| sessiontimeout | string | 设置 SequoiaDB 的连接属性会话超时时间，如：sessiontimeout '100' | 否 |
| transaction | string | 设置 SequoiaDB 与 PostgreSQL 之间的操作是否使用事务，默认为'off'，开启为'on' | 否 | 
| cipher | string | 设置是否使用加密文件输入密码，默认为'off'，开启为'on'<br> 密文模式的介绍可参考[密码管理][system_security] | 否 |
| token | string | 设置密文文件的加密令牌<br>如果创建密文文件时未指定 token，可忽略该参数 | 否 |
| cipherfile | string | 设置加密文件，默认为 `~/sequoiadb/passwd` | 否 |

>**Note:** 
>
> 如果用户没有配置数据库密码验证，可以忽略 user 与 password 字段。





[^_^]:
    本文使用到的所有连接及引用
[preferredinstance]:manual/Manual/Sequoiadb_Command/Sdb/setSessionAttr.md
[system_security]:manual/Distributed_Engine/Maintainance/Security/system_security.md
