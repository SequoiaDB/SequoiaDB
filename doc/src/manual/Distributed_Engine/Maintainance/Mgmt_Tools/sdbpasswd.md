sdbpasswd 是 SequoiaDB 巨杉数据库的密码管理工具，该工具可以把指定数据库用户的用户名和密码以密文方式保存在密文文件中，使用户可以基于该密文文件使用密文模式输入密码。

> **Note：**
> 
> 关于密文模式的介绍可参考[密码管理][passwd]。

##参数说明##

sdbpasswd 工具支持以下设置参数：

| 参数        | 缩写 | 描述                                              | 是否必填 |
| ----------- | ---- | ------------------------------------------------- | -------- |
| --help      | -h   | 返回帮助说明                                      |    否    |
| --adduser   | -a   | 增加用户，支持使用 @ 符区分同名用户，格式：user@cluster1。 |  adduser/removeuser 必选其一 |
| --removeuser| -r   | 删除用户                                          | adduser/removeuser 必选其一 |
| --password  | -p   | 指定数据库用户密码，如果不使用该参数指定密码，工具会通过交互式界面提示用户输入密码。 | 是 |
| --token     | -t   | 对保存的密码指定加密令牌以增强安全性，所设置的加密令牌不能超过 256 个字节 | 否 |
| --file      | -f   | 指定密文文件的路径，默认为 `~/sequoiadb/passwd`；如果是新文件，默认创建权限为 0600 | 否 |

> **Note:**
>
> - 用户通过 sdbpasswd 工具增加和删除用户只会在密文文件中进行操作，而不会在 SequoiaDB 引擎中进行操作。因此 SequoiaDB 引擎的用户信息发生变化时，需要使用 sdbpasswd 工具同步更新密文文件中的用户信息。
> - 用户通过 sdbpasswd 工具更改数据库用户的密码时，只需要重新添加用户，使密文文件中相同用户名的记录被覆盖即可。
> - 数据库用户名和密码的限制请参考[数据库限制][databast_limite]

##使用##

运行 sdbpasswd 命令的用户必须对准备写入密码的加密文件具有读写权限。

* 增加用户 sdbadmin，并指定密码为 sdbadmin

   ```lang-bash
   $ sdbpasswd --adduser sdbadmin --password sdbadmin
   ```

* 增加用户 sdbadmin，不指定 password 的值

   ```lang-bash
   $ sdbpasswd --adduser sdbadmin --password
   password:
   ```

* 增加用户 sdbadmin，且通过 @ 符区分从属于不同集群的用户

   ```lang-bash
   $ sdbpasswd --adduser sdbadmin@db1 --password 123456
   $ sdbpasswd --adduser sdbadmin@db2 --password 654321
   ```
   > **Note:**
   >
   > 数据库工具在使用加密文件方式连接数据库时，会自动去掉 @ 及后面部分，使用原始用户名连接数据库。

* 增加用户 sdbadmin，指定密码为 sdbadmin，并指定加密令牌为 sequoiadb

   ```lang-bash
   $ sdbpasswd --adduser sdbadmin --password sdbadmin --token sequoiadb
   ```

* 增加用户 sdbadmin，指定密码为 sdbadmin，并指定待写入密文文件路径

   ```lang-bash
   $ sdbpasswd --adduser sdbadmin --password sdbadmin --file ./cipher
   ```

* 删除用户 sdbadmin@db1

   ```lang-bash
   $ sdbpasswd --removeuser sdbadmin@db1
   ```
   > **Note:**
   >
   > 删除用户时，对于使用了 @ 符的用户，需要指定全名才能匹配删除。

##与数据库工具配合使用##

其他数据库工具在连接数据库时可以使用密文模式输入密码。

> **Note:**
>
> 如果用户使用 sdbpasswd 工具增加用户时指定了 token，其他工具在使用密文模式时也需要指定 token。

###示例###

* 与[数据导出工具][sdbexprt]配合使用

   ```lang-bash
   $ sdbexprt -s localhost -p 11810 --type csv --file SAMPLE.EMPLOYEE.csv --fields field1,fieldNotExist,field3 -c SAMPLE -l EMPLOYEE --user sdbadmin --cipher true --token sequoiadb --cipherfile ./passwd
   ```

* 与[数据导入工具][sdbimprt]配合使用

   ```lang-bash
   $ sdbimprt -s localhost -p 11810 -c SAMPLE -l EMPLOYEE --file SAMPLE.EMPLOYEE.csv --type csv --headerline true --fields='c int,d string' --user sdbadmin --cipher true --token sequoiadb --cipherfile ./passwd
   ```

* 与[日志重放工具][log_replay]配合使用

   ```lang-bash
   $ sdbreplay --hostname localhost --svcname 11810 --path 20000/archivelog/archivelog.0 --user sdbadmin --cipher true --token sequoiadb --cipherfile ./passwd
   ```

* 与[数据库性能工具][sdbtop]配合使用

   ```lang-bash
   $ sdbtop -i localhost -s 11810 --usrname sdbadmin --cipher true --token sequoiadb --cipherfile ./passwd
   ```

* 与[大对象工具][lobtools]配合使用

   ```lang-bash
   $ sdblobtool --operation export --hostname localhost --svcname 50000 --collection SAMPLE.EMPLOYEE --file /opt/mylob --usrname sdbadmin --cipher true --token sequoiadb --cipherfile ./passwd
   ```

* 与[节点数据一致性检测工具][consistency_check]配合使用

   ```lang-bash
   $ sdbinspect -d localhost:50000 -o item.bin --auth sdbadmin --cipher true --token sequoiadb --cipherfile ./passwd
   ```

* 与[Sequoiasql-pgsql][connection]配合使用

   ```lang-bash
   SAMPLE=# create server sdb_server foreign data wrapper sdb_fdw options(address '127.0.0.1', service '11810', user 'sdbUserName', cipher 'on', token 'sequoiadb', cipherfile '/opt/sequoiadb/passwd', preferredinstance 'A', transaction 'off');
   ```


[^_^]:
     本文使用的所有链接及引用
[passwd]:manual/Distributed_Engine/Maintainance/Security/system_security.md#密码管理
[sdbexprt]:manual/Distributed_Engine/Maintainance/Mgmt_Tools/sdbexprt.md
[sdbimprt]:manual/Distributed_Engine/Maintainance/Mgmt_Tools/sdbimprt.md
[log_replay]:manual/Distributed_Engine/Maintainance/Mgmt_Tools/log_replay.md
[sdbtop]:manual/Distributed_Engine/Maintainance/Monitoring/sdbtop.md
[lobtools]:manual/Distributed_Engine/Maintainance/Mgmt_Tools/lobtools.md
[consistency_check]:manual/Distributed_Engine/Maintainance/Mgmt_Tools/consistency_check.md
[connection]:manual/Database_Instance/Relational_Instance/PostgreSQL_Instance/Operation/connection.md
[databast_limite]:manual/Manual/sequoiadb_limitation.md#数据库
