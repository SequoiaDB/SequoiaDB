
该对象用于指定快照查询参数，可以指定的参数包括指定选择条件、返回的字段名、排序的情况、快照参数等。


##语法##

**SdbSnapshotOption[.cond(\<cond\>)]  
&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;[.sel(\<sel\>)]  
&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;[.sort(\<sort\>)]  
&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;[.options(\<options\>)]  
&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;[.skip(\<skipNum\>)]  
&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;[.limit(\<retNum\>)]**

**SdbSnapshotOption[.cond(\<cond\>)][.skip(\<skipNum\>)][.limit(\<retNum\>)]**

**SdbSnapshotOption[.cond(\<cond\>)].options(\<options\>)**

##方法##

###cond(\<cond\>)###

选择需要返回的节点或复制组的快照信息

| 参数名 			| 类型 	| 描述 		| 是否必填 |
| ------ 			| ------ 	| ------ 	| ------   |
|cond |	object| 选择条件，只返回 cond 字段指定的节点或复制组的快照信息，为 null 时，返回整个集群的快照信息 | 是 |

###sel(\<sel\>)###

选择返回记录时需要返回的字段名

| 参数名 			| 类型 	| 描述 		| 是否必填 |
| ------ 			| ------ 	| ------ 	| ------   |
|sel     |object | 选择返回字段名，为 null 时，返回所有的字段名 | 是 |

> **Note：**
>
>* sel 参数是一个 json 结构，如：{字段名:字段值}，字段值一般指定为空串。sel 中指定的字段名在记录中存在，设置字段值不生效；不存在则返回 sel 中指定的字段名和字段值。
>* 记录中字段值类型为数组的，我们可以在 sel 中指定该字段名，用"."操作符加上双引号("")来引用数组元素。

###sort(\<sort\>)###

指定结果集按指定字段名排序

| 参数名 			| 类型 	| 描述 		| 是否必填 |
| ------ 			| ------ 	| ------ 	| ------   |
|sort |	object| 指定结果集按指定字段名排序的情况，如：{"name":1,"age":-1}，取值如下：<br> 1：代表升序 <br> -1：代表降序  <br>如果不设定 sort 则表示不对结果集做排序 | 是 |

###options(\<options\>)###

指定快照参数

| 参数名 			| 类型 	| 描述 		| 是否必填 |
| ------ 			| ------ 	| ------ 	| ------   |
| options   | object | 指定快照参数，因不同快照类型而异，在对应[快照类型][snapshot]查看选项及示例  | 是 |

####options选项####
| 参数名  | 参数类型 | 对应快照 | 描述 | 是否必填 |
| ------  | -------- | -------- | ---- | -------- |
| Mode    |  string  | [配置快照][SNAP_CONFIGS] | 指定返回配置的模式，默认为"run"，取值如下：<br>"run"：显示当前运行时配置信息 <br>"local"：显示配置文件中配置信息<br>如{"Mode":"local"}  | 否 |
| Expand  |  boolean/string  | [配置快照][SNAP_CONFIGS]  | 是否扩展显示用户未配置的配置项，默认为 true，如 {"Expand":false} | 否 |
| ShowError | string | ALL | 指定是否返回错误信息，默认为"show"，取值如下： <br>"show"：显示错误信息 <br>"only"：只显示错误信息，不显示其他快照信息 <br>"ignore"：不显示错误信息 <br>如 { "ShowError: "only" }  | 否 |
| ShowErrorMode | string | ALL | 指定返回错误信息的格式，默认为"aggr"，取值如下： <br>"aggr"：错误信息聚合为一条记录显示<br>"flat"：一个错误节点对应一条记录显示 <br>如 {"ShowErrorMode":"flat"}  | 否 |
| viewHistory | boolean/string | [查询快照][SNAP_QUERIES]、[锁等待快照][SNAP_LOCKWAITS]、[闩锁等待快照][SNAP_LATCHWAITS]  | 显示快照中保留的慢查询相关的历史信息，默认为 false<br>当前暂时只支持显示查询快照、锁等待快照和闩锁等待快照的历史信息，如 { "viewHistory": true }   | 否 |


> **Note：**
>
> - 参数 ShowError 和 ShowErrorMode 仅当用户在协调节点执行快照时生效。
> - 当参数 ShowError 取值为"ignore"时，参数 ShowErrorMode 不生效。
> - 参数 ShowErrorMode 对[操作系统快照][SNAP_SYSTEM]和[数据库快照][SNAP_DATABASE]不生效，快照返回的错误信息均聚合为一条记录。

###skip(\<skipNum\>)###

指定结果集的返回记录起始值

| 参数名 			| 参数类型 	| 描述 		| 是否必填 |
| ------ 			| ------ 	| ------ 	| ------   |
| skipNum | int32 | 自定义从结果集哪条记录开始返回，默认值为 0，表示从第一条记录开始返回 | 是 |

> **Note：**
>
> 如果想从结果集的第三条记录开始返回，可设置 skipNum 的值为 2。

###limit(\<retNum\>)###

指定返回结果集的记录条数


| 参数名 			| 参数类型 	| 描述 		| 是否必填 |
| ------ 			| ------ 	| ------ 	| ------   |
| retNum | int32 | 自定义返回结果集的记录条数，默认值为 -1，表示返回从 skipNum 参数指定位置至结果集结束位置的所有记录 | 是 |

> **Note：**
>
>如果想返回结果集的前五记录，可设置 retNum 的值为 5。

##返回值##

快照命令执行成功则返回自身，类型为 SdbSnapshotOption。

##错误##

[错误码][error_code]


##示例##

* 通过组名或组 ID 查询某个复制组的快照信息 

   ```lang-javascript
   > var option = new SdbSnapshotOption().cond({GroupName:'data1'})
   > db.snapshot(SDB_SNAP_CONTEXTS,option)
   ```

   输出结果如下：

   ```lang-json
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

* 通过“组名+主机名+服务名”或“组 ID+节点 ID”查询某个节点的快照信息 

   ```lang-javascript
   > var option = new SdbSnapshotOption().cond({GroupName:'data1',HostName:"vmsvr1-cent-x64-1",svcname:"11820"})
   > db.snapshot(SDB_SNAP_CONTEXTS,option)
   ```

   输出结果如下：

   ```lang-json
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

* 通过“主机名+服务名”查询某个节点的快照信息 

   ```lang-javascript
   > var option = new SdbSnapshotOption().cond({HostName:"ubuntu-200-043",ServiceName:"11820"})
   > db.snapshot(SDB_SNAP_CONTEXTS,option)
   ```

   输出结果如下：

   ```lang-json
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

* 查看数据组 db1 中数据节点 20000 上配置文件中的配置信息并指定快照参数 

   ```lang-javascript
   > var option = new SdbSnapshotOption().cond({GroupName:'db1',ServiceName:'20000'}).options({"mode":"local","expand":false})
   > db.snapshot(SDB_SNAP_CONFIGS,option)
   ```

   输出结果如下：

   ```lang-json
	{
  	"NodeName": "ubuntu-zwb:20000",
  	"dbpath": "/home/sequoiadb/20000/",
  	"ServiceName": "20000",
  	"diaglevel": 3,
  	"role": "data",
  	"catalogaddr": "ubuntu-zwb:30003,ubuntu-zwb:30013,ubuntu-zwb:30023",
  	"perfstat": "FALSE",
  	"businessname": "yyy",
  	"clustername": "xxx"
	}
	Return 1 row(s).
	```

- 查看历史查询记录

   ```lang-javascript
   > db.snapshot(SDB_SNAP_QUERIES, new SdbSnapshotOption().options({"viewHistory":true}))
   ```

   输出结果如下：

   ```lang-json
   {
     "NodeID": [
       2,
       4
     ],
     "StartTimestamp": "2020-06-12-11.02.27.429347",
     "EndTimestamp": "2020-06-12-11.02.27.904392",
     "TID": 10107,
     "OpType": "QUERY",
     "Name": "sbtest1.sbtest6",
     "QueryTimeSpent": 0,
     "ReturnNum": 0,
     "TotalMsgSent": 1,
     "LastOpInfo": "Collection:sbtest1.sbtest6, Matcher:{ \"id\": { \"$et\": 5014 } }, Selector:{}, OrderBy:{ \"id\": 1 }, Hint:{ \"\": \"PRIMARY\" }, Skip:0, Limit:-1, Flag:0x00000200(512)",
     "MsgSentTime": 0.046,
     "RemoteNodeWaitTime": 0,
     "ClientInfo": {
       "ClientTID": 13971,
       "ClientHost": "192.168.56.101"
     },
     "RelatedNode": [
       1002
     ]
   }
   ```

[^_^]:
    本文使用的所有引用及链接
[snapshot]:manual/Manual/Snapshot/Readme.md
[SNAP_CONFIGS]:manual/Manual/Snapshot/SDB_SNAP_CONFIGS.md
[SNAP_QUERIES]:manual/Manual/Snapshot/SDB_SNAP_QUERIES.md
[SNAP_LOCKWAITS]:manual/Manual/Snapshot/SDB_SNAP_LOCKWAITS.md
[SNAP_LATCHWAITS]:manual/Manual/Snapshot/SDB_SNAP_LATCHWAITS.md
[SNAP_SYSTEM]:manual/Manual/Snapshot/SDB_SNAP_SYSTEM.md
[SNAP_DATABASE]:manual/Manual/Snapshot/SDB_SNAP_DATABASE.md
[configuration]:manual/Manual/Database_Configuration/configuration_parameters.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
