[^_^]:
    数据导出
    作者：杨垚
    时间：20190424
    王涛：
    许建辉：
    市场部：20190513


SequoiaDB 巨杉数据库支持将集合中的数据导出到 UTF-8 编码的 CSV 格式或者 JSON 格式的数据存储文件。

数据导出CSV
----
CSV（Comma-Separated Values）是一种最为常见的数据库间通用数据交换格式标准之一，以逗号作为字段分隔符，空行作为记录分隔符，并以纯文本形式存储的表格数据文件。CSV 格式定义在 [RFC 4180][rfc4180] 文档中进行了详细描述。  

用户可以使用 SequoiaDB 巨杉数据库提供的 [sdbexprt 工具][sdbexprt]将[集合][collection]中的数据导出到 CSV 数据存储文件中。

### 数据准备

集合 info.user_info 存在以下数据：

```lang-bash
$ sdb 'db.info.user_info.find()'
{
  "_id": {
    "$oid": "5cd2dc7b294ffa8385000000"
  },
  "id": 1,
  "name": "Jack",
  "age": 18,
  "identity": "student",
  "phone_number": "18921222226",
  "email": "jack@example.com",
  "country": "China"
}
{
  "_id": {
    "$oid": "5cd2dc7b294ffa8385000001"
  },
  "id": 2,
  "name": "Mike",
  "age": 20,
  "identity": "student",
  "phone_number": "18923244255",
  "email": "mike@example.com",
  "country": "USA"
}
{
  "_id": {
    "$oid": "5cd2dc7b294ffa8385000002"
  },
  "id": 3,
  "name": "Woody",
  "age": 25,
  "identity": "worker",
  "phone_number": "18945253245",
  "email": "woody@example.com",
  "country": "China"
}
Return 3 row(s).
```

### 数据导出

- **指定 sdbexprt 参数数据导出**

   1. 以指定 sdbexprt 参数的方式将集合 info.user_info 用户信息数据导出到 `user_info.csv` 文件中

     ```lang-bash
     $ sdbexprt -s localhost -p 11810 --type csv --file user_info.csv -c info -l user_info --fields id,name,age,identity,phone_number,email,country 
     ```

   2. 查看 `user_info.csv` 文件中的用户信息数据

     ```lang-bash
     $ cat user_info.csv 
     ```

     输出结果如下：

     ```lang-text
     id,name,age,identity,phone_number,email,country
     1,"Jack",18,"student","18921222226","jack@example.com","China"
     2,"Mike",20,"student","18923244255","mike@example.com","USA"
     3,"Woody",25,"worker","18945253245","woody@example.com","China"
     ```

   > **Note**:
   >
   > - 在导出数据时，如需增加记录中不存在的字段时，可在参数 --fields 中增加需添加字段名称，导出工具会自动默认为空值。
   > - --filter 参数支持对需导出字段的值进行过滤。
   > - 更多参数说明详见 [sdbexprt 工具][sdbexprt]介绍。

- **使用参数配置文件数据导出**

    1. 编辑配置文件 `export.conf`

     ```lang-ini
     hostname = localhost 
     svcname = 11810 
     user = sdbadmin 
     password = admin 
     type = csv 
     file = user_info.csv 
     csname = info 
     clname = user_info 
     fields = id,name,age,identity,phone_number,email,country
     ```

   2. 以使用参数配置文件的方式将集合 info.user_info 用户信息数据导出到 `user_info.csv` 文件中

     ```lang-bash
     $ sdbexprt --conf export.conf
     ```

   3. 查看 `user_info.csv` 文件中的用户信息数据

     ```lang-bash
     $ cat user_info.csv 
     ```

     输出结果如下：

     ```lang-text
     id,name,age,identity,phone_number,email,country
     1,"Jack",18,"student","18921222226","jack@example.com","China"
     2,"Mike",20,"student","18923244255","mike@example.com","USA"
     3,"Woody",25,"worker","18945253245","woody@example.com","China"
     ```

   > **Note：**
   >
   > 配置文件中参数说明详见 [sdbexprt 工具][sdbexprt]介绍

数据导出JSON
----

用户可以使用 SequoiaDB 巨杉数据库提供的 [sdbexprt 工具][sdbexprt]将集合中的数据导出到 JSON 数据存储文件中。

### 数据准备

集合 info.user_info 存在以下数据：

```lang-bash
$ sdb 'db.info.user_info.find()'
{
  "_id": {
    "$oid": "5cd2dc7b294ffa8385000000"
  },
  "id": 1,
  "name": "Jack",
  "age": 18,
  "identity": "student",
  "phone_number": "18921222226",
  "email": "jack@example.com",
  "country": "China"
}
{
  "_id": {
    "$oid": "5cd2dc7b294ffa8385000001"
  },
  "id": 2,
  "name": "Mike",
  "age": 20,
  "identity": "student",
  "phone_number": "18923244255",
  "email": "mike@example.com",
  "country": "USA"
}
{
  "_id": {
    "$oid": "5cd2dc7b294ffa8385000002"
  },
  "id": 3,
  "name": "Woody",
  "age": 25,
  "identity": "worker",
  "phone_number": "18945253245",
  "email": "woody@example.com",
  "country": "China"
}
Return 3 row(s).
```

### 数据导出

- **指定 sdbexprt 参数数据导出**

   1. 以指定 sdbexprt 参数的方式将集合 info.user_info 用户信息数据导出到 `user_info.csv` 文件中

     ```lang-bash
     $ sdbexprt -s localhost -p 11810 --type json --file user_info.json -c info -l user_info --fields id,name,age,identity,phone_number,email,country 
     ```

   2. 查看 `user_info.json` 文件中的用户信息数据

     ```lang-bash
     $ cat user_info.json 
     ```

     输出结果如下：

     ```lang-json
     { "id": 1, "name": "Jack", "age": 18, "identity": "student", "phone_number": "18921222226", "email": "jack@example.com", "country": "China" }
     { "id": 2, "name": "Mike", "age": 20, "identity": "student", "phone_number": "18923244255", "email": "mike@example.com", "country": "USA" }
     { "id": 3, "name": "Woody", "age": 25, "identity": "worker", "phone_number": "18945253245", "email": "woody@example.com", "country": "China" }
     ```

   > **Note**:
   >
   > - --filter 参数支持对需导出字段的值进行过滤
   > - 更多参数说明详见 [sdbexprt 工具][sdbexprt]介绍

- **使用参数配置文件数据导出** 

   1. 编辑配置文件 `export.conf`

     ```lang-ini
     hostname = localhost 
     svcname = 11810 
     user = sdbadmin 
     password = admin 
     type = json 
     file = user_info.json 
     csname = info 
     clname = user_info 
     fields = id,name,age,identity,phone_number,email,country
     ```

   2. 以使用参数配置文件的方式将集合 info.user_info 用户信息数据导出到 `user_info.csv` 文件中

     ```lang-bash
     $ sdbexprt --conf export.conf
     ```

   3. 查看 `user_info.json` 文件中的用户信息数据

     ```lang-bash
     $ cat user_info.json 
     ```

     输出结果如下：

     ```lang-json
     { "id": 1, "name": "Jack", "age": 18, "identity": "student", "phone_number": "18921222226", "email": "jack@example.com", "country": "China" }
     { "id": 2, "name": "Mike", "age": 20, "identity": "student", "phone_number": "18923244255", "email": "mike@example.com", "country": "USA" }
     { "id": 3, "name": "Woody", "age": 25, "identity": "worker", "phone_number": "18945253245", "email": "woody@example.com", "country": "China" }
     ```

   > **Note：**
   >
   > 配置文件中参数说明详见 [sdbexprt 工具][sdbexprt]介绍

[^_^]:
    本文档使用到的链接或引用：
    TODO：待补充sdbimprt和sdbexprt工具的文档介绍的链接或引用

[sdbimprt]:manual/Distributed_Engine/Maintainance/Mgmt_Tools/sdbimprt.md
[sdbexprt]:manual/Distributed_Engine/Maintainance/Mgmt_Tools/sdbexprt.md
[csv_data_type]:manual/Distributed_Engine/Maintainance/Mgmt_Tools/sdbimprt.md#csv_data_type
[collection_space]:manual/Distributed_Engine/Architecture/Data_Model/collection_space.md
[collection]:manual/Distributed_Engine/Architecture/Data_Model/collection.md
[rfc4180]:https://www.rfc-editor.org/rfc/rfc4180.txt
