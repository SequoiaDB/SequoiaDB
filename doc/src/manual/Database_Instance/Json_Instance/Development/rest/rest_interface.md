
##创建集合空间##

|          | 说明                                                         | 示例                                                         |
| -------- | ------------------------------------------------------------ | ------------------------------------------------------------ |
| 请求头   | 同通用请求头                                                 |                                                              |
| 请求内容 | cmd：create collectionspace<br>name：集合空间名字            | cmd=create collectionspace&name=sample                       |
| 响应头   | 同通用响应头                                                 |                                                              |
| 响应内容 | {<br>errno: 返回值，0 表示成功，其他为失败<br>description: 失败时的错误描述<br>} | [{ "errno": -33, "description": "Collection space already exists" }] |


##删除集合空间##

|          | 说明                                     | 示例                             |
|----------|------------------------------------------|----------------------------------|
| 请求头   | 同通用请求头                             |                                  |
| 请求内容 | cmd：drop collectionspace<br>name：集合空间名字 <br>options：选项（可选参数，可不填） | 1. cmd=drop collectionspace&name=sample <br>2. cmd=drop collectionspace&name=sample&options={EnsureEmpty:true} |
| 响应头   | 同通用响应头                             |                                  |
| 响应内容 | {<br>errno: 返回值，0表示成功，其他为失败<br>description: 失败时的错误描述 <br>} | [{ "errno": 0 }] |

##创建集合##

|          | 说明                                     | 示例                              |
|----------|------------------------------------------|-----------------------------------|
| 请求头   | 同通用请求头                             |                                   |
| 请求内容 | cmd：create collection<br>name：集合的全称（集合空间.集合）<br>options：选项（可选参数，可不填） | cmd=create collection&name=sample.employee |
| 响应头   | 同通用响应头                             |                                   |
| 响应内容 | {<br>errno: 返回值，0表示成功，其他为失败<br>description: 失败时的错误描述<br>} | [{ "errno": 0 }] |

##删除集合##

|          | 说明                                     | 示例                            |
|----------|------------------------------------------|---------------------------------|
| 请求头   | 同通用请求头                             |                                 |
| 请求内容 | cmd：drop collection<br>name：集合的全称（集合空间.集合） | cmd=drop collection&name=sample.employee |
| 响应头   | 同通用响应头                             |                                 |
| 响应内容 | {<br>errno: 返回值，0表示成功，其他为失败<br>description: 失败时的错误描述<br>} | [{ "errno": 0 }] |

##插入数据##

|          | 说明                                                         | 示例                                                         |
| -------- | ------------------------------------------------------------ | ------------------------------------------------------------ |
| 请求头   | 同通用请求头                                                 |                                                              |
| 请求内容 | cmd：insert<br>name：集合的全称（集合空间.集合）<br>insertor：待插入的数据 <br>flag：标志位（可选参数，可不填） | cmd=insert&name=sample.employee&insertor={"age":12,"name":"hello"}&flag=SDB_INSERT_CONTONDUP |
| 响应头   | 同通用响应头                                                 |                                                              |
| 响应内容 | {<br>errno：返回值，0表示成功，其他为失败<br>description：失败时的错误描述<br>}<br>{<br>返回成功插入的记录数信息<br>}<br>| [{ "errno": 0 },{ "InsertedNum": 0, "DuplicatedNum": 1}]                                             |

> **Note:**
>
> flag 不支持同时指定多个取值，可选取值如下：
> - SDB_INSERT_CONTONDUP：遇到索引键重复则跳过，并继续插入
> - SDB_INSERT_REPLACEONDUP：遇到索引键重复则使用新纪录替换现有记录，并继续插入
> - SDB_INSERT_CONTONDUP_ID：遇到 $id 索引键重复则跳过，并继续插入
> - SDB_INSERT_REPLACEONDUP_ID：遇到 $id 索引键重复则使用新纪录替换现有记录，并继续插入
>
> 成功插入的记录数信息。字段说明如下：
> - InsertedNum：成功插入的记录数，不包含替代和忽略的记录
> - DuplicatedNum：因重复键冲突被忽略或替代的记录数
> - LastGenerateID：自增字段的值 (仅在集合包含自增字段时显示)

##查询数据##

|          | 说明                                                         | 示例                                                         |
| -------- | ------------------------------------------------------------ | ------------------------------------------------------------ |
| 请求头   | 同通用请求头                                                 |                                                              |
| 请求内容 | cmd：query<br>name：集合的全称（集合空间.集合）<br>sort：待排序字段名（可选参数，可不填）<br>selector：查询结果列（可选参数，可不填）<br>skip：跳过多少行（可选参数，可不填）<br>returnnum：最大返回条数（可选参数，可不填）<br>filter：查询条件（可选参数，可不填） | cmd=query&name=sample.employee&sort={"name":1}&skip=0&returnnum=1&filter={"name":"hello"} |
| 响应头   | 同通用响应头                                                 |                                                              |
| 响应内容 | {<br>errno: 返回值，0表示成功，其他为失败<br>description: 失败时的错误描述<br>}<br>{<br>返回表里的记录<br>}<br>... | [{ "errno": 0 },{ "_id":{ "$oid":"54def72f0d8737161d9d6934" },"age":12,"name":"hello" }] |

##查询更新数据##

|          | 说明                                                         | 示例                                                         |
| -------- | ------------------------------------------------------------ | ------------------------------------------------------------ |
| 请求头   | 同通用请求头                                                 |                                                              |
| 请求内容 | cmd：queryandupdate<br>name：集合的全称（集合空间.集合）<br>updator：更新操作<br>sort：待排序字段名（可选参数，可不填）<br>selector：查询结果列（可选参数，可不填）<br>skip：跳过多少行（可选参数，可不填）<br>returnnum：最大返回条数（可选参数，可不填）<br>filter：查询条件（可选参数，可不填）<br>returnnew：是否返回更新后记录（可选参数，可不填）<br>flag：标志位（可选参数，可不填） | cmd=queryandupdate&name=sample.employee&updator={$set:{"age":100}}&filter={"name":"hello"}&returnnew=true |
| 响应头   | 同通用响应头                                                 |                                                              |
| 响应内容 | {<br>errno: 返回值，0表示成功，其他为失败<br>description: 失败时的错误描述<br>}<br>{<br>返回表里的记录<br>}<br>... | [{ "errno": 0 },{ "_id":{ "$oid":"54def72f0d8737161d9d6934" },"age":100,"name":"hello" }] |

> **Note:**
>
> flag 取值如下：  
> - SDB_QUERY_FORCE_HINT：强制使用指定的提示进行查询，如果数据库没有与提示匹配的索引，则查询失败  
> - SDB_QUERY_PARALLED：启用并行子查询，每个子查询将对数据的不同部分进行扫描  
> - SDB_QUERY_WITH_RETURNDATA：指定数据在查询响应时返回，以提高性能 
> - SDB_QUERY_KEEP_SHARDINGKEY_IN_UPDATE：指定不过滤更新规则中的分区键


##查询删除数据##

|          | 说明                                                         | 示例                                                         |
| -------- | ------------------------------------------------------------ | ------------------------------------------------------------ |
| 请求头   | 同通用请求头                                                 |                                                              |
| 请求内容 | cmd：queryandremove<br>name：集合的全称（集合空间.集合）<br>sort：待排序字段名（可选参数，可不填）<br>selector：查询结果列（可选参数，可不填）<br>skip：跳过多少行（可选参数，可不填）<br>returnnum：最大返回条数（可选参数，可不填）<br>filter：查询条件（可选参数，可不填） | cmd=queryandremove&name=sample.employee&filter={"name":"hello"} |
| 响应头   | 同通用响应头                                                 |                                                              |
| 响应内容 | {<br>errno: 返回值，0表示成功，其他为失败<br>description: 失败时的错误描述<br>}<br>{<br>返回表里的记录<br>}<br>... | [{ "errno": 0 },{ "_id":{ "$oid":"54def72f0d8737161d9d6934" },"age":12,"name":"hello" }] |

##删除记录##

|          | 说明                                                         | 示例                                                     |
| -------- | ------------------------------------------------------------ | -------------------------------------------------------- |
| 请求头   | 同通用请求头                                                 |                                                          |
| 请求内容 | cmd：delete<br>name：集合的全称（集合空间.集合）<br>deletor：删除条件 <br>flag：标志位（可选参数，可不填） | cmd=delete&name=sample.employee&deletor={"name":"hello"}&flag=SDB_DELETE_ONE |
| 响应头   | 同通用响应头                                                 |                                                          |
| 响应内容 | {<br>errno：返回值，0表示成功，其他为失败<br>description：失败时的错误描述<br>}<br>{<br>返回成功删除的记录数信息<br>}<br>| [{ "errno": 0 },{ "DeletedNum": 1 }]                                        |

> **Note:**
>
> flag 取值如下： 
> - SDB_DELETE_ONE：指定只删除第一条匹配记录，默认为删除所有匹配记录
>
> 成功删除的记录数信息，字段说明如下:
> - DeletedNum：成功删除的记录数

##更新记录##

|          | 说明                                                         | 示例                                                         |
| -------- | ------------------------------------------------------------ | ------------------------------------------------------------ |
| 请求头   | 同通用请求头                                                 |                                                              |
| 请求内容 | cmd：update<br>name：集合的全称（集合空间.集合）<br>updator：更新操作<br>filter：更新条件<br>flag：标志位（可选参数，可不填） | cmd=update&name=sample.employee&updator={$set:{"age":100}}&filter={"name":"hello"}&flag=SDB_UPDATE_KEEP_SHARDINGKEY |
| 响应头   | 同通用响应头                                                 |                                                              |
| 响应内容 | {<br>errno：返回值，0表示成功，其他为失败<br>description：失败时的错误描述<br>}<br>{<br>返回成功更新的记录数信息<br>}<br>| [{ "errno": 0 },{ "UpdatedNum": 1, "ModifiedNum": 1, "InsertedNum": 0 }]                                             |

> **Note:**  
> 
> flag 取值如下：  
> - SDB_UPDATE_KEEP_SHARDINGKEY：指定不过滤更新规则中的分区键
> - SDB_UPDATE_ONE：指定只更新第一条匹配记录，默认为更新所有记录
>
> 成功更新的记录数信息，字段说明如下：
> - UpdatedNum：成功更新的记录数，包括匹配但未发生数据变化的记录
> - ModifiedNum：成功更新且发生数据变化的记录数
> - InsertedNum：成功插入的记录数 

##更新或插入记录##

|          | 说明                                                         | 示例                                                         |
| -------- | ------------------------------------------------------------ | ------------------------------------------------------------ |
| 请求头   | 同通用请求头                                                 |                                                              |
| 请求内容 | cmd：upsert<br>name：集合的全称（集合空间.集合）<br>updator：更新操作<br>filter：更新条件（可选参数，可不填）<br> setoninsert：插入数据（可选参数，可不填）<br>flag：标志位（可选参数，可不填） | cmd=upsert&name=sample.employee&updator={$set:{"age":100}}&filter={"name":"hello"}&setoninsert={"sex":"male"}&flag=SDB_UPDATE_KEEP_SHARDINGKEY |
| 响应头   | 同通用响应头                                                 |                                                              |
| 响应内容 | {<br>errno：返回值，0表示成功，其他为失败<br>description：失败时的错误描述<br>}<br>{<br>返回成功更新的记录数信息<br>}<br>| [{ "errno": 0 },{ "UpdatedNum": 0, "ModifiedNum": 0, "InsertedNum": 1 }]                                            |

> **Note:**  
>
> flag 取值如下：
> - SDB_UPDATE_KEEP_SHARDINGKEY：指定不过滤更新规则中的分区键
> - SDB_UPDATE_ONE：指定只更新第一条匹配记录，默认为更新所有记录
>
> 成功更新的记录数信息，字段说明如下：
> - UpdatedNum：成功更新的记录数，包括匹配但未发生数据变化的记录
> - ModifiedNum：成功更新且发生数据变化的记录数
> - InsertedNum：成功插入的记录数

##获取记录数##

|          | 说明                                     | 示例                        |
|----------|------------------------------------------|-----------------------------|
| 请求头   | 同通用请求头                             |                             |
| 请求内容 | cmd：get count<br>name：集合的全称（集合空间.集合）<br>filter：过滤条件（可选） | cmd=get count&name=sample.employee |
| 响应头   | 同通用响应头                             |                             |
| 响应内容 | {<br>errno: 返回值，0 表示成功，其他为失败<br>description: 失败时的错误描述<br>}<br>{<br>Total: 总计数<br>} | [{ "errno": 0 },{ "Total":1 }] |

##修改表属性##

|          | 说明                                                         | 示例                                                         |
| -------- | ------------------------------------------------------------ | ------------------------------------------------------------ |
| 请求头   | 同通用请求头                                                 |                                                              |
| 请求内容 | cmd：alter collection<br>name：集合的全称（集合空间.集合）<br>options：属性 | cmd=alter collection&name=sample.employee&options={ShardingKey:{age:1},ShardingType:"hash"} |
| 响应头   | 同通用响应头                                                 |                                                              |
| 响应内容 | {<br>errno: 返回值，0表示成功，其他为失败<br>description: 失败时的错误描述<br>} | [{ "errno": 0 }] 

##创建自增字段   

|          | 说明                                                         | 示例                                                         |
| -------- | ------------------------------------------------------------ | ------------------------------------------------------------ |
| 请求头   | 同通用请求头                                                 |                                                              |
| 请求内容 | cmd：create autoincrement<br>name：集合的全称（集合空间.集合）<br>options：属性 | 1. 创建一个自增字段：cmd=create autoincrement&name=sample.employee&options={AutoIncrement:{Field:"id"}}<br>2. 创建多个自增字段：cmd=create autoincrement&name=sample.employee&options={AutoIncrement:[{Field:"id"},{Field:"times"}]} |
| 响应头   | 同通用响应头                                                 |                                                              |
| 响应内容 | {<br>errno: 返回值，0 表示成功，其他为失败<br>description: 失败时的错误描述<br>} | [{ "errno": 0 }]     

##删除自增字段##


|          | 说明                                                         | 示例                                                         |
| -------- | ------------------------------------------------------------ | ------------------------------------------------------------ |
| 请求头   | 同通用请求头                                                 |                                                              |
| 请求内容 | cmd：drop autoincrement<br>name：集合的全称（集合空间.集合）<br>options：属性 | 1. 删除一个自增字段：cmd=drop autoincrement&name=sample.employee&options={Field:"id"}<br >2. 删除多个自增字段：cmd=drop autoincrement&name=sample.employee&options={Field:["id","times"]} |
| 响应头   | 同通用响应头                                                 |                                                              |
| 响应内容 | {<br>errno: 返回值，0 表示成功，其他为失败<br>description: 失败时的错误描述<br>} | [{ "errno": 0 }]                                         

##表分区##

|          | 说明                                                         | 示例                                                         |
| -------- | ------------------------------------------------------------ | ------------------------------------------------------------ |
| 请求头   | 同通用请求头                                                 |                                                              |
| 请求内容 | cmd：split<br>name：集合的全称（集合空间.集合）<br>source：源数据组<br>target：目标数据组<br>splitpercent：百分比<br>splitquery：开始条件<br>splitendquery：结束条件 | cmd=split&name=sample.employee&source=group1&target=group2&splitpercent=50 |
| 响应头   | 同通用响应头                                                 |                                                              |
| 响应内容 | {<br>errno: 返回值，0表示成功，其他为失败<br>description: 失败时的错误描述<br>} | [{ "errno": 0 }]                                             |

##列出数据组##

|          | 说明                                      | 示例            |
|----------|-------------------------------------------|-----------------|
| 请求头   | 同通用请求头                              |                 |
| 请求内容 | cmd：list groups                          | cmd=list groups |
| 响应头   | 同通用响应头                              |                 |
| 响应内容 | {<br>errno: 返回值，0 表示成功，其他为失败<br>description: 失败时的错误描述<br>}<br>{<br>返回数据组的内容<br>} | [{ "errno": 0 }] |

##列出序列##

|          | 说明                                      | 示例               |
|----------|-------------------------------------------|--------------------|
| 请求头   | 同通用请求头                              |                    |
| 请求内容 | cmd：list sequences                       | cmd=list sequences |
| 响应头   | 同通用响应头                              |                    |
| 响应内容 | {<br>errno: 返回值，0 表示成功，其他为失败<br>description: 失败时的错误描述<br>}<br>{<br>返回序列的内容<br>} | [{ "errno": 0 }] |

##列出集合空间的集合##

|          | 说明                                      | 示例            |
|----------|-------------------------------------------|-----------------|
| 请求头   | 同通用请求头                              |                 |
| 请求内容 | cmd：list collections in collectionspace<br> name: 集合空间的名字  | cmd=list collections in collectionspace&name=sample |
| 响应头   | 同通用响应头                              |                 |
| 响应内容 | {<br>errno: 返回值，0 表示成功，其他为失败<br>description: 失败时的错误描述<br>}<br>{<br>返回集合空间下所有集合的全名<br>} | [{ "errno": 0 },{ "Name": "sample.employee" }]  |

##获取数据域名##

|          | 说明                                      | 示例            |
|----------|-------------------------------------------|-----------------|
| 请求头   | 同通用请求头                              |                 |
| 请求内容 | cmd：get domain name<br> name: 集合空间的名字               | cmd=get domain name&name=sample |
| 响应头   | 同通用响应头                              |                 |
| 响应内容 | {<br>errno: 返回值，0 表示成功，其他为失败<br>description: 失败时的错误描述<br>}<br>{<br>返回集合空间所属数据域名<br>} | [{ "errno": 0 },{ "Domain": "domain" }]  |

##收集统计信息##

|          | 说明                                      | 示例            |
|----------|-------------------------------------------|-----------------|
| 请求头   | 同通用请求头                              |                 |
| 请求内容 | cmd：analyze<br>options：设定分析模式、指定集合空间以及命令位置参数，可参考 [db.analyze\(\)][analyze] | cmd=analyze<br>cmd=analyze&options={Collection:"sample.employee"}  |
| 响应头   | 同通用响应头                              |                 |
| 响应内容 | {<br>errno: 返回值，0表示成功，其他为失败<br>description: 失败时的错误描述<br>} | [{ "errno": 0 }] |


##查询快照##

|          | 说明                                            | 示例                                                                                  |
|----------|-------------------------------------------------|---------------------------------------------------------------------------------------|
| 请求头   | 同通用请求头                                    |                                                                                       |
| 请求内容 | cmd：snapshot [type]<br>sort：待排序字段名（可选参数，可不填）<br>selector：查询结果列（可选参数，可不填）<br>filter：查询条件（可选参数，可不填）<br>hint：快照参数，格式：{ $options: {\<options\>} }（可选参数，可不填）<br>skip：跳过多少行（可选参数，可不填）<br>returnnum：最大返回条数（可选参数，可不填）<br> 详细说明可参考 [db.snapshot\(\)][snapshot] | cmd=snapshot health&filter={"IsPrimary":false}&selector={"NodeName":null}&sort={"NodeName":-1} |
| 响应头   | 同通用响应头                                    |                                                                                       |
| 响应内容 | {<br>errno: 返回值，0表示成功，其他为失败<br>description: 失败时的错误描述<br>}<br>{<br>返回快照<br>}<br>... | [{ "errno": 0 },{ "NodeName": "ubuntu-test-03:41000" },{ "NodeName": "ubuntu-test-03:40000" }] |

> **Note:** 
>
> [type]指快照类型，取值如下：  
> - contexts：上下文快照  
> - contexts current：当前会话上下文快照  
> - sessions：会话快照  
> - sessions current：当前会话快照  
> - collections：集合快照  
> - collectionspaces：集合空间快照  
> - database：数据库快照  
> - system：系统快照  
> - catalog：编目信息快照  
> - accessplans：访问计划缓存快照  
> - health：节点健康检测快照  
> - configs：配置快照  
> - sequences：序列快照  

##更新配置参数##

|          | 说明                                      | 示例            |
|----------|-------------------------------------------|-----------------|
| 请求头   | 同通用请求头                              |                 |
| 请求内容 | cmd：update config<br>configs：配置参数，包含配置名和配置值<br>options：命令位置参数 <br>详细说明可参考 [db.updateConf\(\)][updateConf] | cmd=update config<br>cmd=update config&configs={'diagnum':27}&options={'svcname':'20000'} |
| 响应头   | 同通用响应头                              |                 |
| 响应内容 | {<br>errno: 返回值，0表示成功，其他为失败<br>description: 失败时的错误描述<br>} | [{ "errno": 0 }] |


##删除配置参数##

|          | 说明                                      | 示例            |
|----------|-------------------------------------------|-----------------|
| 请求头   | 同通用请求头                              |                 |
| 请求内容 | cmd：delete config<br>configs：配置参数，包含配置名和配置值<br>options：命令位置参数 <br>详细说明可参考 [db.deleteConf\(\)][deleteConf] | cmd=update config<br>cmd=update config&configs={'diagnum':1}&options={'svcname':'20000'} |
| 响应头   | 同通用响应头                              |                 |
| 响应内容 | {<br>errno: 返回值，0表示成功，其他为失败<br>description: 失败时的错误描述<br>} | [{ "errno": 0 }] |

##创建序列##

|          | 说明                                      | 示例            |
|----------|-------------------------------------------|-----------------|
| 请求头   | 同通用请求头                              |                 |
| 请求内容 | cmd：create sequence<br>name：序列名<br>options：指定序列属性 <br>详细说明可参考 [db.createSequence\(\)][createSequence] | cmd=create sequence&name=IDSequence&options={"Cycled":true} |
| 响应头   | 同通用响应头                              |                 |
| 响应内容 | {<br>errno: 返回值，0表示成功，其他为失败<br>description: 失败时的错误描述<br>} | [{ "errno": 0 }] |

##删除序列##

|          | 说明                                      | 示例            |
|----------|-------------------------------------------|-----------------|
| 请求头   | 同通用请求头                              |                 |
| 请求内容 | cmd：drop sequence<br>name：序列名        | cmd=drop sequence&name=IDSequence |
| 响应头   | 同通用响应头                              |                 |
| 响应内容 | {<br>errno: 返回值，0表示成功，其他为失败<br>description: 失败时的错误描述<br>} | [{ "errno": 0 }] |

##修改序列名##

|          | 说明                                      | 示例            |
|----------|-------------------------------------------|-----------------|
| 请求头   | 同通用请求头                              |                 |
| 请求内容 | cmd：rename sequence<br>name：当前序列名<br>newname：新的序列名 | cmd=rename sequence&name=IDSequence&newname=ID_SEQ |
| 响应头   | 同通用响应头                              |                 |
| 响应内容 | {<br>errno: 返回值，0表示成功，其他为失败<br>description: 失败时的错误描述<br>} | [{ "errno": 0 }] |

##获取序列下一个值##

|          | 说明                                      | 示例            |
|----------|-------------------------------------------|-----------------|
| 请求头   | 同通用请求头                              |                 |
| 请求内容 | cmd：get sequence next value<br>name：序列名 | cmd=get sequence next value&name=IDSequence |
| 响应头   | 同通用响应头                              |                 |
| 响应内容 | {<br>errno: 返回值，0表示成功，其他为失败<br>description: 失败时的错误描述<br>}<br>{<br>返回序列下一个值<br>}<br>... | [{ "errno": 0 },{ "NextValue": 1, "ReturnNum": 1, "Increment": 1 }] |

##获取序列当前值##

|          | 说明                                      | 示例            |
|----------|-------------------------------------------|-----------------|
| 请求头   | 同通用请求头                              |                 |
| 请求内容 | cmd：get sequence current value<br>name：序列名 | cmd=get sequence current value&name=IDSequence |
| 响应头   | 同通用响应头                              |                 |
| 响应内容 | {<br>errno: 返回值，0表示成功，其他为失败<br>description: 失败时的错误描述<br>}<br>{<br>返回序列当前值<br>}<br>... | [{ "errno": 0 },{ "CurrentValue": 1 }] |

##重置序列计数##

|          | 说明                                      | 示例            |
|----------|-------------------------------------------|-----------------|
| 请求头   | 同通用请求头                              |                 |
| 请求内容 | cmd：restart sequence<br>name：序列名<br>startvalue：起始值 | cmd=restart sequence&name=IDSequence&StartValue=1 |
| 响应头   | 同通用响应头                              |                 |
| 响应内容 | {<br>errno: 返回值，0表示成功，其他为失败<br>description: 失败时的错误描述<br>} | [{ "errno": 0 }] |

##修改序列属性##

|          | 说明                                      | 示例            |
|----------|-------------------------------------------|-----------------|
| 请求头   | 同通用请求头                              |                 |
| 请求内容 | cmd：set sequence attributes<br>name：序列名<br>options：序列属性<br>详细说明可参考 [SdbSequence.setAttributes\(\)][setSequenceAttr] | cmd=set sequence attributes&name=IDSequence&options={MinValue:0} |
| 响应头   | 同通用响应头                              |                 |
| 响应内容 | {<br>errno: 返回值，0表示成功，其他为失败<br>description: 失败时的错误描述<br>} | [{ "errno": 0 }] |

[^_^]:
     本文使用的所有引用及链接
[analyze]:manual/Manual/Sequoiadb_Command/Sdb/analyze.md
[snapshot]:manual/Manual/Sequoiadb_Command/Sdb/snapshot.md
[deleteConf]:manual/Manual/Sequoiadb_Command/Sdb/deleteConf.md
[updateConf]:manual/Manual/Sequoiadb_Command/Sdb/updateConf.md
[createSequence]: manual/Manual/Sequoiadb_Command/Sdb/createSequence.md
[setSequenceAttr]: manual/Manual/Sequoiadb_Command/SdbSequence/setAttributes.md
