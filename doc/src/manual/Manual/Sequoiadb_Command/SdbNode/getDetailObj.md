##名称##

getDetailObj - 获取当前节点信息

##语法##

**node.getDetailObj()**

##类别##

SdbNode

##描述##

获取当前 SdbNode 节点的基本信息。包括节点所属组,节点启动的服务等。

##参数##

NULL

##返回值##

函数执行成功时，返回当前节点详细信息，其类型为 BSONObj。

函数执行失败时，将抛异常并输出错误信息。


##错误##

当抛出异常时,可以通过[getLastErrMsg()](manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md)获取错误信息或通过[getLastError()](manual/Manual/Sequoiadb_Command/Global/getLastError.md)获取错误码。
关于错误处理可以参考[常见错误处理指南](manual/FAQ/faq_sdb.md)。

##版本##

v3.2.8及以上版本、v3.4.2及以上版本、v5.0.2及以上版本

##示例##

* 获取 node 节点的信息

```lang-javascript
> node.getDetailObj()
{
  "HostName": "localhost",
  "Status": 1,
  "dbpath": "/opt/sequoiadb/database/data/11830/",
  "Service": [
    {
      "Type": 0,
      "Name": "11830"
    },
    {
      "Type": 1,
      "Name": "11831"
    },
    {
      "Type": 2,
      "Name": "11832"
    }
  ],
  "Location": "GuangZhou",
  "NodeID": 1002,
  "GroupID": 1001,
  "GroupName": "group1"
}
```
