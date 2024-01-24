
sdbexprt 是 SequoiaDB 巨杉数据库的数据导出工具。用户可以通过 sdbexprt 将 SequoiaDB 中的集合以 JSON 或 CSV 格式导出到数据存储文件中。sdbexprt 支持将一个集合导出到一个文件，同时也支持将多个集合批量导出到指定目录下。

##JSON##

JSON 导出格式中的 JSON 记录符合 JSON 的定义，以左右花括号作为 JSON 记录的分界符，并且字符串类型的数据必须包含在两个双引号之间，转义字符为反斜杠“\\”。默认情况下（SequoiaDB）记录以换行符分隔，用户能够指定记录分隔符。

##CSV##

CSV（Comma Separated Values）导出格式以逗号分隔数值。默认情况下记录以换行符分隔，字段以逗号分隔。用户能够指定字符串分隔符、字段分隔符以及记录分隔符。

##分隔符##

| 类型         | 默认值          |
| ------------ | --------------- |
| 字符串分隔符 | "（双引号）     |
| 字段分隔符   | ,（逗号）       |
| 记录分隔符   | '\\n'（换行符） |

>   **Note:**
>
>* 可以使用 UTF-8 字符作为分隔符；
>* 可以使用多个字符作为分隔符；
>* 可以使用 ASCII 码中的不可见字符作为分隔符，通过转义字符“\\”输入 ASCII 码的十进制数值（0~127），例如“\\30”。回车符、换行符、制表符、转义字符“\\”可以直接使用“\\r”、“\\n”、“\\t”、“\\\\”。

##选项##

###通用选项###

| 参数名      | 缩写 | 描述 |
| ----------- | ---- | ---- |
| --help      | -h   | 显示帮助信息 |
| --version   |      | 显示版本信息 |
| --hosts     |      | 指定主机地址（hostname:svcname），用“,”分隔多个地址，默认值为 `localhost:11810` |
| --user      | -u   | 指定数据库用户名 |
| --password  | -w   | 指定数据库密码，指定值则使用明文输入，不指定值则命令行提示输入 |
| --cipher    |      | 是否使用密文模式输入密码，默认为 false，不使用密文模式输入密码，关于密文模式的介绍可参考[密码管理][passwd] |
| --token     |      | 指定密文文件的加密令牌<br>如果创建密文文件时未指定 token，可忽略该参数|
| --cipherfile|      | 指定密文文件路径，默认为 `~/sequoiadb/passwd` |
| --delrecord | -r   | 指定记录分隔符，默认是'\\n' |
| --type      |      | 指定导出数据格式，为 csv 或 json，默认为 csv |
| --filelimit |      | 指定单个导出文件的大小上限，单位可以为 k、K、M、m、G、g、T 或 t，默认值为 16G<br>当导出文件将超过限制时，会切分为多个文件，具有编号后缀，如 `file.csv`、`file.csv.1`、`file.csv.2` |
| --fields    |      | 指定导出集合的字段，该选项可以指定多次以指定多个导出集合的字段<br>格式为 [csName.clName:][field1[,...]]，当确定只导出一个集合时，可以仅指定字段列表 [field1[,...]] |
| --withid    |      | 强制导出或者在配置文件中生成字段时，是否包含 _id 字段 <br>当 type 为 json 时，默认为 true，包含 _id 字段；当 type 为 csv 时，默认为 false ，不包含 _id 字段 |
| --floatfmt  |      | 指定浮点数格式，默认是 '%.16g'，输入 'db2' 表示 '%+.14E'，其他格式 %[+][.precision]\(f\|e\|E\|g\|G\) |
| --ssl       |      | 指定是否使用 SSL 连接，默认 false，不使用 SSL 连接 |
| --replace   |      | 覆盖导出数据文件 |

###单集合选项###

| 选项     | 缩写 | 说明 |
| -------- | ---- | ---- |
| --csname | -c   | 导出数据的集合空间名 |
| --clname | -l   | 导出数据的集合名 |
| --file   |      | 导出的文件名 |
| --select |      | 选择规则，例如：`--select '{ age:"", address:{$trim:1} }'`<br>不能和选项 --fields 同时使用 |
| --filter |      | 导出过滤条件，例如：`--filter '{ age: 18 }'` |
| --sort   |      | 导出数据排序条件，例如：`--sort '{ name: 1 }'` |
| --skip   |      | 指定从第几条记录开始导出，默认是 0 |
| --limit  |      | 指定导出的记录数，默认值为 -1 （导出所有记录） |

> **Note:**
> 
> 导出单集合时，--select 和 --fields 选项具有一样的作用，但 --select 选项更加灵活。

###多集合选项###

| 参数名        | 缩写 | 描述 |
| ------------- | ---- | ---- |
| --cscl        |      | 导出的若干个导出集合或集合空间，多个名称使用逗号分隔，如 `--cscl cs1,cs2.cla` |
| --excludecscl |      | 不包含的集合或集合空间，类似 --cscl |
| --dir         |      | 导出的目录，导出的每一个集合对应目录中的同名文件，如 `sample.employee.csv` |

> **Note:**
> 
>* 导出工具支持单集合导出和多集合批量导出，单集合选项只能用于导出一个集合，但具有更灵活的导出条件选项，如过滤、排序。
>* 当不指定导出任何集合或者集合空间，即 -c、-l、--cscl 都不指定，则导出数据库中所有的集合。

###JSON 选项###

| 参数名          | 缩写 | 描述 |
| --------------- | ---- | ---- |
| --strict        |      | 是否严格按照数据类型导出，默认值为 false，不严格按照数据类型导出 |

###CSV 选项###

| 参数名          | 缩写 | 描述 |
| --------------- | ---- | ---- |
| --delchar       | -a   | 字符分隔符，默认值为双引号' " ' |
| --delfield      | -e   | 字段分隔符，默认值为逗号',' |
| --included      |      | 是否导出字段名到文件首行，默认值为 true，导出字段名到文件首行 |
| --includebinary |      | 是否导出完整二进制数据，默认值为 false，不导出完整二进制数据 |
| --includeregex  |      | 是否导出完整的正则表达式，默认值为 false，不导出完整的正则表达式 |
| --force         |      | 对于导出 csv 格式，每个集合必须指定对应的字段，否则不允许导出；<br>--force 选项可以强制导出，未指定字段的集合默认为第一行记录中除了 _id 以外的字段 |
| --kicknull      |      | 是否踢掉 null 值，默认为 false <br> true：输出空字符 <br> false：输出 null |
| --checkdelimeter|      | 是否严格校验分隔符，默认为 true <br>true：禁止字符分隔符、字段分隔符、记录分隔符互相包含；<br>false：允许字符分隔符、字段分隔符、记录分隔符互相包含。|

###配置文件选项###

| 参数名      | 缩写 | 描述 |
| ----------- | ---- | ---- |
| --genconf   |      | 指定一个配置文件名，将当前命令行中所指定的选项和值按照“键=值”的方式写入到配置文件，不执行导出工作 |
| --genfields |      | 生成配置文件时，是否对每一个集合生成对应的 --fields 选项，默认值为 true，工具会对每一个集合生成对应的 --fields 选项 |
| --conf      |      | 指定一个配置文件作为输入，如果命令中和配置文件中存在相同的选项，优先选择命令行中的值 |

> **Note:**
>
>* 以 csv 格式导出多集合时，必须使用 --fields 选项对每一个集合指定字段，工具提供的 --genconf 选项将每一个集合的第一行记录的字段导出到配置文件中的 --fields 选项，可以比较方便地编辑每一个集合的字段。
>* --genconf 选项将当前命令行的选项写入到配置文件中，下次使用 --conf 选项指定配置文件执行即可，这提供一种多次执行相似命令的便捷方式，另外这种方式主要用于在多集合导出 csv 情况下，对每一个集合生成对应的 --fields 选项。
>* 当使用配置文件的选项和命令行选项一样时，优先选择命令行值。

##返回值##

用户执行相关命令后，返回 0 则表示执行成功，返回非 0 则表示执行失败。

##示例##

* 以 csv 格式导出集合 sample.employee，导出文件为 `sample.employee.csv`，指定字段“field1”、“fieldNotExist”和“field3”，其中字段“fieldNotExist”在集合中不存在

    ```lang-bash
    $ sdbexprt -s localhost -p 11810 --type csv --file sample.employee.csv --fields field1,fieldNotExist,field3 -c sample -l employee
    ```

    导出的 `sample.employee.csv` 内容如下：

    ```lang-text
    field1, fieldNotExist, field3
    "Jack",,"China"
    "Mike",,"USA"
    ```

* 以 json 格式导出数据库除集合空间 cs1 和集合 cs2.cla 以外的所有集合，导出文目录为 `exportpath`

    ```lang-bash
    $ sdbexprt --type json --dir exportpath --excludecscl cs1,cs2.cla
    ```

* 以 csv 格式导出集合空间 cs2 中除 cs2.cla 外的所有集合和集合 cs1.cla；由于必须指定每一个集合的 --fields，使用 --force 选项强制导出

    ```lang-bash
    $ sdbexprt --dir exportpath --cscl cs1.cla,cs2 --excludecscl cs2.cla --force true
    ```

* 导出条件同上例，要求配置文件中包含每一个所对应的 --fields 选项，根据需求生成配置文件之后，再执行导出。

    生成配置文件

    ```lang-bash
    $ sdbexprt --dir exportpath --cscl cs1.cla,cs2 --excludecscl cs2.cla --genconf export.conf
    ```

    配置文件内容可能如下：

    ```lang-ini
    hostname = localhost
    ...
    dir = exportpath/
    cscl = cs1.cla,cs2
    excludecscl = cs2.cla
    fields = cs1.cla: a1, a2, a3
    fields = cs2.clb: b1, b2, b3
    fields = cs2.clc: c1, c2
    fields = cs2.cld: d1, d2, d3, d4
    ```

    执行导出

    ```lang-bash
    $ sdbexprt --conf export.conf
    ```

[^_^]:
     本文使用的所有引用和链接
[passwd]:manual/Distributed_Engine/Maintainance/Security/system_security.md#密码管理
