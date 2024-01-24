CipherUser 对象，该对象是为了保存用户名、密文文件路径、用户加密令牌和集群名。可直接用于 new Sdb、createUsr 和 dropUsr 接口中，代替原有的用户名和密码的输入，但是密码是从指定路径的密文文件中读取的。关于在密文文件中生成用户名和密文，详细内容可参考 [sdbpasswd](manual/Distributed_Engine/Maintainance/Mgmt_Tools/sdbpasswd.md)。

##语法##

**CipherUser( \<username\> )[.token( \<token\> )]
</br>&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp; [.clusterName( \<clusterName\> )]
</br>&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp; [.cipherFile( \<cipherFile\> )]**

##方法##

**CipherUser( \<username\> )**

创建 CipherUser 对象

| 参数名   | 参数类型 | 默认值 | 描述   | 是否必填 |
| -------- | -------- | ------ | ------ | -------- |
| username | string   | ---    | 用户名 | 是       |

**token( \<token\> )**

设置加密令牌

| 参数名 | 参数类型 | 默认值 | 描述     | 是否必填 |
| ------ | -------- | ------ | -------- | -------- |
| token  | string   | ---    | 加密令牌 | 是       |

**clusterName( \<clusterName\> )**

设置集群名

| 参数名      | 参数类型 | 默认值 | 描述   | 是否必填 |
| ----------- | -------- | ------ | ------ | -------- |
| clusterName | string   | ---    | 集群名 | 是       |

**cipherFile( \<cipherFile\> )**

设置密文文件路径

| 参数名     | 参数类型 | 默认值 | 描述         | 是否必填 |
| ---------- | -------- | ------ | ------------ | -------- |
| cipherFile | string   | ---    | 密文文件路径 | 是       |

>Note:
>
> 如果不调用该方法设置密文文件路径，则密文文件路径默认为 `~/sequoiadb/passwd`。

**getUsername()**

获取用户名

**getClusterName()**

获取集群名

**getCipherFile()**

获取密文文件路径

**toString()**

把 CipherUser 对象以字符串的形式输出

##错误##

如果出错则抛异常，并输出错误信息，可以通过 [getLastErrMsg()](manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md) 获取错误信息或通过 [getLastError()](manual/Manual/Sequoiadb_Command/Global/getLastError.md) 获取错误码。
关于错误处理可以参考[常见错误处理指南](manual/FAQ/faq_sdb.md)。

常见错误可参考[错误码](manual/Manual/Sequoiadb_error_code.md)。

##示例##

1. 创建 CipherUser 对象

   ```lang-javascript
   > var a = CipherUser( "sdbadmin" )
   ```

2. 设置集群名

   ```lang-javascript
   > a.clusterName( "c1" )
   sdbadmin@c1:/root/sequoiadb/passwd
   ```

3. 获取用户名

   ```lang-javascript
   > a.getUsername()
   sdbadmin
   ```

4. 把 User 对象以字符串的形式输出

   ```lang-javascript
   > a.toString()
   sdbadmin@c1:/root/sequoiadb/passwd
   ```
