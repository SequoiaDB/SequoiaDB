##名称##

listNodes - 显示节点信息

##语法##

**Sdbtool.listNodes([options],[filter],[confRootPath])**

##类别##

Sdbtool

##描述##

该函数用于显示节点的详细信息。

##参数##

| 参数名   | 参数类型 | 默认值               | 描述                | 是否必填 |
| -------- | -------- | -------------------- | ------------------- | -------- |
| options  | JSON     | 默认显示数据节点，协调节点和编目节点的信息 | 显示指定类型的节点的信息 | 否 |
| filter   | JSON     | 默认显示全部内容     | 筛选条件            | 否       |
| confRootPath | string   |  `<INSTALL_DIR>/conf` | 指定节点的配置文件根目录（仅在 mode 属性为 local 时生效）  | 否       |

options 选项：

| 属性 | 值类型 | 默认值 | 格式 | 描述 |
| ---- | ------ | ------ | ---- | ---- |
| type | string |  db |{ type: "all" }<br>{ type: "db" }<br>{ type: "om" }<br>{ type: "cm" } |  显示所有节点的信息<br>显示数据节点，协调节点和编目节点的信息<br>显示 om 节点的信息<br>显示 cm 节点的信息 |
| mode | string | run |{ mode: "run" }<br>{ mode: "local" } | 显示正在运行的节点的信息<br>显示配置文件根目录下所有节点的信息 |
| role      | string | 空 | { role: "data" }<br>{ role: "coord" }<br>{ role: "catalog" }<br>{ role: "standalone" }<br>{ role: "om" }<br>{ role: "cm" } | 显示数据节点的信息<br>显示协调节点的信息<br>显示编目节点的信息<br>显示 standalone 节点的信息<br>显示 om 节点的信息<br>显示 cm 节点的信息 |
| svcname   | string | 空 | { svcname: "11790" } | 显示指定端口节点的信息 |
| showalone | bool | false | { showalone：true }<br>{ showalone: false } | 是否显示以 standalone 模式启动的 cm 节点的信息 |
| expand    | bool | false | { expand: true }<br>{ expand: false } | 是否显示详细的扩展配置 |

> Note：

> 1. cm 有 standalone 的启动模式。除了当前的 cm 之外，还可以通过 standalone 模式再启动一个 cm 作为临时 cm (启动 cm 的时候指定 standalone 参数)，默认存活时间为 5 分钟。

> 2. 当指定多个 svcname 时，可以以 ‘,’ 隔开。

> 3. filter 参数支持对结果中的某些字段进行 and 、 or 、not 和精确匹配计算，对结果集进行筛选。

##返回值##

函数执行成功时，将返回一个 BSONArray 类型的对象。通过该对象获取节点详细信息列表，字段说明如下：

| 字段名 | 类型 | 描述 |
|--------|------|------|
| svcname | string | 节点端口号 |
| type | string | 节点类型 |
| role | string | 节点角色，data 表示数据节点，coord 表示协调节点，catalog 表示编目节点 |
| pid | int32 | 进程号 |
| groupid | int32 | 节点所属复制组的 ID |
| nodeid | int32 | 节点 ID
| primary | int32 | 当前节点是否为主节点，1 表示主节点，0 表示备节点 |
| isalone | int32 | 节点是否以独立模式启动（仅在节点类型为 cm 时生效） |
| groupname | string | 节点所属复制组的名称 |
| starttime | string | 节点启动的时间 |
| dbpath | string | 节点数据文件的存放路径 |

函数执行失败时，将抛异常并输出错误信息。

##错误##

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取[错误码][error_code]。更多错误处理可以参考[常见错误处理指南][faq]。

##版本##

v3.2 及以上版本

##示例##

* 显示节点信息

    ```lang-javascript
    > Sdbtool.listNodes( { type: "all", mode: "local", role: "data", svcname: "20000, 40000" } )
    {
        "svcname": "20000",
        "type": "sequoiadb",
        "role": "data",
        "pid": 17390,
        "groupid": 1000,
        "nodeid": 1000,
        "primary": 1,
        "isalone": 0,
        "groupname": "db1",
        "starttime": "2019-05-31-17.14.14",
        "dbpath": "/opt/trunk/database/20000/"
    }
    {
        "svcname": "40000",
        "type": "sequoiadb",
        "role": "data",
        "pid": 17399,
        "groupid": 1001,
        "nodeid": 1001,
        "primary": 0,
        "isalone": 0,
        "groupname": "db2",
        "starttime": "2019-05-31-17.14.14",
        "dbpath": "/opt/trunk/database/40000/"
    }
    ```

* 显示节点信息后，对结果进行筛选

    ```lang-javascript
    > Sdbtool.listNodes( { type: "all", mode: "local", role: "data", svcname: "20000, 40000" }, { groupname: "db2" } )
    {
        "svcname": "40000",
        "type": "sequoiadb",
        "role": "data",
        "pid": 17399,
        "groupid": 1001,
        "nodeid": 1001,
        "primary": 0,
        "isalone": 0,
        "groupname": "db2",
        "starttime": "2019-05-31-17.14.14",
        "dbpath": "/opt/trunk/database/40000/"
    }
    ```

* 显示节点信息，指定系统配置文件的根路径

    ```lang-javascript
    > Sdbtool.listNodes( { mode: "local", svcname: "40000" }, { groupname: "db2" }, "/opt/sequoiadb/conf" )
    {
        "svcname": "40000",
        "type": "sequoiadb",
        "role": "data",
        "pid": 17399,
        "groupid": 1001,
        "nodeid": 1001,
        "primary": 0,
        "isalone": 0,
        "groupname": "db2",
        "starttime": "2019-05-31-17.14.14",
        "dbpath": "/opt/trunk/database/40000/"
    }
    ```

[^_^]:
     本文使用的所有引用及链接
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md

[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md

[faq]:manual/FAQ/faq_sdb.md

[error_code]:manual/Manual/Sequoiadb_error_code.md
