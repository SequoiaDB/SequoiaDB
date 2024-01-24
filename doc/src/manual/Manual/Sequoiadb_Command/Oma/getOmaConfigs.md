
##名称##

getOmaConfigs - 获取 sdbcm 的配置信息。

##语法##

**oma.getOmaConfigs([confFile])**

##类别##

Oma

##描述##

获取 sdbcm 的配置信息。

##参数##

* `confFile` ( *String*， *选填* )

    sdbcm 的配置文件，若不指定该参数，默认使用[getOmaConfigFile()](manual/Manual/Sequoiadb_Command/Oma/getOmaConfigFile.md)指定的配置文件。

##返回值##

成功：返回 BSONObj 对象，该对象包含 sdbcm 的配置信息。

失败：抛出异常。

##错误##

`getOmaConfigs()`函数常见异常如下：

| 错误码 | 错误类型 | 描述 | 解决方法 |
| ------ | ------ | --- | ------ |
| -4 | SDB_FNE | 文件不存在 | 确认输入的 sdbcm 配置文件是否存在。	|

当异常抛出时，可以通过 [getLastError()][getLastError] 获取[错误码][error_code]，或通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息。 可以参考[常见错误处理指南][faq]了解更多内容。

##版本##

v1.0 及以上版本。

##示例##

1. 打印 sdbcm 配置文件。

	```lang-javascript
	> Oma.getOmaConfigs()
	{
  		"AutoStart": true,
		"DiagLevel": 3,
  		"EnableWatch": "TRUE",
  		"IsGeneral": "TRUE",
  		"OMAddress": "rhel64-test8:11785",
  		"RestartCount": 5,
  		"RestartInterval": 0,
  		"defaultPort": "11790",
  		"rhel64-test8_Port": "11790"
	}
 	```

2. 获取 sdbcm 配置文件。

	```lang-javascript
	> var ret = Oma.getOmaConfigs()
	> var obj = ret.toObj()
	> println(obj["DiagLevel"])
	3
	```


[^_^]:
    本文使用的所有引用及链接
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md