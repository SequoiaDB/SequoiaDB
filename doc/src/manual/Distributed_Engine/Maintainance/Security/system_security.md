SequoiaDB 巨杉数据库系统安全是通过鉴权、加密通信、审计日志和密码管理来保证的。

##鉴权##

SequoiaDB 默认开启鉴权功能，但若不存在数据库用户，则访问数据库不需要指定用户名和密码。因此 SequoiaDB 安装完成后，为了保障数据库系统安全，建议用户创建数据库用户并设置密码。SequoiaDB v3.4.1 及以上版本支持 SCRAM-SHA256 鉴权机制，详情可参考[鉴权机制][authentication_algorithm]。

> **Note:**
>
>* 数据库用户之间属于平级关系，没有超级用户的概念。
>* 每次新的会话都要进行新的鉴权

##加密通信##

SequoiaDB 通过配置可以支持 SSL，SequoiaDB 客户端和 SequoiaDB 实例直接可以使用 SSL 加密连接。若要使用 SSL 加密连接，需要安装 SequoiaDB v1.12 或更高版本。目前该特性仅限于 SequoiaDB 企业版，社区版暂不支持，详情可参考[网络安全][network_security]。

##审计日志##

SequoiaDB 提供了审计功能，审计日志记录了用户对数据库执行的所有操作。通过审计日志，用户可以对数据库进行故障分析、行为分析和安全审计等操作，能有效帮助用户获取数据库的执行情况。详情可参考[审计日志][auditlog]。

##密码管理##

对于 SequoiaDB v3.2.6 以下版本和 v3.4.1 以下版本，在进行连接数据库、创建数据库用户和删除数据库用户等涉及到密码的操作时，用户必须提供明文密码，例如：

连接数据库

```lang-javascript
> var db = new Sdb( "localhost", 50000, "sdbadmin", "sdbadmin" )
```

明文密码被暴露在接口上，操作也会被记录在历史文件中，存在被他人窃取密码的风险。因此 SequoiaDB v3.2.6 及以上版本和 v3.4.1 及以上版本提供了密文模式输入密码，且涉及密码的操作也不会被记录在历史文件中，保证了密码无痕。
 
**密码无痕**

为了兼容老版本，连接数据库、创建数据库用户和删除数据库用户等涉及到密码的操作，依旧支持明文密码输入，但这些操作不会被记录在历史文件中，他人无法通过翻找历史记录来窃取密码。

**密文模式**

虽然保证了密码无痕，但明文密码被暴露在接口上还是不安全，因此建议用户使用密文模式。

当用户创建一个数据库用户之后，可以使用 sdbpasswd 工具把该数据库用户的用户名和密码以密文方式保存在密文文件中。在密文模式下，涉及到密码的操作，用户可以不需要提供明文密码，只需要提供保存有数据库用户名和密码的密文文件。

1. 创建密文文件，添加用户名为 sdbadmin，密码为 sdbadmin 的用户信息

   ```lang-bash
   $ sdbpasswd --adduser sdbadmin --password sdbadmin --file ./passwd
   ```

2. 连接数据库

   ```lang-javascript
   > var cipherUser = new CipherUser( "sdbadmin" ).cipherFile( "./passwd" )
   > var db = new Sdb( "localhost", 50000, cipherUser )
   ```

其他数据库工具在连接数据库时同样可以使用密文模式，生成密文文件可参考 [sdbpasswd][passwd]。

使用密文模式连接数据库、创建数据库用户和删除数据库用户，可参考 [new Sdb][sdb]、[createUsr()][createUsr] 和 [dropUsr()][dropUsr]。


[^_^]:
     本文使用的所有链接和引用
[authentication_algorithm]:manual/Distributed_Engine/Maintainance/Security/authentication_algorithm.md
[network_security]:manual/Distributed_Engine/Maintainance/Security/network_security.md
[auditlog]:manual/Distributed_Engine/Maintainance/DiagLog/auditlog.md
[passwd]:manual/Distributed_Engine/Maintainance/Mgmt_Tools/sdbpasswd.md
[sdb]:manual/Manual/Sequoiadb_Command/Sdb/Sdb.md
[createUsr]:manual/Manual/Sequoiadb_Command/Sdb/createUsr.md
[dropUsr]:manual/Manual/Sequoiadb_Command/Sdb/dropUsr.md