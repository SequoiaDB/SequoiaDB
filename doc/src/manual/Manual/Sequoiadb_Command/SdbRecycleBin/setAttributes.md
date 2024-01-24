##名称##

setAttributes - 修改回收站的属性

##语法##

**db.getRecycleBin().setAttributes(\<options\>)**

##类别##

SdbRecycleBin

##描述##

该函数用于修改回收站的属性。

##参数##

options（ *object，必填* ）

通过 options 可以修改回收站的属性：

- Enable（ *boolean* ）：是否启用回收站机制

    格式：`Enable: true`

- ExpireTime（ *number* ）：回收站项目的过期时间，单位为分钟，取值范围为[-1, 2^31 -1]

    
    该参数取值为 0 表示不使用回收站机制；取值为 -1 表示永不过期。

    格式：`ExpireTime: 2880`

- MaxItemNum（ *number* ）：回收站中最多可存放的项目个数，取值范围为[-1, 2^31 -1]

    该参数取值为 0 表示不使用回收站机制；取值为 -1 表示不限制项目个数。

    格式：`MaxItemNum: 2000`

- MaxVersionNum（ *number* ）：回收站中最多可存放的重复项目个数，取值范围为[-1, 2^31 -1]

    该参数取值为 0 表示不使用回收站机制；取值为 -1 表示不限制重复项目的个数。

    格式：`MaxVersionNum: 6`

- AutoDrop（ *number* ）：回收站存放的项目个数超过限制时是否自动清理

    格式：`AutoDrop: true`

> **Note:**
>
> 各字段的详细说明可参考 [getDetail()][getDetail]。

##返回值##

函数执行成功时，无返回值。

函数执行失败时，将抛异常并输出错误信息。

##错误##

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取[错误码][error_code]。更多错误处理可以参考[常见错误处理指南][faq]。

##版本##

v3.6 及以上版本

##示例##

修改回收站项目的过期时间为 2880 分钟

```lang-javascript
> db.getRecycleBin().setAttributes({ExpireTime: 2880})
```

[^_^]:
    本文使用的所有引用及链接
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
[getDetail]:manual/Manual/Sequoiadb_Command/SdbRecycleBin/getDetail.md#返回值
