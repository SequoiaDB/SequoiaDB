##名称##

createDomain - 创建域

##语法##

**db.createDomain(\<name\>, \<groups\>, [options])**

##类别##

Sdb

##描述##

该函数用于创建一个域，域中可以包含若干个复制组。

##参数##

| 参数名 | 类型 | 描述 | 是否必填 |
| ------ | ------ | ------ | ------ |
| name | string | 域名，全局唯一 | 是 |
| groups | array | 域中包含的复制组 | 否 |
| options | object | 在创建域时可以通过 options 设置其他属性 | 否 |

目前通过 options 可设置的属性如下：

| 属性名 | 描述 | 格式 |
| ------ | ------ | ------ |
| AutoSplit | 是否自动切分散列分区集合 | AutoSplit: true |

> **Note:**
>
> 不支持在空域（不包含复制组的域）中创建集合空间。

##返回值##

函数执行成功时，将返回一个 SdbDomain 类型的对象。

函数执行失败时，将抛异常并输出错误信息。

##错误##

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取[错误码][error_code]。更多错误处理可以参考[常见错误处理指南][error_guide]。

##版本##

v2.0 及以上版本

##示例##

* 创建一个域，包含两个复制组

    ```lang-javascript
    > db.createDomain('mydomain', ['group1', 'group2'])
    ```

* 创建一个域，包含两个复制组，并且指定自动切分

    ```lang-javascript
    > db.createDomain('mydomain', ['group1', 'group2'], {AutoSplit: true})
    ```

[^_^]:
     本文使用的所有引用及链接
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[error_guide]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md