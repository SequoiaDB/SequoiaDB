##名称##

getSessionAttr - 获取会话属性

##语法##

**db.getSessionAttr()**

##类别##

Sdb

##描述##

该函数用于获取会话属性。

> **Note:**
>
> 如果当前会话属性不符合预期，可使用 [setSessionAttr()][setSessionAttr] 设置会话属性。

##参数##

无

##返回值##

函数执行成功时，将返回一个 BSONObj 类型的对象。通过该对象获取会话属性的详细信息列表，字段说明可参考setSessionAttr()。

函数执行失败时，将抛异常并输出错误信息。

##错误##

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取[错误码][error_code]。更多错误处理可以参考[常见错误处理指南][faq]。

##版本##

v2.8 及以上版本

##示例##

获取会话属性

```lang-javascript
> db.getSessionAttr()
{
  "PreferedInstance": "M",
  "PreferredInstance": "M",
  "PreferedInstanceMode": "random",
  "PreferredInstanceMode": "random",
  "PreferedStrict": false,
  "PreferredStrict": false,
  "PreferedPeriod": 60,
  "PreferredPeriod": 60,
  "PreferredConstraint": "",
  "Timeout": -1,
  "TransIsolation": 0,
  "TransTimeout": 60,
  "TransUseRBS": true,
  "TransLockWait": false,
  "TransAutoCommit": false,
  "TransAutoRollback": true,
  "TransRCCount": true,
  "TransAllowLockEscalation": true,
  "TransMaxLockNum": 10000,
  "TransMaxLogSpaceRatio": 50,
  "Source": ""
}
```

> **Note:**
>
> v3.4.5 及以上版本中，字段 PreferedInstance、PreferedInstanceMode、PreferedStrict 和 PreferedPeriod 已更名为 PreferredInstance、PreferredInstanceMode、PreferredStrict 和 PreferredPeriod，用户应使用更名后的字段。为保证兼容性，SequoiaDB 由低版本升级至 v3.4.5 及以上版本后，仍兼容原字段。

[^_^]:
     本文使用的所有引用及链接
[setSessionAttr]:manual/Manual/Sequoiadb_Command/Sdb/setSessionAttr.md
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
