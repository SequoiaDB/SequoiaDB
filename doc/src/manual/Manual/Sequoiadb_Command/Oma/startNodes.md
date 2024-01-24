
##名称##

startNodes - 通过服务端口启动节点

##语法##

**oma.startNodes(\<svcname\>)**

**oma.startNodes([\<svcname\>,...])**

##类别##

Oma

##描述##

启动目标集群控制器（sdbcm）所在的机器中的指定节点。


##参数##

* `svcname` ( *Int | String*， *必填* )

	节点端口号。

##返回值##

成功：无。

失败：抛出异常。

##错误##

`startNodes()`函数常见异常如下：

| 错误码 | 错误类型 | 描述 | 解决方法 |
| ------ | ------ | --- | ------ |
| -264 | SDB_COORD_NOT_ALL_DONE| 部分节点未返回成功 | 使用getLastErrObj()查看是哪些节点错误 |
| -146 | SDBCM_NODE_NOTEXISTED | 节点不存在 | 检查节点的配置文件 |
| -6   | SDB_INVALIDARG | 非法输入参数 | 检查svcname参数的值 |

当异常抛出时，可以通过 [getLastError()][getLastError] 获取[错误码][error_code]，或通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息。 可以参考[常见错误处理指南][faq]了解更多内容。

##版本##

v3.0.2 及以上版本

##示例##

1. 启动一个端口号为 11810 的节点

 	```lang-javascript
	> var oma = new Oma()
	> oma.startNodes( 11810 )
 	```

2. 启动一个端口号为 11820 的节点

 	```lang-javascript
	> var oma = new Oma()
	> oma.startNodes( "11820" )
    ```

3. 同时启动端口号为 11810、11820、11830 的节点

 	```lang-javascript
	> var oma = new Oma()
	> oma.startNodes( [ 11810, 11820, 11830 ] )
 	```

[^_^]:
    本文使用的所有引用及链接
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md