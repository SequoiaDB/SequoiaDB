[^_^]:
    JSON数据文件导入
    作者：谢建宏
    时间：20190219
    评审意见
    王涛：20190309
    许建辉：
    市场部：20190325

用户可以使用 SequoiaDB 巨杉数据库提供的 [sdbimprt工具][sdbimprt]将以下数据导入 SequoiaDB 的集合中：

+ 从其他数据库导出的 JSON 数据文件中的数据
+ [sdbexprt工具][sdbexprt] 导出的 JSON 数据文件中的数据
+ 用户程序生成的 JSON 数据文件中的数据

数据准备
----

以下是 JSON 数据文件 `data.json` 中的三条 JSON 数据：

```lang-json
{"id": 1, "name": "sdbUserA", "phone": "13249996666", "email": "sdbUserA@example.com" }
{"id": 2, "name": "sdbUserB", "phone": "13248885555", "email": "sdbUserB@example.com" }
{"id": 3, "name": "sdbUserC", "phone": "13248886666", "email": "sdbUserC@example.com" }
```

> **Note：**
>
> JSON 数据文件中的 JSON 数据必须满足以下要求：
>
> + 符合 JSON 的定义，以左右括号作为记录的分界符
> + JSON 数据之间无逗号（","）分隔
> + 字符串类型的数据包含在两个双引号（""）之间
> + 字符串类型的数据包含双引号时，需要使用反斜杠（"\"）转义字符

数据导入
----

假设本地 SequoiaDB 已存在集合 sample.employee，现将上述示例 JSON 数据文件导入集合 sample.employee 中

```lang-bash
sdbimprt --hosts "localhost:11810" --csname sample --clname employee --file data.json --type json
```

> **Note:**
>
> + 数据文件的数据量较大时，可使用 `-n` 指定每次导入的记录数以及 `-j` 指定导入连接数来提供导入效率
> + `--file` 参数支持指定多个文件或者目录，使用逗号“,”分隔，重复出现的文件会被忽略
>
> 更多参数说明详见 [sdbimprt 工具][sdbimprt]介绍

小结
----

SequoiaDB 的 sdbimprt 工具支持并发导入单一的 JSON 数据文件和批量导入 JSON 数据文件目录。用户使用该工具能简单快速地将 JSON 数据导入 SequoiaDB。

[^_^]:
    本文使用到的所用链接或引用。TODO：待补充sdbimprt和sdbexprt工具的文档介绍的链接或引用

[sdbimprt]:manual/Distributed_Engine/Maintainance/Mgmt_Tools/sdbimprt.md
[sdbexprt]:manual/Distributed_Engine/Maintainance/Mgmt_Tools/sdbexprt.md
