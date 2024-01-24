##名称##

getDetail - 获取回收站的具体信息

##语法##

**db.getRecycleBin().getDetail()**

##类别##

SdbRecycleBin

##描述##

该函数用于获取回收站的具体信息。

##参数##

无

##返回值##

函数执行成功时，将返回一个 BSONObj 类型的对象。通过该对象获取回收站的详细信息列表，字段说明如下：

| 字段名 | 类型 | 描述 |
| ---- | ---- | ---- |
| Enable | boolean | 是否启用回收站机制，默认值为 true，表示回收站机制已启用 |
| ExpireTime | number | 回收站项目的过期时间，默认值为 4320 分钟（即三天）<br> 过期的回收站项目将被删除|
| MaxItemNum | number | 回收站中最多可存放的项目个数，默认值为 100 |
| MaxVersionNum | number | 回收站中最多可存放的重复项目个数，默认值为 2 <br> 当多个项目的字段 OriginName 或 OriginID 相同时，这些项目被认为是重复项目 |
| AutoDrop | boolean | 回收站存放的项目个数超过限制时是否自动清理，默认值为 false，表示不自动清理<br>自动清理分为如下情况：<br> 1）项目个数超过字段 MaxItemNum 的限制时，将自动删除最早生成的项目<br>2）重复项目个数超过字段 MaxVersionNum 的限制时，将自动删除该重复项目的最早版本 |

函数执行失败时，将抛异常并输出错误信息。

##错误##

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取[错误码][error_code]。更多错误处理可以参考[常见错误处理指南][faq]。

##版本##

v3.6 及以上版本

##示例##

获取回收站的具体信息

```lang-javascript
> db.getRecycleBin().getDetail()
```

输出结果如下：

```lang-json
{
  "Enable": true,
  "ExpireTime": 4320,
  "MaxItemNum": 100,
  "MaxVersionNum": 2,
  "AutoDrop": false
}
```

[^_^]:
    本文使用的所有引用及链接
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
[view_items]:manual/Manual/Distributed_Engine/Maintainance/Recycle_Bin/view_items.md#查看回收站配置
