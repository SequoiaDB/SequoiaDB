[^_^]:
    CSV数据文件导入
    作者：谢建宏
    时间：20190313
    王涛：20190316
    许建辉：
    市场部：20190325


CSV（Comma-Separated Values）是一种最为常见的数据库间通用数据交换格式标准之一，以逗号作为字段分隔符，空行作为记录分隔符，并以纯文本形式存储的表格数据文件。CSV 格式定义在 [RFC 4180][rfc4180] 文档中进行了详细描述。 

用户可以使用 SequoiaDB 巨杉数据库提供的 [sdbimprt 工具][sdbimprt]将以下数据导入 SequoiaDB 的集合中：

- 从其他数据库导出的 CSV 数据
- [sdbexprt 工具][sdbexprt] 导出的 CSV 数据
- 用户程序生成的 CSV 数据

> **Note**：
>
> 在导入数据前，用户需要确保 CSV 文件编码格式为 UTF-8。

数据准备
----

- 以下是首行为字段定义的 CSV 数据文件 `data1.csv` 中的三条用户信息数据：

   ```lang-text
   id,name,age,identity,phone_number,email,country
   1,"Jack",18,"student","18921222226","jack@example.com","China"
   2,"Mike",20,"student","18923244255","mike@example.com","USA"
   3,"Woody",25,"worker","18945253245","woody@example.com","China"
   ```

- 以下是 CSV 数据文件 `data2.csv` 中的三条用户信息数据：

   ```lang-text
   1,"Jack",18,"student","18921222226","jack@example.com","China"
   2,"Mike",20,"student","18923244255","mike@example.com","USA"
   3,"Woody",25,"worker","18945253245","woody@example.com","China"
   ```

- 以下是以“|”作为字段分隔符的 CSV 数据文件 `data3.csv` 中的三条用户信息数据：

   ```lang-text
   1|"Jack"|18|"student"|"18921222226"|"jack@example.com"|"China"
   2|"Mike"|20|"student"|"18923244255"|"mike@example.com"|"USA"
   3|"Woody"|25|"worker"|"18945253245"|"woody@example.com"|"China"
   ```

数据导入
----

### 以数据文件首行作为字段定义 ###

将上述示例 CSV 数据文件 `data1.csv` 中的用户信息数据导入集合 info.user_info 中

```lang-bash
sdbimprt --hosts "localhost:11810" --type csv --csname info --clname user_info --headerline true --file data1.csv
```

> **Note**:
>
> + 数据文件的数据量较大时，可使用 `-n` 指定每次导入的记录数以及 `-j` 指定导入连接数来提供导入效率
> + --file 参数支持指定多个文件或者目录，使用逗号“,”分隔，重复出现的文件会被忽略
> + 更多参数说明详见 [sdbimprt 工具][sdbimprt]介绍

### 命令行指定字段定义 ###

将上述示例 CSV 数据文件 `data2.csv` 中的用户信息数据导入集合 info.user_info 中

```lang-bash
sdbimprt --hosts "localhost:11810" --type csv --csname info --clname user_info --fields 'id long, name string default "Anonymous", age int, identity, phone_number, email, country' --file data2.csv
```

> **Note：**
>
> - fields语法：`fieldName [type [default <value>], ...]`
> - 更多 CSV 格式说明详见 [CSV 数据类型][csv_data_type]

### 自定义字段分隔符 ###

将上述示例 CSV 数据文件 `data3.csv` 中的用户信息数据导入集合 info.user_info 中

```lang-bash
sdbimprt --hosts "localhost:11810" --type csv --csname info --clname user_info --fields 'id long, name string default "Anonymous", age int, identity, phone_number, email, country' --delfield '|' --file data3.csv
```

> **Note：**
>
> sdbimprt 工具支持使用 --delchar 参数自定义字符串分隔符和 --delrecord 参数自定义记录分隔符。

### 字段的值存在换行符

将上述示例 CSV 数据文件 `data4.csv` 中的用户信息数据导入集合 info.user_info 中

```lang-bash
sdbimprt --hosts "localhost:11810" --type csv --csname info --clname user_info --fields 'id long, name string default "Anonymous", age int, identity, phone_number, email, country, address' --linepriority false --file data4.csv
```

> **Note:**
>
> --linepriority 参数的作用是设置分隔符的优先级，默认值为 true
> - 值为 true 时，分隔符的优先级为：记录分隔符，字符串分隔符，字段分隔符
> - 值为 false 时，分隔符的优先级为：字符串分隔符，记录分隔符，字段分隔符

小结
----

SequoiaDB 的 sdbimprt 工具支持并发导入单一的 CSV 数据文件或批量导入 CSV 数据文件目录。用户使用该工具能简单快速地将 CSV 数据导入 SequoiaDB。

[^_^]:
    本文档使用到的链接或引用：
    TODO：待补充sdbimprt和sdbexprt工具的文档介绍的链接或引用

[sdbimprt]:manual/Distributed_Engine/Maintainance/Mgmt_Tools/sdbimprt.md
[sdbexprt]:manual/Distributed_Engine/Maintainance/Mgmt_Tools/sdbexprt.md
[csv_data_type]:manual/Distributed_Engine/Maintainance/Mgmt_Tools/sdbimprt.md#csv_data_type
[rfc4180]:https://www.rfc-editor.org/rfc/rfc4180.txt
