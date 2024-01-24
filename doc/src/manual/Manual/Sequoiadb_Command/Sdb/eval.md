##名称##

eval - 调用存储过程

##语法##

**db.eval(\<code\>)**

##类别##

Sdb

##描述##

该函数用于在语句中调用已经创建好的存储过程。用户可根据需要填入 JavaScript 语句。

##参数##

| 参数名 | 类型 | 描述 | 是否必填 |
| ------ | ------ | ------ | ------ |
| code | string | JavaScript 语句或者创建好的存储过程函数 | 是 |

> **Note：**
>
> 存储过程会屏蔽所有标准输出和标准错误。同时，不建议在函数执行时加入输出语句，大量的输出可能会导致存储过程运行失败。

##返回值##

- 函数执行成功时，将按照语句返回结果。可以将返回值直接赋值给另一个变量，如：`var a = db.eval( 'db.sample.employee' ); a.find(); `。

- 函数执行失败时，将抛异常并输出错误信息。

- 在函数执行结束前操作不会返回。中途退出则终止整个执行，但已经执行的代码不会被回滚。

- 自定义返回值的长度有一定限制，参考 SequoiaDB 插入记录的最大长度。

- 支持定义临时函数，如：`db.eval( 'function sum(x,y){return x+y;} sum(1,2)' )`。

- 全局 db 使用方式与 [createProcedure()][createProcedure] 相同。

##错误##

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取[错误码][error_code]。更多错误处理可以参考[常见错误处理指南][faq]。

##版本##

v2.0 及以上版本

##示例##

* 通过 eval() 调用存储过程函数 sum

    ```lang-javascript
    //初始时 sum() 方法不存在，返回异常信息
    > var a = db.eval( 'sum(1,2)' );
    { "errmsg": "(nofile):1 ReferenceError: getCL is not defined", "retCode": -152 }
    (nofile):0 uncaught exception: -152
    //初始化 sum()
    > db.createProcedure( function sum(x,y){return x+y;} )
    //调用 sum()
    > db.eval( 'sum(1,2)' )
    3
    ```

* 通过 eval() 填写 JavaScript 语句并执行

    ```lang-javascript
    > var ret = db.eval( "db.sample.employee" )
    > ret.find()
    {
      "_id": {
        "$oid": "5248d3867159ae144a000000"
      },
      "a": 1
    }
    {
      "_id": {
        "$oid": "5248d3897159ae144a000001"
      },
      "a": 2
    }...
    ```

[^_^]:
     本文使用的所有引用及链接
[createProcedure]:manual/Manual/Sequoiadb_Command/Sdb/createProcedure.md
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
