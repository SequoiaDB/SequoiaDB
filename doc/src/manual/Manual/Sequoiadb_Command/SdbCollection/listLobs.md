##名称##

listLobs - 列举集合中的大对象

##语法##

**db.collectionspace.collection.listLobs([SdbQueryOption])**

##类别##

SdbCollection

##描述##

该函数用于获取集合中的大对象列表。

##参数##

SdbQueryOption( *Object*， *选填* )
    
使用一个对象来指定记录查询参数，使用方法可参考 [SdbQueryOption][QueryOption]
    
    
>**Note:**
>
> 当用户使用 SdbQueryOption 指定 hint 为{"ListPieces": 1}时，可以获取 Lob 详细的分片信息。


##返回值##

函数执行成功时，将返回一个 DBCursor 类型的对象。  

函数执行失败时，将抛出异常并输出错误信息。

##错误##

`listLobs()` 函数常见异常如下：

| 错误码 | 错误类型 | 描述 | 解决方法 |
| ------ | ------ | --- | ------ |
| -6 | SDB_INVALIDARG | 参数错误 | 查看参数是否填写正确 |
| -34 | SDB_DMS_CS_NOTEXIST | 集合空间不存在 | 检查集合空间是否存在 |
| -23 | SDB_DMS_NOTEXIST| 集合不存在 | 检查集合是否存在 |

当异常抛出时，可以通过 [getLastError()][getLastError] 获取[错误码][error_code]，或通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息。详细信息可参考[常见错误处理指南][faq]。

##版本##

此函数适用于 v2.0 及以上版本，其中 v3.2 及以上版本支持通过输入参数获取指定的大对象。

##示例##

* 列举 sample.employee 中所有的大对象

    ```lang-javascript
    > db.sample.employee.listLobs()
    {
       "Size": 2,
       "Oid": {
         "$oid": "00005d36c8a5350002de7edc"
       },
       "CreateTime": {
         "$timestamp": "2019-07-23-16.43.17.360000"
       },
       "ModificationTime": {
         "$timestamp": "2019-07-23-16.43.17.508000"
       },
       "Available": true,
       "HasPiecesInfo": false
     }
     {
       "Size": 51717368,
       "Oid": {
         "$oid": "00005d36cae8370002de7edd"
       },
       "CreateTime": {
         "$timestamp": "2019-07-23-16.52.56.278000"
       },
       "ModificationTime": {
         "$timestamp": "2019-07-23-16.52.56.977000"
       },
       "Available": true,
       "HasPiecesInfo": false
    }
    Return 2 row(s).
    ```
   
* 列举 sample.employee 中 Size 大于 10 的大对象
   
    ```lang-javascript
    > db.sample.employee.listLobs( SdbQueryOption().cond( { "Size": { $gt: 10 } } ) )
    {
       "Size": 51717368,
       "Oid": {
         "$oid": "00005d36cae8370002de7edd"
       },
       "CreateTime": {
         "$timestamp": "2019-07-23-16.52.56.278000"
       },
       "ModificationTime": {
         "$timestamp": "2019-07-23-16.52.56.977000"
       },
       "Available": true,
       "HasPiecesInfo": false
    }
    Return 1 row(s).
    ```



[^_^]:
     本文使用的所有引用及链接
[QueryOption]:manual/Manual/Sequoiadb_Command/AuxiliaryObjects/SdbQueryOption.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md