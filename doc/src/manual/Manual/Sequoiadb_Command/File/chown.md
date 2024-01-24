##名称##

chown - 设置文件的所有者

##语法##

**File.chown(\<filepath\>,\<options\>,\[recursive\])**

##类别##

File

##描述##

设置文件的所有者。

##参数##

| 参数名    | 参数类型 | 默认值 | 描述                   | 是否必填 |
| --------- | -------- | ------ | ---------------------- | -------- |
| filename  | string   | ---    | 文件路径               | 是       |
| options   | JSON     | ---    | 指定用户名或者用户组名 | 是       |
| recursive | boolean  | false  | 是否递归处理           | 否       |

options 参数详细说明如下：

| 属性名    | 值类型 | 描述     | 是否必填 | 
| --------- | ------ | -------- | -------- |
| username  | string | 用户名   | 否       |
| groupname | string | 用户组名 | 否       |


> Note:

> username 和 groupname 两个参数必须指定其中一个参数

##返回值##

无返回值。

##错误##

如果出错则抛异常，并输出错误信息，可以通过[getLastErrMsg()](manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md)获取错误信息或通过[getLastError()](manual/Manual/Sequoiadb_Command/Global/getLastError.md)获取错误码。
关于错误处理可以参考[常见错误处理指南](manual/FAQ/faq_sdb.md)。

常见错误可参考[错误码](manual/Manual/Sequoiadb_error_code.md)。

##版本##

v3.2 及以上版本

##示例##

设置文件的所有者

```lang-javascript
> File.chown( "/opt/sequoiadb/file.txt", { "username": "sequoiadb", "groupname": "SequoiadDB" }, false )
```