sdbimprt 是 SequoiaDB 巨杉数据库的数据导入工具，用于将 JSON 或 CSV 格式的数据导入到 SequoiaDB 数据库中。

JSON
----

JSON 格式的记录必须符合 JSON 的定义，以左右花括号作为记录的分界符，并且字符串类型的数据必须包含在两个双引号之间，转义字符为反斜杠“\\”。

CSV
----

CSV（Comma Separated Values）格式以逗号分隔数值。默认情况下记录以换行符分隔，字段以逗号分隔。用户能够指定字符串分隔符、字段分隔符以及记录分隔符。

##分隔符##

| 类型         | 默认值          |
| ------------ | --------------- |
| 字符串分隔符 | "（双引号）     |
| 字段分隔符   | ,（逗号）       |
| 记录分隔符   | '\\n'（换行符） |

>   **Note:**
>
>*   可以使用 UTF-8 字符作为分隔符；
>*   可以使用多个字符作为分隔符；
>*   可以使用 ASCII 码中的不可见字符作为分隔符，通过转义字符“\\”输入 ASCII 码的十进制数值（0~127），例如“\\30”。回车符、换行符、制表符、转义字符“\\”可以直接使用“\\r”，“\\n”，“\\t”，“\\\\”。

CSV类型
----

| 类型          | 描述 |
| ------------- | ---- |
| int32         | 十进制整型，取值范围为 -2147483648 ~ 2147483647 |
| int64         | 十进制长整型，取值范围 -9223372036854775808 ~ 9223372036854775807 |
| double        | 双精度浮点型，取值范围为 1.79E +/- 308 (15 位) |
| decimal       | 高精度数，范围为小数点前最高 131072 位;小数点后最高 16383 位。<br>可以指定精度，如 ```decimal(18, 6)``` |
| number        | 数值类型，自动判断数值的具体类型（int，long，double，decimal） |
| boolean       | 布尔型，取值可以为 true，false，t，f，yes，no，y 或 n，不区分大小写 |
| string        | 字符串。<br>可以指定长度，如 ```string(5, 30)``` |
| null          | 空值 |
| oid           | OID 类型，长度必须为 24 个字符，不支持类型自动判断 |
| date          | 日期类型，取值范围为 0000-01-01 ~ 9999-12-31，不支持类型自动判断 |
| autodate      | 日期类型，取值范围为 0000-01-01 ~ 9999-12-31，不支持类型自动判断 |
| timestamp     | 时间戳类型，取值范围为 1902-01-01-00.00.00.000000 ~ 2037-12-31-23.59.59.999999，不支持类型自动判断。<br> 可以指定格式，如``timestamp("YYYY-MM-DD HH:mm:ss")`` |
| autotimestamp | 时间戳类型，取值范围为 1902-01-01-00.00.00.000000 ~ 2037-12-31-23.59.59.999999，不支持类型自动判断 |
| binary        | 二进制类型，使用 base64 编码，不支持类型自动判断 |
| regex         | 正则表达式类型，不支持类型自动判断 |
| skip          | 忽略指定的列，该列的数据不导入到数据库 |

>   **Note:**
>
> * int32、int64、double 支持以‘#’开头的数字，例如“#123.456”。
> * double 支持科学计数法，例如“1.23e-4”，“-1.23E+4”。
> * double 支持忽略小数点前的“0”，例如“.123”。
> * 在自动判断类型时，整数超过 int64 的范围，浮点数超过 double 的范围，以及浮点数总位数超过 15 位或小数位超过 6 位时，类型判断为 decimal。
> * autodate 类型支持使用整数，表示自 1970-01-01-00.00.00.000000 以来的毫秒数, 取值范围为 int64 类型的范围。
> * autotimestamp 类型支持使用整数，表示自 1970-01-01-00.00.00.000000 以来的毫秒数，取值范围为 -2147414400000~2147443199000。

CSV类型自动判断
----

在不指定 CSV 字段类型时，导入工具会自动判断类型。其中 oid、date、timestamp、binary 和 regex 不支持自动类型判断，会被识别为 string 类型。整数超过 int64 的范围，浮点数超过 double 的范围，以及浮点数总位数超过 15 位或小数位超过 6 位时，类型判断为 decimal。例如：

| CSV 数据            | 判断类型 | 实际数据            |
| ------------------- | -------- | ------------------- |
| 123                 | int32    | 123                 |
| 123.                | int32    | 123                 |
| +123                | int32    | 123                 |
| -123                | int32    | -123                |
| 0123                | int32    | 123                 |
| #-123.              | int32    | -123                |
| 2147483648          | int64    | 2147483648          |
| 123.1               | double   | 123.1               |
| .123                | double   | 0.123               |
| 9223372036854775808 | decimal  | 9223372036854775808 |
| true                | boolean  | true                |
| false               | boolean  | false               |
| "123"               | string   | "123"               |
| 123a                | string   | "123a"              |
| "true"              | string   | "true"              |
| "false"             | string   | "false"             |
| "null"              | string   | "null"              |
| null                | null     | null                |

CSV类型转换
----

在指定 CSV 字段类型时，导入工具会将字段转换为指定的类型。如果字段的实际类型不是指定的类型，则转换可能失败。具体参考下表，最左边一列是指定的类型，Y 表示可以转换，N 表示不能转换。

| 指定类型 \\ 实际类型 | int32 | int64 | double | decimal | boolean | string | null | oid | date | timestamp | binary | regex |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| int32 | Y | 可能溢出 | 可能丢失精度 | 可能丢失精度 | Y | 支持数值字符串 | Y | N | N | N | N | N |
| int64 | Y | Y | 可能丢失精度 | 可能丢失精度 | Y | 支持数值字符串 | Y | N | N | N | N | N |
| double | Y | Y | Y | 可能丢失精度 | N | 支持数值字符串 | Y | N | N | N | N | N |
| decimal | Y | Y | Y | Y | N | 支持数值字符串 | Y | N | N | N | N | N |
| number | Y | Y | Y | Y | Y | 支持数值字符串 | Y | N | N | N | N | N |
| boolean | Y | Y | N | N | Y | 支持 Bool 字符串 | Y | N | N | N | N | N |
| string | Y | Y | Y | Y | Y | Y | Y | Y | Y | Y | Y | Y |
| null | Y | Y | Y | Y | Y | Y | Y | Y | Y | Y | Y | Y |
| oid | N | N | N | N | N | 支持 OID 字符串 | N | Y | N | N | N | N |
| date | Y | Y | N | N | N | 支持 date 字符串 | Y | N | Y | Y | N | N |
| timestamp | Y | Y | N | N | N | 支持 Timestamp 字符串 | Y | N | Y | Y | N | N |
| binary | N | N | N | N | N | 支持 Binary 字符串 | N | N | N | N | Y | N |
| regex | N | N | N | N | N | 支持 Regex 字符串 | N | N | N | N | N | Y |

> **Note:**
>
> * 指定类型为 boolean，实际类型为 int32 或 int64 时，0 值转为 false，非 0 值转为 true；
> * 指定类型为 int32 或 int64，实际类型为 boolean 时，true、t、yes 或 y 转为 1，false、f、no 或 n 转为 0；
> * 参数 --cast 可以指定数值转换时是否允许精度损失或数值溢出。

命令选项
----

###通用选项###

| 参数名      | 缩写 | 描述 |
| ----------- | ---- | ---- |
| --help      | -h   | 显示帮助信息 |
| --version   | -V   | 显示版本号 |
| --hosts     |      | 指定主机地址（hostname:svcname），用“,”分隔多个地址，默认值为 `localhost:11810` |
| --user      | -u   | 指定数据库用户名 |
| --password  | -w   | 指定数据库用户密码，如果不使用该参数指定密码，工具会通过交互式界面提示用户输入密码      |
| --cipher    |      | 是否使用密文模式输入密码，默认值为 false，不使用密文模式输入密码，关于密文模式的介绍可参考[密码管理][passwd] |
| --token     |      | 指定密文文件的加密令牌<br>如果创建密文文件时未指定 token，可忽略该参数 |
| --cipherfile|      | 指定密文文件路径，默认值为 `~/sequoiadb/passwd` |
| --csname    | -c   | 指定集合空间的名字 |
| --clname    | -l   | 指定集合的名字 |
| --errorstop |      | 遇到解析错误是否停止，默认值为 false，遇到解析错误不停止 |
| --ssl       |      | 使用 SSL 连接，默认值为 false，不使用 SSL 连接 |
| --verbose   | -v   | 显示详细的执行信息 |

###输入选项###

| 参数名         | 缩写 | 描述 |
| -------------- | ---- | ---- |
| --file         |      | 要导入的数据文件的名称，使用“,”分隔多个文件或目录。<br>如果 --file 和 --exec 都没有指定，则从标准输入读取数据 |
| --exec         |      | 执行外部程序来获取数据，外部程序必须将数据输出到标准输出 |
| --type         |      | 导入数据格式，可以是 csv 或 json，默认值为 csv |
| --linepriority |      | 指定分隔符的优先级，默认值为 auto <br>auto：当 type 为 csv 时，记录分隔符最高优先级；当 type 为 json 时，字符串分隔符最高优先级 <br>true：记录分隔符 > 字符串分隔符 > 字段分隔符 <br>false：字符串分隔符 > 记录分隔符 > 字段分隔符 |
| --delrecord    | -r   | 指定记录分隔符，默认值为换行符'\\n' |
| --force        |      | 如果数据中有非 UTF-8 的字符，强制导入数据，默认为 false，不强制导入数据|

> **Note:**
>
> * --linepriority 参数需要被特别关注，如果设置不当，可能会导入数据失败。当记录中包含记录分隔符，并且 --linepriority 为 true 时，工具会优先按照记录分隔符解析，而导致导入失败。例如：如果记录为 `{"name": "Mike\n"}`，应当设置 --linepriority 为 false；
> * 使用 --file 参数指定文件或目录时，重复出现的文件会被忽略；
> * 使用 --hosts 指定地址时，重复出现的地址会被忽略。

###导入选项###

| 参数名        | 缩写 | 描述 |
| ------------- | ---- | ---- |
| --insertnum   | -n   | 指定每次导入的记录数，取值范围为 1~100000，默认值为 1000 |
| --jobs        | -j   | 指定导入连接数（每个连接一个线程），取值范围为 1~1000，默认值为 4 |
| --parsers     |      | 指定解析任务数（每个任务一个线程），取值范围为 1~1000，默认值为 4 |
| --coord       |      | 指定是否自动查找协调节点，默认值为 true，自动查找协调节点 |
| --sharding    |      | 指定是否按分区信息重新打包记录，默认值为 true，按照分区信息重新打包信息 |
| --transaction |      | 指定导入数据时是否开启事务，默认为 false，不开启事务<br>**注意：此功能需要服务端开启事务** |
| --allowkeydup |      | 指定是否允许唯一索引的键出现重复时忽略错误继续导入，默认值为 true，忽略错误继续导入 |
| --replacekeydup |      | 指定唯一索引键重复时替换记录，默认值为 false |
| --allowidkeydup |      | 指定是否允许 $id 索引的键出现重复时忽略错误继续导入，默认值为 false |
| --replaceidkeydup |      | 指定 $id 索引键重复时替换记录，默认值为 false |

> **Note:**  
>
> 对于参数 --allowkeydup、--replacekeydup、--allowidkeydup 和 --replaceidkeydup，不支持同时设置为 true。当任意一个参数设置为 true 时，其余参数将默认为 false。

###JSON选项###

| 参数名         | 缩写 | 描述 |
| -------------- | ---- | ---- |
| --unicode      |      | 是否转义unicode字符编码（\uXXXX），默认值为 true，自动转义 Unicode 字符编码|
| --decimalto    |      | decimal 类型强制转换，默认值为 ""<br>""：不转换，保留 decimal 类型 <br>double：强制转换成 double 类型，可能会发生精度丢失 <br>string：强制转换成 string 类型 |

###CSV选项###

| 参数名         | 缩写 | 描述 |
| -------------- | ---- | ---- |
| --delchar      | -a   | 指定字符串分隔符，默认值为双引号' " ' |
| --autodelchar  |      | 针对没有字符串分隔符的 string 类型数据，指定是否自动补齐字符串分隔符，默认值为 false。当值为 true 时，会在字符串数据（包括前后的所有空格）的首尾补齐字符串符分隔符 |
| --delfield     | -e   | 指定字段分隔符，默认值为逗号',' |
| --fields       |      | 指定导入数据的字段名、类型及默认值 |
| --datefmt      |      | 指定日期格式，默认值为“YYYY-MM-DD” |
| --timestampfmt |      | 指定时间戳格式，默认值为“YYYY-MM-DD-HH.mm.ss.ffffff” |
| --trim         |      | 删除字符串左右两侧的空格（包括 ASCII 空格和 UTF-8 全角空格），<br>取值可以是 no、right、left 或 both， 默认值为 no |
| --headerline   |      | 指定导入数据首行是否作为字段名，默认值为 false，不指定首行数据为字段名 |
| --sparse       |      | 指定导入数据时是否自动添加字段名，默认值为 true，字段名按 filed1、field2 顺序增加 |
| --extra        |      | 指定导入数据中，数据的列数小于字段的列数时，是否自动添加 null 值，默认值为 false，不自动添加 |
| --cast         |      | 指定是否允许数值类型转换时丢失精度或数值溢出，默认值为 false，不允许丢失精度或数据溢出 |
| --strictfieldnum|     | 指定是否严格限制记录的字段数与定义的字段数一致，默认值为 false，不严格限制 |
| --checkdelimeter|     | 是否严格校验分隔符，默认为 true <br>true：禁止字符分隔符、字段分隔符、记录分隔符互相包含 <br>false：允许字符分隔符、字段分隔符、记录分隔符互相包含|

>   **Note:**
>
>   *   fields 语法：```fieldName [type [default <value>], ...]```
>       *   type 支持所有的 CSV 类型
>       *   type 可不写，由导入工具自动判断
>       *   指定字段可以用命令行指定，也可以在导入文件的首行指定。如果在命令行指定了 --fields，并且 --headerline 设为 true，导入工具将会优先使用命令行指定字段并且跳过导入文件的首行
>       *   字段名不能以“$”开头，中间不能有“.”，不能有不可见字符，包含空格时需要将字段名包含在单引号或双引号中
>       *   decimal 类型可以指定精度，如 ```decimal(18, 6)```
>       *   例如：```--fields='name string default "Jack", age int default 18, phone'```
>   *   string 字符串，可以指定最小长度和最大长度，最大长度如果为 0，表示不设限制，语法：```string([min length,][max length[,STRICT]])```
>       *   指定最大长度：当值超过最大长度则截断。如：```string(128)``` 指定最大长度为 128 个字符，超出则截断。  
当指定 ```STRICT``` 则不截断并输出到 rec 文件，如：```string(32,STRICT)``` 
>       *   指定最小长度：当值的长度不满足最小长度时，使用默认值代替；如果没有默认值，则用 null 代替。如：```string(10, 0)``` 指定最小长度为 10 个字符，少于 10 个字符的字段用 null 代替。  
当指定 ```STRICT``` 则使用默认值代替；如果没有默认值，则不导入记录并输出到 rec 文件，如：```string(10,0,STRICT)```
>       *   指定最大长度和最小长度：如：```string(10, 128)``` 指定最小长度为10个字符，最大长度为128个字符
>       *   默认值长度不可以小于最小长度；如果默认值长度大于最大长度，则会根据最大长度截断。如：```key string(2, 4) default address``` 指定最大长度为 4 个字符，默认值 address 会截断为 addr
>   *   datefmt 格式包括年、月、日、通配符以及特定字符
>       *   年：YYYY
>       *   月：MM
>       *   日：DD
>       *   通配符：*
>       *   特定字符：任意 UTF-8 字符
>       *   其中年、月、日必须是整数，并且符合日期类型的范围
>       *   指定通配符时，日期字段上对应的位置可以为任意字符
>       *   指定特定字符时，日期字段上对应的位置必须为该指定字符
>       *   例如需要导入的数据中日期格式为“3/15, 2015”，则设置 ```--datefmt="MM/DD, YYYY"``` 与该格式匹配
>   *   timestamp 格式包括年、月、日、时、分、秒、微秒或毫秒、通配符以及特定字符
>       *   年：YYYY
>       *   月：MM
>       *   日：DD
>       *   时：HH
>       *   分：mm
>       *   秒：ss
>       *   微秒：ffffff
>       *   毫秒：SSS
>       *   时区：Z
>       *   通配符：*
>       *   特定字符：任意 UTF-8 字符
>       *   其中年、月、日、时、分、秒、微秒、毫秒必须是整数，并且符合时间戳类型的范围
>       *   微秒和毫秒不能同时出现，只能出现其中一个
>       *   指定通配符时，时间戳字段上对应的位置可以为任意字符
>       *   指定特定字符时，时间戳字段上对应的位置必须为该指定字符
>       *   例如需要导入的数据中时间戳格式为“3/15/2015 T 12.30.123”，则设置 ```--timestampfmt="MM/DD/YYYY T mm.ss.SSS"``` 与该格式匹配
>       *   例如指定带时区的时间戳：``--timestampfmt="YYYY-MM-DD HH.mm.ssZ"``
>       *   例如指定东八区时间戳： ``--timestampfmt="YYYY-MM-DD HH.mm.ss+0800"``

##结果字段解析##

导入操作完成后，将会输出如下结果字段：

| 字段名 | 描述 |
| ------ | ---- | 
| Parsed records | 解析成功的记录条数 |
| Parsed failure | 解析失败的记录条数 |
| Sharding records | 根据分区信息打包成功的记录条数 |
| Sharding failure | 根据分区信息打包失败的记录条数 |
| Imported records | 导入成功的记录条数 |
| Imported failure | 导入失败的记录条数 |
| Duplicated records | 因唯一索引键冲突而被替换或被忽略的记录条数 |

> **Note:**
>
> Sharding records 和 Sharding failure 仅在导入集合为分区集合且参数 --sharding 为 true 时统计。

示例
----

- 数据文件 `test.csv` 存在如下记录：

   ```lang-text
   name string default "Anonymous", age int, country
   "Jack",18,"China"
   "Mike",20,"USA"
   ```

   将数据通过协调节点导入至集合 sample.employee 中

    ```lang-bash
    $ sdbimprt --hosts=localhost:11810 --type=csv --file=test.csv -c sample -l employee --headerline=true
    ```

- 数据文件  `test.csv` 存在以下记录：

    ```lang-text
    "Jack",18,"China"
    "Mike",20,"USA"
    ```

   将数据导入到本地数据库 11810 中的集合 sample.employee

    ```lang-bash
    $ sdbimprt --hosts=localhost:11810 --type=csv --file=test.csv -c sample -l employee --fields='name string default "Anonymous", age int, country'
    ```

-  数据文件 `test.csv` 存在以下记录，其中文件第一行是字段定义，需要跳过：

    ```lang-text
    name, age, country
    "Jack",18,"China"
    "Mike",20,"USA"
    ```

   将数据导入到本地数据库 11810 中的集合 sample.employee

    ```lang-bash
    $ sdbimprt --hosts=localhost:11810 --type=csv --file=test.csv -c sample -l employee --fields='name string default "Anonymous", age int, country' --headerline=true
    ```

- 将目录 `../data` 中的所有文件以 csv 格式导入至集合 sample.employee 

    ```lang-bash
    $ sdbimprt --hosts=localhost:11810 --type=csv --file=../data -c sample -l employee --fields='name string default "Anonymous", age int, country'
    ```

- 将目录 `../data` 中的所有文件以及 `./sample_employee_data.csv` 以 csv 格式导入至集合 sample.employee 中；有 11810 和 11910 两个协调节点，记录中时间戳类型的数据类似于“2015-10-01 T 12.31.15.123 T”，使用两个连接同时导入

    ```lang-bash
    $ sdbimprt --type=csv --file=../data,./sample_employee_data.csv --fields='name, time timestamp' -c sample -l employee --timestampfmt="YYYY-MM-DD T HH.mm.ss.SSS T" --hosts=localhost:11810,localhost:11910 -j 2
    ```

- 通过管道从其它工具 other 获取数据，并以 json 格式导入至集合 sample.employee 中

    ```lang-bash
    $ other | sdbimprt --hosts=localhost:11810 --type=json -c sample -l employee 
    ```
- 导入多种时间戳格式，以系统时区是东八区为例，如下是导入文件的内容：

    ```lang-text
    2014-01-01, 2001/01/01, 1990-01-01
    2014-01-01Z, 2001/01/01Z, 1990-01-01Z
    2014-01-01+0200, 2001/01/01+0200, 1990-01-01+0200
    ```

   将导入文件以 csv 格式导入至集合 sample.employee 中

    ```lang-bash
    $ sdbimprt --hosts=localhost:11810 --type=csv --file=test.csv -c sample -l employee --fields='time1 timestamp("YYYY-MM-DD"), time2 timestamp("YYYY/MM/DDZ"), time3 timestamp("YYYY-MM-DD+0600")'
    ```

   > **Note:**
   >
   > - time1没有指定时区，因此都用系统的时区。
   > - time2指定时区字符Z，如果数据没有时区信息，则用系统的时区；如果数据有Z字符，则作为 UTC 时间。
   > - time3指定+0600时区，如果数据没有时区信息，则用字段指定的+0600时区；如果数据有Z字符，则作为 UTC 时间。

    查询结果如下：

    ```lang-bash
    > db.sample.employee.find()
    {
       "_id": {
          "$oid": "5ad5565f13f513e620000000"
       },
       "time1": {
          "$timestamp": "2014-01-01-00.00.00.000000"
       },
       "time2": {
          "$timestamp": "2001-01-01-00.00.00.000000"
       },
       "time3": {
          "$timestamp": "1990-01-01-02.00.00.000000"
       }
    }
    {
       "_id": {
          "$oid": "5ad5565f13f513e620000001"
       },
       "time1": {
          "$timestamp": "2014-01-01-00.00.00.000000"
       },
       "time2": {
          "$timestamp": "2001-01-01-08.00.00.000000"
       },
       "time3": {
          "$timestamp": "1990-01-01-08.00.00.000000"
       }
    }
    {
       "_id": {
          "$oid": "5ad5565f13f513e620000002"
       },
       "time1": {
          "$timestamp": "2014-01-01-00.00.00.000000"
       },
       "time2": {
          "$timestamp": "2001-01-01-06.00.00.000000"
       },
       "time3": {
          "$timestamp": "1990-01-01-06.00.00.000000"
       }
    }
    ```


[^_^]:
     本文使用的所有引用和链接
[passwd]:manual/Distributed_Engine/Maintainance/Security/system_security.md#密码管理