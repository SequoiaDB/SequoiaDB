
##名称##

addAOmaSvcName - 将目标机器为 sdbcm 设置的服务端口号写到该 sdbcm 的配置文件中。

##语法##

**Oma.addAOmaSvcName(\<hostname\>,\<svcname\>,[isReplace],[confFile])**

##类别##

Oma

##描述##

将目标机器为 sdbcm 设置的服务端口号写到该 sdbcm 的配置文件中，设置成功后，在目标机器上重启 sdbcm 就可以使用新的服务端口号启动 sdbcm 服务。

##参数##

* `hostname` ( *String*， *必填* )

	目标机器主机名。

* `svcname` ( *Int | String*， *必填* )

	节点端口号，必须是未使用的端口号。

* `isReplace` ( *Bool*， *选填* )

	是否替换配置文件。

* `configFile` ( *String*， *选填* )

	配置文件路径，当不填时使用默认的配置文件。

##返回值##

成功：无。  

失败：抛出异常。

##错误##

`addAOmaSvcName()`函数常见异常如下：

| 错误码 | 错误类型 | 描述 | 解决方法 |
| ------ | ------ | --- | ------ |
| -4     | SDB_FNE| 文件不存在。| 检查配置文件路径是否正确。 |


当异常抛出时，可以通过[getLastError()](manual/Manual/Sequoiadb_Command/Global/getLastError.md)获取[错误码](manual/Manual/Sequoiadb_error_code.md)，
或通过[getLastErrMsg()](manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md)获取错误信息。
可以参考[常见错误处理指南](manual/FAQ/faq_sdb.md)了解更多内容。

##版本##

v2.0及以上版本。

##示例##

1. 为目标机器 sdbserver1 上的 sdbcm 配置一个新的服务端口号 "11780" 到其 sdbcm 的配置文件中。

    ```lang-javascript
    > var oma = new Oma( "sdbserver1", 11790 )
    > oma.addAOmaSvcName( "sdbserver1", 11780, false)
    ```

2. 查看目标机器上 sdbcm 的配置文件 sdbcm.conf ，会发现多出如下一行：

    ```lang-javascript
    sdbserver1_Port=11780
    ```