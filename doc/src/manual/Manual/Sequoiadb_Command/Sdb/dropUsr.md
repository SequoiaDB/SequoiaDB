##名称##

dropUsr - 删除数据库用户

##语法##

**db.dropUsr( \<name\>, \<password\> )**

**db.dropUsr( \<User\> )**

**db.dropUsr( \<CipherUser\> )**

##类别##

Sdb

##描述##

该函数用于删除数据库用户。

##参数##

| 参数名     | 参数类型 | 描述            | 是否必填 |
| ---------- | -------- | --------------- | -------- |
| name       | string   | 用户名          | 是       |
| password   | string   | 密码            | 是       |
| User       | object   | [User][user] 对象       | 是       |
| CipherUser | object   | [CipherUser][cipherUser] 对象 | 是       |

> **Note:**
>
> * 删除用户时，如果集群中除待删除用户外，不存在任何一个用户拥有内建角色`_root`或者旧版本`admin`, 将会删除失败

##返回值##

函数执行成功时，无返回值。

函数执行失败时，将抛异常并输出错误信息。

##错误##

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取错误码。更多错误处理可以参考[常见错误处理指南][error_guide]。

##版本##

v2.0 及以上版本

##示例##

* 删除用户名为 sdbadmin，密码为 sdbadmin 的用户。

    ```lang-javascript
    > db.dropUsr( "sdbadmin", "sdbadmin" )
    ```

* 使用 User 对象删除用户名为 sdbadmin，密码为 sdbadmin 的用户。

    ```lang-javascript
    > var a = User( "sdbadmin", "sdbadmin" )
    > db.dropUsr( a )
    ```

* 使用 CipherUser 对象删除用户名为 sdbadmin，密码为 sdbadmin 的用户（密文文件中必须存在用户名为 sdbadmin，密码为 sdbadmin 的用户信息，关于如何在密文文件中添加删除密文信息，详细可见[sdbpasswd](manual/Distributed_Engine/Maintainance/Mgmt_Tools/sdbpasswd.md)）。

    ```lang-javascript
    > var a = CipherUser( "sdbadmin" )
    > db.dropUsr( a )
    ```


[^_^]:
    本文使用的所有引用及链接
[user]:manual/Manual/Sequoiadb_Command/AuxiliaryObjects/User.md
[cipherUser]:manual/Manual/Sequoiadb_Command/AuxiliaryObjects/CipherUser.md
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_guide]:manual/FAQ/faq_sdb.md