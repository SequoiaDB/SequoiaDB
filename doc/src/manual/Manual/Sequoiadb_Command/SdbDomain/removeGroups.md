##名称##

removeGroups - 删除域包含的复制组

##语法##

**domain.removeGroups(\<object\>)**


##类别##

SdbDomain

##描述##

该函数用于删除域包含的复制组。

##参数##

options ( *object，必填* )

通过参数 options 可以指定需要删除的复制组：

-  Groups ( *string/array* )：域包含的复制组

    该参数指定的复制组中不允许存在数据，否则操作报错。

    格式：`Groups: ['group1', 'group2']`

##返回值##

函数执行成功时，无返回值。

函数执行失败时，将抛异常并输出错误信息。

##错误##

`removeGroups()` 函数常见异常如下：

| 错误码 | 错误类型 | 可能发生的原因 | 解决办法 |
| ------ | ---------|--------------- |----------|
| -154   | SDB_CLS_GRP_NOT_EXIST |分区组不存在 | 使用列表查看分区组是否存在 |
| -256   |SDB_DOMAIN_IS_OCCUPIED | 域已被使用   | 使用 [listCollectionSpaces()][listCollectionSpaces] 查看域是否存在集合空间 |

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取[错误码][error_code]。更多错误处理可以参考[常见错误处理指南][faq]。

##版本##

v3.4 及以上版本

##示例##

- 创建一个包含复制组 group1 和 group2 的域，复制组中不存在数据

    ```lang-javascript
    > var domain = db.createDomain('mydomain', ['group1', 'group2'])
    ```

    删除域包含的复制组 group1

    ```lang-javascript
    > domain.removeGroups({Groups: ['group1']})
    ```

- 创建一个包含复制组 group1 的域，复制组中存在数据

    ```lang-javascript
    > var domain = db.createDomain('mydomain', ['group1'])
    ```

    删除域包含的复制组 group1

    ```lang-javascript
    > domain.removeGroups({Groups: ['group1']})
    ```

    由于 group1 中存在数据，操作报错
   
    ```lang-javascript
    (nofile):0 uncaught exception: -256
    Domain has been used
    ```

[^_^]:
     本文使用的所有引用及链接
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
[listCollectionSpaces]:manual/Manual/Sequoiadb_Command/SdbDomain/listCollectionSpaces.md
[split]:manual/Manual/Sequoiadb_Command/SdbCollection/split.md