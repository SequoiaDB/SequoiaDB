##名称##

snapshot - 查看回收站项目快照

##语法##

**db.getRecycleBin().snapshot([cond], [sel], [sort])**
**db.getRecycleBin().snapshot([SdbSnapshotOption])**

##类别##

SdbRecycleBin

##描述##

该函数用于查看回收站项目快照。

##参数##

* cond（ *object，选填* ）

    匹配条件以及[命令位置参数][location]，只返回符合 cond 的记录

* sel（ *object，选填* ）

    选择返回的字段名

* sort（ *object，选填* ）

    对返回的记录按选定的字段排序，取值如下：
    - 1：升序
    - -1：降序

* SdbSnapshotOption（ *SdbSnapshotOption，选填* ）

    使用一个对象指定快照查询参数，使用方法可参考 [SdbSnapshotOption][shotOption] 

##返回值##

函数执行成功时，将返回一个 SdbCursor 类型的对象。通过该对象获取回收站项目快照，字段说明可参考[回收站项目快照][SDB_SNAP_RECYCLEBIN]。

函数执行失败时，将抛异常并输出错误信息。

##错误##

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取[错误码][error_code]。更多错误处理可以参考[常见错误处理指南][faq]。

##版本##

v3.6 及以上版本

##示例##

查看所有回收站项目的快照

```lang-javascript
> db.getRecycleBin().snapshot()
```

[^_^]:
    本文使用的所有引用及链接
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
[SDB_SNAP_RECYCLEBIN]:manual/Manual/Snapshot/SDB_SNAP_RECYCLEBIN.md
[location]:manual/Manual/Sequoiadb_Command/location.md
[shotOption]:manual/Manual/Sequoiadb_Command/AuxiliaryObjects/SdbSnapshotOption.md
