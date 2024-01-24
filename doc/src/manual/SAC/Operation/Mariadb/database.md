
用户通过点击导航【数据】->【数据库实例的名字】，可以进入 MariaDB 实例操作页面。在 MariaDB 实例操作页面可以进行创建数据库、删除数据库、创建数据表、删除数据表等操作。

![SequoiaSQL-MariaDB数据库][database]

创建数据库
----

点击 **创建数据库** 按钮，填写数据库名后点击 **确定** 按钮
![创建数据库][create_database]

创建数据表 
----

创建数据表可以选择存储引擎，在 SAC 中创建默认为 SequoiaDB 引擎，确保数据表创建成功需提前查看是否已经[添加 MariaDB 实例存储][deployment_mariadb]。

点击 **创建数据表** 按钮，根据实际需要填写参数后点击 **确定** 按钮

   ![创建数据表][create_table_normal]


> **Note:**
>  
> - 创建完数据表后，可点击数据表名进入[数据操作][record]。
> - 成功创建数据表时会在对应的 SequoiaDB 服务创建集合空间和集合。

删除数据库
----
点击 **删除数据库** 按钮，选择需要删除的数据库后点击 **确定** 按钮

![删除数据库][drop_database]

删除数据表
----

从数据表列表中选择需要删除的表，点击行中的【X】符号，确认后点击 **确定** 按钮
![删除数据表][drop_table]

> **Note:**
>  
> 系统表不可删除。

[^_^]:
    本文使用的所有引用和链接
[deployment_mariadb]:manual/SAC/Deployment/Deployment_Bystep/deployment_mariadb.md#添加MariaDB实例存储
[record]:manual/SAC/Operation/Mariadb/record.md
[database]:images/SAC/Operation/Mariadb/mariadb_database_1.png
[create_database]:images/SAC/Operation/Mariadb/mariadb_database_2.png
[create_table_normal]:images/SAC/Operation/Mariadb/mariadb_database_3.png
[drop_database]:images/SAC/Operation/Mariadb/mariadb_database_4.png
[drop_table]:images/SAC/Operation/Mariadb/mariadb_database_5.png
