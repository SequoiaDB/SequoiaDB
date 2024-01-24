##名称##

invalidateCache - 清除节点的缓存信息

##语法##

**db.invalidateCache( [options] )**

##类别##

Sdb

##描述##

该函数用于清除节点的缓存信息。

##参数##

| 参数名 | 参数类型 | 描述 | 是否必填 |
| ------ | ------ | ------ | ------ |
| options | Json对象 | **[命令位置参数](manual/Manual/Sequoiadb_Command/location.md)** | 否 |

> **Note:**
>
> 当不指定 options 时，作用域为所有协调节点、所有数据节点、所有编目节点。

##返回值##

函数执行成功时，无返回值。

函数执行失败时，将抛异常并输出错误信息。

##错误##

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取错误码。更多错误处理可以参考[常见错误处理指南][error_guide]。

##版本##

v2.0 及以上版本

##示例##

* 清除当前协调节点和数据组‘group1’的缓存信息。

    ```lang-javascript
    > db.invalidateCache( { GroupName: 'group1' } )
    ```
 
* 清除当前协调节点的缓存信息。

    ```lang-javascript
    > db.invalidateCache( { Global: false } )
    ```

* 清除所有协调节点的缓存信息。

    ```lang-javascript
    > db.invalidateCache( { GroupName: 'SYSCoord' } )
    ```

[^_^]:
     本文使用的所有引用及链接

[list_info]:manual/Manual/List/list.md
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[error_guide]:manual/FAQ/faq_sdb.md