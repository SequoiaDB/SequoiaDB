##名称##

Sdb - SequoiaDB 连接对象。 

##语法##

**var db = new Sdb( [hostname], [svcname] )**

**var db = new Sdb( [hostname], [svcname], [username], [password] )**

**var db = new Sdb( [hostname], [svcname], [User] )**

**var db = new Sdb( [hostname], [svcname], [CipherUser] )**

##类别##

Sdb

##描述##

新建一个 Sdb 对象，用于连接 SequoiaDB。

##参数##

| 参数名     | 参数类型 | 默认值             | 描述            | 是否必填 |
| ---------- | -------- | ------------------ | --------------- | -------- |
| hostname   | string   | localhost          | 主机名          | 否       |
| svcname    | int      | 11810              | 节点端口号      | 否       |
| username   | string   | 默认为空（''）     | 用户名          | 否       |
| password   | string   | 默认为空（''）     | 密码            | 否       |
| User       | object   | ---                | [User][ser] 对象       | 否       |
| CipherUser | object   | ---                | [CipherUser][cipher] 对象 | 否       |

> **Note:**

> * 可以通过 [createUsr()][createUsr] 创建 SequoiaDB 的用户，并设置对应的密码。

> * 当 SequoiaDB 没有用户时，创建 Sdb 对象可以不使用 username 和 password，否则必须使用相应的 username 和 password 去创建 Sdb 对象。

##返回值##

成功：返回 Sdb 对象。  

失败：抛出异常。

##错误##

`Sdb()`函数常见异常如下：

| 错误码 | 错误类型 | 描述 | 解决方法 |
| ------ | --- | ------------ | ----------- |
| -15 | SDB_NETWORK | 网络错误 | 检查填写的地址或者端口是否可达。|
| -79 | SDB_NET_CANNOT_CONNECT| 无法连接指定的地址 | 检查地址、端口以及节点的配置信息是否正确。|
| -104 | SDB_CLS_NOT_PRIMARY| 分区组不存在主节点 | 检查当前分区组是否存在 "IsPrimary" 为 "true" 的节点。若当前分区组存在节点未启，请启动节点。|
| -250 | SDB_CLS_NODE_BSFAULT | 节点状态不正确  | 检查节点状态，如检查 catalog 节点是否启动。|

如果出错则抛异常，并输出错误信息，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取错误码。
关于错误处理可以参考[常见错误处理指南][faq]。

常见错误可参考[错误码][Sequoiadb_error_code]。

##版本##

v1.12及以上版本。

##示例##

1. 连接默认主机上的 SequoiaDB，hostname 默认为 "localhost"，svcname 默认为 11810。

	```lang-javascript
 	> var db = new Sdb()
 	```

2. 连接指定机器上的 SequoiaDB，目标机器 "sdbserver1"。

	```lang-javascript
 	> var db = new Sdb( "sdbserver1", 11810 )
	```

3. 使用用户名和密码连接指定机器上的 SequoiaDB。

	```lang-javascript
 	> var db = new Sdb( "sdbserver1", 11810, "sdbadmin", "sdbadmin" )
	```

4. 使用 User 对象连接指定机器上的 SequoiaDB。

	```lang-javascript
    > var a = User( "sdbadmin" ).promptPassword()
    password:
    sdbadmin
 	> var db = new Sdb( "sdbserver1", 11810, a )
	```

5. 使用 CipherUser 对象连接指定机器上的 SequoiaDB（密文文件中必须存在用户名为 sdbadmin，密码为 sdbadmin 的用户信息，关于如何在密文文件中添加删除密文信息，详细可见[sdbpasswd][passwd]）。

   	```lang-javascript
    > var a = CipherUser( "sdbadmin" )
 	> var db = new Sdb( "sdbserver1", 11810, a )
	```


[^_^]:
     本文使用的所有引用及链接
[ser]:manual/Manual/Sequoiadb_Command/AuxiliaryObjects/User.md
[cipher]:manual/Manual/Sequoiadb_Command/AuxiliaryObjects/CipherUser.md
[createUsr]:manual/Manual/Sequoiadb_Command/Sdb/createUsr.md
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[Sequoiadb_error_code]:manual/Manual/Sequoiadb_error_code.md
[passwd]:manual/Distributed_Engine/Maintainance/Mgmt_Tools/sdbpasswd.md