##名称##

createCS - 创建集合空间

##语法##

**db.createCS(\<name\>, [options])**

##类别##

Sdb

##描述##

该函数用于在数据库对象中创建集合空间。

##参数##

- name（ *string，必填* ）

    - 集合空间名，命名限制可参考[限制][limitation]
    - 同一个数据库对象中，集合空间名必须唯一

- options（ *object，选填* ）

    通过 options 参数可以设置集合空间的属性：

    - PageSize（ *number* ）：指定数据页/索引页大小，默认值为 65536，单位为字节

        该参数取值只能为 0、4096、8192、16384、32768 或 65536，取值为 0 表示选取默认值。

        格式：`PageSize: 4096`

    - Domain（ *string* ）：指定所属域，默认所属域为系统域“SYSDOMAIN”，系统域中包含所有的复制组

        通过该参数指定的域必须已存在，且不能指定为系统域。
       
        格式：`Domain: "mydomain"`

    - LobPageSize（ *number* ）：指定 Lob 数据页的大小，默认值为 262144，单位为字节

        该参数取值只能为 0、4096、8192、16384、32768、65536、131072、262144 或 524288，取值为 0 表示选取默认值。

        格式：`LobPageSize: 65536`

    - DataSource（ *string* ）：指定所使用的数据源名称

        格式：`DataSource: "datasource"`

    - Mapping（ *string* ）：指定所映射的集合空间名称

        格式：`Mapping: "sample"`

    > **Note:**
    >
    > - 参数 PageSize 和 LobPageSize 在集合空间写入数据后将不能修改，用户应谨慎取值。
    > - DataSource 和 Mapping 参数的具体使用场景可参考[数据源][datasource]。
    > * 为兼容较早版本接口，`db.createCS(<name>, [PageSize])` 依旧可用。

##返回值##

函数执行成功时，将返回一个 SdbCS 类型的对象。

函数执行失败时，将抛异常并输出错误信息。

##错误##

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取[错误码][error_code]。更多错误处理可以参考[常见错误处理指南][faq]。

##版本##

v2.0 及以上版本

##示例##

* 创建名为“sample”的集合空间

    ```lang-javascript
    > db.createCS("sample")
    ```


* 创建名为“sample”的集合空间，并指定数据页大小为 4096B，所属域为“mydomain”

    ```lang-javascript
    > db.createCS("sample", {PageSize: 4096, Domain: "mydomain"})
    ```



[^_^]:
    本文使用的所有引用和链接
[limitation]:manual/Manual/sequoiadb_limitation.md
[datasource]:manual/Distributed_Engine/Architecture/datasource.md
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
