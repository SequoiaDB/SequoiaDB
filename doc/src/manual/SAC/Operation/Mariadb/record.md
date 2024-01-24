
用户在 MariaDB 实例数据操作页面可以进行增删改查以及执行 SQL 操作。

在查询、更新和删除操作弹窗中用户可以选择执行模式：
- 当执行模式为“马上执行”时，点击确定将会马上执行所做的操作
- 当执行模式为“仅生成 SQL”时，点击确定会在页面的 SQL 执行框中按填写的条件生成 SQL 语句

## 执行 SQL
 
在页面中的 SQL 执行框可输入需要执行的 SQL 语句进行执行
![执行SQL][exec_sql]

> **Note:**
>
> 如果用户需要做复杂的查询、插入、删除和更新操作，建议使用 SQL 执行框代替弹窗，填写 SQL 后进行操作。

## 查询操作

点击 **查询** 按钮，填写查询条件后点击 **确定** 按钮执行查询

![查询][query]

## 插入操作

点击 **插入** 按钮，填写插入参数后点击 **确定** 按钮执行插入

![插入][insert]

## 更新操作

点击 **更新** 按钮，填写更新的条件后点击 **确定** 按钮执行更新

![更新][update]

## 删除操作

点击 **删除** 按钮，填写删除条件后点击 **确定** 按钮执行删除

![更新][update]

## 查看执行记录

查看执行记录可以查看当前页面所执行过的 SQL 语句，包括使用增删改查弹窗进行的操作。点击语句可粘贴到 SQL 执行框。
![执行记录][history]



[^_^]:
     本文使用的所有引用及链接
[exec_sql]:images/SAC/Operation/Mariadb/mariadb_operation_1.png
[query]:images/SAC/Operation/Mariadb/mariadb_operation_2.png
[insert]:images/SAC/Operation/Mariadb/mariadb_operation_3.png
[update]:images/SAC/Operation/Mariadb/mariadb_operation_4.png
[update]:images/SAC/Operation/Mariadb/mariadb_operation_5.png
[history]:images/SAC/Operation/Mariadb/mariadb_operation_6.png
