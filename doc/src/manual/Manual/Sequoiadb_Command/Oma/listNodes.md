##名称##

listNodes - 列举节点信息

##语法##

**oma.listNodes(\[options\], \[filter\])**

##类别##

Oma

##描述##

该函数用于列举当前 sdbcm 所在机器符合条件的节点信息，默认显示数据节点、协调节点和编目节点的信息。

##参数##

- options ( *object，选填* )

    通过参数 options 可以指定节点类型、模式等参数：

    - type ( *string* )：节点类型，默认值为"db"
  
        取值如下：

        - "all"：所有节点
        - "db"：数据节点，协调节点和编目节点
        - "om"：sdbom 节点
        - "cm"：sdbcm 节点

        格式：`type: "all"` 

    - mode ( *string* )：节点模式，默认值为"run"

        取值如下：

        - "run"：正在运行的节点
        - "local"：本地节点，无论是否正在运行

        格式：`mode: "local"`

    - role ( *string* )：节点角色

        取值如下：

        - "data"：数据节点
        - "coord"：协调节点
        - "catalog"：编目节点
        - "standalone"：standalone 节点
        - "om"：sdbom 节点
        - "cm"：sdbcm 节点

        格式：`role: "data"`

    - svcname ( *string* )：节点端口号

        当指定多个 svcname 时，可用逗号（,）隔开

        格式：`svcname: "11820, 11830" `

    - showalone ( *boolean* )：是否显示以 [standalone 模式][standalone]启动的 sdbcm 节点信息，默认为 false

        格式：`standalone: true`

    - expand ( *boolean* )：是否显示节点的扩展信息，默认值为 false

        格式：`expand: true`

- filter ( *object，选填* )

    指定筛选节点信息的条件，支持通过[匹配符][match] $and、$or、$not 或精确匹配检索节点信息

##返回值##

函数执行成功时，将返回一个 BSONArray 类型的对象。通过该对象获取节点详细信息列表，字段说明如下：

|字段名|类型|描述|
|------|----|----|
|svcname|string|节点端口号|
|type|string|节点类型|
|role|string|节点角色|
|pid|int32|进程号|
|groupid|int32|节点所属复制组的 ID|
|nodeid|int32|节点 ID|
|primary|int32|节点是否为主节点，1 表示主节点，0 表示备节点|
|isalone|int32|节点是否以独立模式启动（仅在参数 role 为"cm"时有效）|
|groupname|string|节点所属复制组的名称|
|starttime|string|节点启动的时间|
|dbpath|string|节点数据文件的存放路径|

函数执行失败时，将抛异常并输出错误信息。

##错误##

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取[错误码][error_code]。更多错误处理可以参考[常见错误处理指南][faq]。

##版本##

v2.0 及以上版本

##示例##

连接到本地的集群管理服务进程 sdbcm，获取 11830 节点的信息

```lang-javascript
> var oma = new Oma("localhost", 11790)
> oma.listNodes({svcname: "11830"})
{
  "svcname": "11830",
  "type": "sequoiadb",
  "role": "data",
  "pid": 17984,
  "groupid": 1001,
  "nodeid": 1001,
  "primary": 1,
  "isalone": 0,
  "groupname": "group2",
  "starttime": "2021-07-15-16.27.47",
  "dbpath": "/opt/sequoiadb/database/data/11830/"
}
```

[^_^]:
    本文使用的所有引用及链接
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
[match]:manual/Manual/Operator/Match_Operator/Readme.md
[standalone]:manual/Manual/Sequoiadb_Command/Oma/start.md