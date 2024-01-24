##名称##

setSessionAttr - 设置会话属性

##语法##

**db.setSessionAttr(\<options\>)**

##类别##

Sdb

##描述##

该函数用于设置会话属性。

##参数##

options（ *object，必填* ）

通过参数 options 可以设置会话属性：

- PreferredInstance（ *string/array/number* ）：会话读操作优先选择的实例，默认为协调节点配置文件中参数 preferredinstance 的取值。如果节点未配置 preferredinstance，该参数默认值为 "M"。

    取值类型分为角色取值和实例取值，用户可使用数组指定多个取值，具体如下：

    角色取值：

    - "M", "m"：可读写实例（主实例）
    - "S", "s"：只读实例（备实例）
    - "A", "a"：任意实例

    实例取值：

    - 1~255：[实例 ID][instance]

    >**Note:**
    >
    > - 同时指定实例和角色取值时，优先选择符合该实例 ID 和角色的节点。如果未匹配到对应节点，将随机选取数据组中的节点。例如取值为 [1, "S"] 时，表示优先选取实例 ID 为 1 的备节点，未匹配到对应节点时则随机选取节点。
    > - 符号 "-" 用于扩展角色取值。如果使用 "-" 扩展角色取值，在未匹配到对应节点的情况下，将随机选取指定角色中的节点。例如取值为 [1, "-S"] 时，如果未匹配到对应节点，将在数据组包含的备节点中随机选取节点。
    > - 指定多个角色取值时，角色与其扩展取值的语义相同，且只有第一个取值生效。例如取值为 ["-M", "S"] 时，仅 "-M" 生效，表示优先选择数据组中的主节点。
    > - 单独指定实例取值时，如果数据组内不存在对应实例 ID 的节点，节点的 instanceid 将根据数据组中 NodeID 的正序顺序，从 1 开始重新分配。同时服务端将按（实例 ID - 1）%（节点总数）对实例 ID 进行转换，转换后的值再与 instanceid 匹配获取对应节点。
    > - 如果同一个会话中，读请求前有写请求，有效期限内读请求将默认使用写请求使用的节点（主实例）进行读取，用户可通过配置 PreferredPeriod 修改读请求复用写请求节点的有效期限。

    格式：`PreferredInstance: "M"` 或 `PreferredInstance: [1, 10, "S"]`

- PreferredInstanceMode（ *string* ）：当存在多个候选实例时，指定会话的选择模式，默认为协调节点配置文件中参数 preferredinstancemode 的取值。如果节点未配置 preferredinstancemode，该参数默认值为 "random"。

    该参数取值如下：

    - "random"：从候选实例中随机选择
    - "ordered"：从候选实例中按照 PreferredInstance 的顺序进行选择

    格式：`PreferredInstaceMode: "random"`

- PreferredPeriod（ *number* ）：优先实例的有效周期，单位为秒，默认为协调节点配置文件中参数 preferredperiod 的取值。如果节点未配置 preferredperiod，该参数默认值为 60。
 
    - 每个 PreferredPeriod 有效周期之后，读请求将按 PreferredInstance 的取值重新选择合适的实例进行查询。
    - 该参数取值范围为 [-1, 2^31 - 1]；取值为 -1 表示不失效；取值为 0 表示本次查询不使用上次匹配的节点，根据 PreferredInstance 重新选择。
    - 该参数仅适用于 SequoiaDB v2.8.9、v3.2.5 及以上版本。

    格式：`PreferredPeriod: 60`

- PreferredStrict（ *boolean* ）：节点选择是否为严格模式，默认值为 false，表示非严格模式。

    当指定为严格模式时，节点只能从参数 PreferredInstance 指定的实例取值中选取。如果 PreferredInstance 未指定实例取值，该参数不生效。

    格式：`PreferredStrict: true `

- PreferredConstraint（ *string* ）：优先实例的约束条件，默认值为""，表示无约束条件。

    该参数取值如下：

    - "primaryonly"：仅选取主实例作为优先实例
    - "secondaryonly"：仅选取备实例作为优先实例
    - ""：无约束条件

    设置约束条件时，该参数指定的行为需与参数 PreferredInstance 一致。如 PreferredInstance 取值为"S"，PreferredConstraint 取值应为"secondaryonly"。

    格式：`PreferredConstraint: "secondaryonly"`

- Timeout（ *number* ）：会话执行操作的超时时间，超时则返回错误提示信息，单位为毫秒，默认值为 -1。

    该参数最小取值为 1000 毫秒；取值为 -1 表示不进行超时检测。

    格式：`Timeout: 10000`

- TransIsolation（ *number* ）：会话事务的隔离级别，默认值为 0。

    该参数取值如下：

    - 0：RU 级别
    - 1：RC 级别
    - 2：RS 级别

    格式：`TransIsolation: 1`

- TransTimeout（ *number* ）：会话事务锁等待超时时间，超时则返回错误提示信息，单位为秒，默认值为 60。

    格式：`TransTimeout: 10`

- TransLockWait（ *boolean* ）：会话事务在 RC 隔离级别下是否需要等锁，默认值为 false，表示无需等待记录锁。

    格式：`TransLockWait: true`

- TransUseRBS（ *boolean* ）：会话事务是否使用回滚段，默认值为 true，表示使用回滚段。

    格式：`TransUseRBS: true`

- TransAutoCommit（ *boolean* ）：会话事务是否开启自动事务提交，默认值为 false，表示不开启自动事务提交。

    格式：`TransAutoCommit: true`

- TransAutoRollback（ *boolean* ）：会话事务在操作失败时是否自动回滚，默认值为 true，表示自动回滚。

    格式：`TransAutoRollback: true`

- TransRCCount（ *boolean* ）：会话事务是否使用读已提交来处理 count() 查询，默认值为 true，表示使用读已提交。

    格式：`TransRCCount: true`

- TransMaxLockNum（ *number* ）：会话事务在一个数据节点上可以持有最大的记录锁个数，默认值为 10000，取值范围为[-1, 2^31 - 1]。

    该参数取值为 -1 时，表示事务对记录锁的个数无限制；取值为 0 时，表示事务不使用记录锁，直接使用集合锁。

    格式：`TransMaxLockNum: 10000`

- TransAllowLockEscalation（ *boolean* ）：会话事务持有记录锁个数超过参数 TransMaxLockNum 设置的值后，是否允许锁升级，默认值为 true，表示允许锁升级。

    如果事务持有的记录锁个数达到上限，但该参数的值为 false，事务操作将报错。

    格式：`TransAllowLockEscalation: true`

- TransMaxLogSpaceRatio（ *number* ）：会话事务在一个数据节点上可以使用的最大日志空间比例(%)，默认值为 50，取值范围为[1, 50]。

    该参数表示事务占数据节点日志空间总大小（日志空间总大小=logfilesz*logfilenum）的最大百分比。当事务使用的日志空间达到上限时，事务操作将报错。

    格式：`TransMaxLogSpaceRatio: 50`

    > **Note：**
    > 
    > - 事务相关属性只有 TransTimeout 允许在事务中设置，其它事务属性需要在非事务中设置。
    > - 获取会话属性请参考 [getSessionAttr()][getSessionAttr]。

##返回值##

函数执行成功时，无返回值。

函数执行失败时，将抛异常并输出错误信息。

##错误##

`setSessionAttr()` 函数常见异常如下：

| 错误码 | 错误类型       | 可能发生的原因       | 解决办法                   |
|--------|----------------|----------------------|----------------------------|
| -6     | SDB_INVALIDARG | options 属性输入错误 | 检查所设置属性的值和范围等 |

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取[错误码][error_code]，关于错误处理可以参考[常见错误处理指南][faq]。

##版本##

v3.4 及以上版本

##示例##

- 指定会话优先从主实例中读数据

    ```lang-javascript
    > db.setSessionAttr({PreferredInstance: "M"})
    ```

- 指定会话优先从实例 ID 为 1 的备实例中读数据

    ```lang-javascript
    > db.setSessionAttr({PreferredInstance: [1, "S"]})
    ```

- 指定会话执行操作的超时时间为 10 秒

    ```lang-javascript
    > db.setSessionAttr({Timeout: 10000})
    ```


[^_^]:
    本文使用的所有引用及链接
[getSessionAttr]:manual/Manual/Sequoiadb_Command/Sdb/getSessionAttr.md
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
[config]:manual/Manual/Database_Configuration/configuration_parameters.md
[instance]:manual/Distributed_Engine/Architecture/Data_Model/instance.md
