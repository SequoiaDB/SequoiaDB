##名称##

copyIndex - 复制索引

##语法##

**db.collectionspace.collection.copyIndex\([subCLName], [indexName])**

##类别##

SdbCollection

##描述##

该函数用于将主集合的索引复制到子集合中。

##参数##

- subCLName（ *string，选填* ）

    指定子集合名，格式为\<csname\>.\<clname\>，默认值为 null ，表示主集合下的所有子集合

- indexName（ *string，选填* ）

    指定索引名，默认值为 null ，表示主集合下的所有索引

##返回值##

函数执行成功时，无返回值。

函数执行失败时，将抛异常并输出错误信息。

##错误##

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取[错误码][error_code]。更多错误处理可以参考[常见错误处理指南][error_guide]。

##版本##

v3.6 及以上版本

##示例##

1. 在主集合 sample.employee 中创建名为"IDIdx"的索引

    ```lang-javascript
    > db.sample.employee.createIndex("IDIdx", {ID: 1})
    ```

2. 将主集合的索引复制到子集合中

    ```lang-javascript
    > db.sample.employee.copyIndex()
    ```

3. 查看子集合 sample.January 的索引信息，显示已添加索引 IDIdx

    ```lang-javascript
    > db.sample.January.listIndexes()
    {
      "IndexDef": {
        "name": "ID",
        "key": {
          "ID": 1
        },
        ...
      },
      ...
    }
    ```

[^_^]:
     本文使用的所有引用和链接
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
[text_index]:manual/Distributed_Engine/Architecture/Data_Model/text_index.md
[text_index]:manual/Distributed_Engine/Architecture/Data_Model/text_index.md
[error_guide]:manual/FAQ/faq_sdb.md
