##名称##

find - 查找文件

##语法##

**File.find(\<options\>, \[filter\])**

##类别##

File

##描述##

该函数用于在指定目录下查找文件。

##参数##

| 参数名 | 类型 | 描述 | 是否必填 |
| ------ | ---- | ---- | -------- |
| options   | object   | 查找模式和查找内容|是       |
| filter    | object   | 筛选条件，支持通过 and、or、not 或精确匹配计算对结果集进行筛选 |否       |

options 参数详细说明如下：

| 参数名 | 类型 | 描述 | 是否必填 |
| ------ | ---- | ---- | -------- |
| mode     | string   | 查找的类型，取值如下：<br>'n'：按文件名查找文件<br>'u'：按用户名查找文件<br>'g'：按用户组名查找文件<br>'p'：按权限查找文件 | 是 |
| pathname | string | 查找的路径，默认在当前路径下查找文件 | 否 |
| value    | string | 查找的内容                        | 是 |

##返回值##

函数执行成功时，将返回一个 BSONArray 类型的对象。通过该对象获取文件的所在路径。

函数执行失败时，将抛异常并输出错误信息。

##错误##

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取[错误码][error_code]。更多错误处理可以参考[常见错误处理指南][faq]。

##版本##

v3.4 及以上版本

##示例##

- 按文件名查找目录 `/opt` 下的文件 `file.txt`

    ```lang-javascript
    > File.find({mode: 'n', value: "file.txt", pathname: "/opt"})
    {
        "pathname": "/opt/sequoiadb1/file.txt"
    }
    {
        "pathname": "/opt/sequoiadb2/file.txt"
    }
    {
        "pathname": "/opt/sequoiadb3/file.txt"
    }
    ```

- 按文件名查找目录 `/opt` 下的文件 `file.txt`，同时指定筛选条件

    ```lang-javascript
    > File.find({mode: 'n', value: "file.txt", pathname: "/opt"}, {$or: [{pathname: "/opt/sequoiadb1/file.txt"}, {pathname: "/opt/sequoiadb2/file.txt"}]})
     {
         "pathname": "/opt/sequoiadb1/file.txt"
     }
     {
         "pathname": "/opt/sequoiadb2/file.txt"
     }
    ```
[^_^]:
    本文使用的所有引用及链接
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md