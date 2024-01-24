##名称##

snapshot - 获取快照

##语法##

**db.snapshot(\<snapType\>,[cond],[sel],[sort])**

**db.snapshot(\<snapType\>,[SdbSnapshotOption])**

##类别##

Sdb

##描述##

该函数用于获取指定快照，查看当前系统状态。

##参数##

| 参数名 			| 类型 	| 描述 		| 是否必填 |
| ------ 			| ------ 	| ------ 	| ------   |
| snapType 			| macro   	| 需要获取的快照，取值可参考[快照类型][snapshot] | 是 |
| cond 				| object    | 设置匹配条件以及[命令位置参数][location] 	| 否 |
| sel 				| object | 选择返回字段名，为 null 时返回所有的字段名 	| 否 |
| sort 				| object | 对返回的记录按选定的字段排序，取值如下：<br>1：升序<br>-1：降序 | 否 |
| SdbSnapshotOption	| object | 使用一个对象指定快照查询参数，使用方法可参考 [SdbSnapshotOption][shotOption] | 否 |

> **Note:**
>
>* sel 参数是一个 json 结构，如：{字段名:字段值}，字段值一般指定为空串。sel 中指定的字段名在记录中存在，设置字段值不生效；不存在则返回 sel 中指定的字段名和字段值。
>* 记录中字段值类型为数组，我们可以在 sel 中指定该字段名，用"."操作符加上双引号("")来引用数组元素。

##返回值##

函数执行成功时，将返回一个 SdbCursor 类型的对象。

函数执行失败时，将抛异常并输出错误信息。

##错误##

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取[错误码][error_code]。更多错误处理可以参考[常见错误处理指南][faq]。

##版本##

v2.0 及以上版本

##示例##

* 指定 snapType 的值为 SDB_SNAP_CONTEXTS 

    ```lang-javascript
    > db.snapshot( SDB_SNAP_CONTEXTS )
    {
      "SessionID": "vmsvr1-cent-x64-1:11820:22",
      "Contexts": [
        {
          "ContextID": 8,
          "Type": "DUMP",
          "Description": "BufferSize:0",
          "DataRead": 0,
          "IndexRead": 0,
          "QueryTimeSpent": 0,
          "StartTimestamp": "2013-12-28-16.07.59.146399"
        }
      ]
    }
    {
      "SessionID": "vmsvr1-cent-x64-1:11830:22",
      "Contexts": [
        {
          "ContextID": 6,
          "Type": "DUMP",
          "Description": "BufferSize:0",
          "DataRead": 0,
          "IndexRead": 0,
          "QueryTimeSpent": 0,
          "StartTimestamp": "2013-12-28-16.07.59.147576"
        }
      ]
    }
    {
      "SessionID": "vmsvr1-cent-x64-1:11840:23",
      "Contexts": [
        {
          "ContextID": 7,
          "Type": "DUMP",
          "Description": "BufferSize:0",
          "DataRead": 0,
          "IndexRead": 0,
          "QueryTimeSpent": 0,
          "StartTimestamp": "2013-12-28-16.07.59.148603"
        }
      ]
    }
    ```

* 通过组名或组 ID 查询某个复制组的快照信息

    ```lang-javascript
    > db.snapshot( SDB_SNAP_CONTEXTS, { GroupName:'data1' } )
    > db.snapshot(SDB_SNAP_CONTEXTS,{GroupID:1000})
    {
      "SessionID": "vmsvr1-cent-x64-1:11820:22",
      "Contexts": [
        {
          "ContextID": 11,
          "Type": "DUMP",
          "Description": "BufferSize:0",
          "DataRead": 0,
          "IndexRead": 0,
          "QueryTimeSpent": 0,
          "StartTimestamp": "2013-12-28-16.13.57.864245"
        }
      ]
    }
    {
      "SessionID": "vmsvr1-cent-x64-1:11840:23",
      "Contexts": [
        {
          "ContextID": 10,
          "Type": "DUMP",
          "Description": "BufferSize:0",
          "DataRead": 0,
          "IndexRead": 0,
          "QueryTimeSpent": 0,
          "StartTimestamp": "2013-12-28-16.13.57.865103"
        }
      ]
    }
    ```

* 通过指定组名、主机名和服务名，或指定组 ID 和节点 ID查询某个节点的快照信息

    ```lang-javascript
    > db.snapshot( SDB_SNAP_CONTEXTS, { GroupName: 'data1', HostName: "vmsvr1-cent-x64-1", svcname: "11820" } )
    > db.snapshot(SDB_SNAP_CONTEXTS,{GroupID:1000,NodeID:1001})
    {
      "SessionID": "vmsvr1-cent-x64-1:11820:22",
      "Contexts": [
        {
          "ContextID": 11,
          "Type": "DUMP",
          "Description": "BufferSize:0",
          "DataRead": 0,
          "IndexRead": 0,
          "QueryTimeSpent": 0,
          "StartTimestamp": "2013-12-28-16.13.57.864245"
        }
      ]
    }
    ```

* 通过指定主机名和服务名查询某个节点的快照信息

    ```lang-javascript
    > db.snapshot( SDB_SNAP_CONTEXTS, { HostName: "ubuntu-200-043", svcname: "11820" } )
    {
      "NodeName": "ubuntu-200-043:11820",
      "SessionID": 18,
      "Contexts": [
        {
          "ContextID": 31,
          "Type": "DUMP",
          "Description": "IsOpened:1,HitEnd:0,BufferSize:0",
          "DataRead": 0,
          "IndexRead": 0,
          "QueryTimeSpent": 0,
          "StartTimestamp": "2016-10-27-17.53.45.042061"
        }
      ]
    }
    ```

* 返回未在 coord 聚集前的原始数据

    ```lang-javascript
    > db.snapshot( SDB_SNAP_DATABASE, { RawData: true } ,{ NodeName: null, GroupName: null, TotalDataRead: null } )
    {
      "NodeName": "ubuntu1604-yt:30000",
      "GroupName": "SYSCatalogGroup",
      "TotalDataRead": 276511
    }
    {
      "NodeName": "ubuntu1604-yt:20000",
      "GroupName": "db1",
      "TotalDataRead": 16542209
    }
    {
      "NodeName": "ubuntu1604-yt:40000",
      "GroupName": "db2",
      "TotalDataRead": 959
    }
    Return 3 row(s).
    ```

[^_^]:
     本文使用的所有引用及链接
[snapshot]:manual/Manual/Snapshot/Readme.md
[location]:manual/Manual/Sequoiadb_Command/location.md
[shotOption]:manual/Manual/Sequoiadb_Command/AuxiliaryObjects/SdbSnapshotOption.md
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md