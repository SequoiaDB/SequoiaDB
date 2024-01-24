
用户通过点击导航【数据】->【数据库实例的名字】，可以进入 PostgreSQL 实例操作页面。在 PostgreSQL 实例操作页面可以进行创建数据库、删除数据库、创建数据表、删除数据表等操作。

![SequoiaSQL-PostgreSQL数据库][database_oltp]

创建数据库
----

点击右下角 **创建数据库**，填写数据库名后点击 **确定** 按钮
![创建数据库][add_database]

创建数据表 
----

创建数据表可以选择创建普通表和外部表，创建外部表需要当前数据库已经进行了[添加 PostgreSQL 实例存储][deployment_postgresql]操作。

点击右下角 **创建数据表** 按钮，根据实际需要填写参数，如果选择创建外部表的话，则需要选择服务名，填写完毕点击 **确定** 按钮
![创建数据表][create_table]

   > **Note:**
   >
   > - 创建完数据表后，可点击数据表名进入[数据操作][record]。
   > - 创建外部表不可定义主键和唯一键。

删除数据库
----

点击左下角删除数据库，选择需要删除的数据库后点击 **确定** 按钮
![删除数据库][drop_database]

   > **Note:**  
   > - 无法删除当前打开的数据库。  
   > - 建议不要删除 postgres 库。
   > - 通过 SAC 删除数据库，至少保留有一个数据库。

删除数据表
----
从数据表列表中选择需要删除的表，点击行中的【X】符号，确认后点击 **确定** 按钮
![删除数据表][drop_table]

   > **Note:**
   >  
   > 系统表不可删除。

[^_^]:
    本文使用的所有引用及链接
[deployment_postgresql]:manual/SAC/Deployment/Deployment_Bystep/deployment_postgresql.md#添加PostgreSQL实例存储
[record]:manual/SAC/Operation/Postgresql/record.md

[database_oltp]:images/SAC/Operation/Postgresql/database_oltp.png
[create_table]:images/SAC/Operation/Postgresql/create_table.png
[add_database]:images/SAC/Operation/Postgresql/add_database.png
[drop_database]:images/SAC/Operation/Postgresql/drop_database.png
[drop_table]:images/SAC/Operation/Postgresql/drop_table.png