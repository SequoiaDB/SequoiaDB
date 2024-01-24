##名称##

list - 查看回收站项目列表

##语法##

**db.getRecycleBin().list([cond], [sel], [sort])**

##类别##

SdbRecycleBin

##描述##

该函数用于查看回收站项目列表。

##参数##

* cond（ *object，选填* ）

    匹配条件，只返回符合 cond 的记录

* sel（ *object，选填* ）

    选择返回的字段名

* sort（ *object，选填* ）

    对返回的记录按选定的字段排序，取值如下：
    - 1：升序
    - -1：降序

##返回值##

函数执行成功时，将返回一个 SdbCursor 类型的对象。通过该对象获取回收站项目列表，字段说明可参考[回收站项目列表][SDB_LIST_RECYCLEBIN]。

函数执行失败时，将抛异常并输出错误信息。

##错误##

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取[错误码][error_code]。更多错误处理可以参考[常见错误处理指南][faq]。

##版本##

v3.6 及以上版本

##示例##

查看所有回收站项目的列表

```lang-javascript
> db.getRecycleBin().list()
```

[^_^]:
    本文使用的所有引用及链接
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
[SDB_LIST_RECYCLEBIN]:manual/Manual/List/SDB_LIST_RECYCLEBIN.md
