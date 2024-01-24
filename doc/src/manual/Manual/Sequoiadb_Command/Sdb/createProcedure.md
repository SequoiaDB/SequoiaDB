##名称##

createProcedure - 创建存储过程

##语法##

**db.createProcedure(\<code\>)**

##类别##

Sdb

##描述##

该函数用于在数据库对象中创建存储过程。

##参数##

code（ *函数，必填* ）

指定符合 JavaScript 语法的自定义函数，格式如下：

```lang-javascript
function 函数名(参数) {
  函数体
}
```

- 函数体中可以调用其他函数。如果调用了不存在的函数，需要保证存储过程运行时相关函数已存在。

- 函数名全局唯一，不支持重载。

- 每个函数均全局可用，随意删除一个函数可能导致相关存储过程运行失败。

- 存储过程会屏蔽所有标准输出和标准错误。同时，不建议在函数定义或执行时加入输出语句，大量的输出可能会导致存储过程运行失败。

- 函数返回值可以是除 Sdb 以外的任意类型数据。

> **Note:**
>
> 独立模式部署的集群，不支持创建存储过程。

##返回值##

函数执行成功时，无返回值。

函数执行失败时，将抛异常并输出错误信息。

##错误##

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取[错误码][error_code]。更多错误处理可以参考[常见错误处理指南][faq]。

##版本##

v2.0 及以上版本

##示例##

集合 sample.employee 存在如下记录：

```lang-json
{id: 1, Name: "Jack", age: 25}
```

1. 连接协调节点

    ```lang-javascript
    > var coord = new Sdb("sdbserver", 11810)
    ```

2. 创建存储过程

    ```lang-javascript
    > coord.createProcedure(function getAll() { return db.sample.employee.find(); })
    ```

    > **Note:**
    >
    > db 在存储过程中已初始化。用户可以使用该全局 db 指代执行该存储过程的会话所对应的鉴权信息。

3. 通过 [eval()][eval] 执行该存储过程

    ```lang-javascript
    > coord.eval("getAll()")
    {
      "_id": {
        "$oid": "60cd4c2e1a52b21546a15826"
      },
      "id": 1,
      "Name": "Jack",
      "age": 25
    }
    ```

用户可以通过 [listProcedures()][listProcedures] 查看已创建的存储过程信息。

[^_^]:
     本文使用的所有引用及链接
[listProcedures]:manual/Manual/Sequoiadb_Command/Sdb/listProcedures.md
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[eval]:manual/Manual/Sequoiadb_Command/Sdb/eval.md
[error_code]:manual/Manual/Sequoiadb_error_code.md