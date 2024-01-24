##名称##

createDataSource - 创建数据源

##语法##

**db.createDataSource(\<name\>, \<address\>, [user], [password], [type], [options])**

##类别##

Sdb

##描述##

该函数用于创建数据源，以实现跨集群的数据访问。

##参数##

- name（ *string，必填* ）

    数据源名称，同一数据库中该名称唯一

- address（ *string，必填* ）

    作为数据源的 SequoiaDB 集群中所有或部分协调节点地址

    >**Note:**
    >
    > - 通过逗号(,)可配置多个协调节点地址。用户需确保地址数不超过七个，且所有地址都指向同一集群。
    > - 协调节点地址所在机器的主机名不能与本地机器重名。

- user（ *string，选填* ）

    数据源用户名

- password（ *string，选填* ）

    与 user 对应的数据源用户密码

- type（ *string，选填* ）

    数据源类型，当前仅支持 sequoiadb

- options（ *object，选填* ）

    通过 options 参数可以设置其他选填参数

    - AccessMode（ *string* ）：配置对该数据源的访问权限，包括读写数据，默认值为"ALL"

        取值如下：

        - "READ"：允许进行只读操作
        - "WRITE"：允许进行写操作
        - "ALL"或"READ|WRITE"：允许进行所有操作
        - "NONE"：不允许进行任何操作

        格式：`AccessMode: "READ"`

    - ErrorFilterMask（ *string* ）：配置对数据源进行数据操作的错误过滤，默认值为"NONE"

        取值如下：

        - "READ"：过滤数据读错误
        - "WRITE"：过滤数据写错误
        - "ALL"或"READ|WRITE"：过滤所有数据读写错误
        - "NONE"：不对任何错误进行过滤

        格式：`ErrorFilterMask: "READ"`

   - ErrorControlLevel（ *string* ）：配置对映射集合或集合空间进行不支持的数据操作（如 DDL）时的报错级别，默认值为"low"

        取值如下：

        - "high"：报错并输出错误信息
        - "low"：忽略不支持的数据操作且不执行

        格式：`ErrorControlLevel: "low"`

    - TransPropagateMode（ *string* ）：配置事务操作在数据源上的传播模式，默认值为"never"

        取值如下：

        - "never": 不允许在数据源上进行事务操作，对操作直接报错
        - "notsupport": 事务操作在数据源上不受支持，如果在事务中操作数据源，对应的操作会降级为非事务操作后发送到数据源处理，在数据源上执行的操作不受事务保护

        格式：`TransPropagateMode: "never"`

    - InheritSessionAttr（ *boolean* ）：协调节点与数据源之间的会话是否继承本地会话的属性，默认值为 true

        支持继承的属性包括 PreferredInstance、PreferredInstanceMode、PreferredStrict、PreferredPeriod 和 Timeout。

        格式：`InheritSessionAttr: true`

##返回值##

函数执行成功时，将返回一个 SdbDataSource 类型对象。

函数执行失败时，将抛异常并输出错误信息。

##错误##

`createDataSource()` 函数常见异常如下：

| 错误码 | 错误类型 | 可能发生的原因 | 解决办法 |
| ------ | -------- | -------------- | -------- |
| -369 | SDB_CAT_DATASOURCE_EXIST | 指定数据源已存在 | 检查是否存在同名数据源 |

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取[错误码][error_code]。更多错误处理可以参考[常见错误处理指南][faq]。

##版本##

v3.4.3 及以上版本

##示例##

创建一个名为“datasource”的数据源，该数据源只允许进行只读操作

```lang-javascript
> db.createDataSource("datasource", "192.168.20.66:50000", "", "", "SequoiaDB", {AccessMode: "READ"})
```



[^_^]:
    本文使用的所有引用及链接
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
