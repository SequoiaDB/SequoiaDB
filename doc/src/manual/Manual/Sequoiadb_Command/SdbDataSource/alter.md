##名称##

alter - 修改数据源的元数据信息

##语法##

**SdbDataSource.alter( \<options\> )**

##类别##

SdbDataSource

##描述##

该函数用于修改数据源的名称、连接地址列表、访问权限等元数据信息。

##参数##

options（ *object，必填* ）

通过 options 参数可以修改数据源的元数据信息：

1. Name（string）：数据源名称

    格式：`Name:"datasource"`

2. Address（string）：数据源集群的协调节点地址

    格式：`Address:"sdbserver:11810"`

3. User（string）：数据源用户名

    格式：`User:"DSuser"`

4. Password（string）：User 所对应的数据源密码

    格式：`Password:"12345"`

5. AccessMode（string）：数据源访问权限

    取值如下：

    - "READ"：允许进行只读操作
    - "WRITE"：允许进行写操作
    - "ALL"或"READ|WRITE"：允许进行所有操作
    - "NONE"：不允许进行任何操作

    格式：`AccessMode:"READ"`

6. ErrorFilterMask（string）：控制对数据源进行数据操作的错误过滤

    取值如下：

    - "READ"：过滤数据读错误
    - "WRITE"：过滤数据写错误
    - "ALL"或"READ|WRITE"：过滤所有数据读写错误
    - "NONE"：不对任何错误进行过滤

    格式：`ErrorFilterMask:"READ"`

7. ErrorControlLevel（string ）：配置对映射集合或集合空间进行不支持的数据操作（如 DDL）时的报错级别，默认值为"low"

    取值如下：

    - "high"：报错并输出错误信息
    - "low"：忽略不支持的数据操作且不执行

    格式：`ErrorControlLevel:"low"`

8. TransPropagateMode（string）：配置事务操作在数据源上的传播模式，默认值为"never"

    取值如下：

    - "never": 不允许在数据源上进行事务操作，对操作直接报错
    - "notsupport": 事务操作在数据源上不受支持，如果在事务中操作数据源，对应的操作会降级为非事务操作后发送到数据源处理，在数据源上执行的操作不受事务保护

    格式：`TransPropagateMode:"never"`

9. InheritSessionAttr（boolean）：协调节点与数据源之间的会话是否继承本地会话的属性，默认值为 true，支持继承的属性包括：PreferredInstance，PreferredInstanceMode，PreferredStrict，PreferredPeriod，Timeout

    格式：`InheritSessionAttr: true`


##返回值##

函数执行成功时，无返回值。

函数执行失败时，将抛异常并输出错误信息。

##错误##

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取[错误码][error_code]。更多错误处理可以参考[常见错误处理指南][faq]。

##版本##

v3.2.8 及以上版本

##示例##

1. 获取数据源 datasource 的引用

    ```lang-javascript
    > var ds = db.getDataSource("datasource")
    ```

2. 修改该数据源的访问权限为“WRITE”

    ```lang-javascript
    > ds.alter({AccessMode:"WRITE"})
    ```



[^_^]:
    本文使用的所有引用及链接
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
