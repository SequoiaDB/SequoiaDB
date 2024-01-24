##名称##

createLobID - 创建大对象ID

##语法##

**db.collectionspace.collection.createLobID([Time])**

##类别##

SdbCollection

##描述##

该函数用于通过服务端生成大对象ID。

##参数##

* `Time`( *String*， *选填* )
  
    根据Time生成大对象ID，目前最小单位只获取到秒级。Time的格式为："YYYY-MM-DD-HH.mm.ss"，如："2019-08-01-12.00.00"。
    
* 无参时，将根据服务器上的时间来生成大对象ID。

##返回值##

函数执行成功时，将返回一个 String 类型的对象。通过该对象获取生成的大对象 ID。

函数执行失败时，将抛异常并输出错误信息。

##错误##

`createLobID()`函数常见异常如下：

| 错误码 | 错误类型 | 描述 | 解决方法 |
| ------ | ------ | --- | ------ |
| -6 | SDB_INVALIDARG | 参数错误。 | 查看参数是否填写正确。|
| -34 | SDB_DMS_CS_NOTEXIST | 集合空间不存在。| 检查集合空间是否存在。|
| -23 | SDB_DMS_NOTEXIST| 集合不存在。 | 检查集合是否存在。|

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取错误码。更多错误处理可以参考[常见错误处理指南][error_guide]。

##版本##

v3.2 及以上版本

##示例##

* 在 sample.employee 中创建大对象ID

    ```lang-javascript
    > db.sample.employee.createLobID()
    00005d36d096350002de7f3a
    Takes 0.329455s.
    ```

* 根据传入的Time，在 sample.employee 中创建大对象ID

    ```lang-javascript
    > db.sample.employee.createLobID( "2015-06-05-16.10.33" )
    00005571c9f93f03e8d8dd57
    Takes 0.108214s.
    ```

[^_^]:
     本文使用的所有引用及链接
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[error_guide]:manual/FAQ/faq_sdb.md