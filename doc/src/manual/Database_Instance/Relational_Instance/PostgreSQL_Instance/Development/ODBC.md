[^_^]:
    ODBC驱动
    作者：赵育
    时间：20190817
    评审意见
    王涛：时间：
    许建辉：时间：
    市场部：时间：20190905

本文档将介绍如何配置数据源及通过 ODBC 驱动访问 PostgreSQL。

配置数据源
----

用户下载安装 [ODBC 驱动][download]后，需要配置数据源才可以使用。下述以 Windows 系统为例介绍配置步骤。

1. 打开【控制面板】->【管理工具】->【数据源（ODBC）】，点击 **添加** 按钮

 ![添加数据源]

2. 选择 PostgreSQL Unicode，点击 **完成** 按钮

 ![创建数据源]

3. 设置数据源，按图中提示完成选项

 ![配置数据源]

4. 点击 **Test** 按钮，提示连接成功后，点击 **Save** 按钮保存配置

示例
----

以下示例通过 C# 对接 ODBC 进行基础的增加和查询操作。

1. 连接到本地 PostgreSQL 实例并准备数据库 sample 和表 test，将其映射到 SequoiaDB 已存在的集合 sample.employee

   ```lang-bash
   $ bin/psql -p 5432 sample
   psql (9.3.4)
   Type "help" for help.
   
   sample=# create foreign table test (name text, id numeric) server sdb_server options (collectionspace 'sample', collection 'employee', decimal 'on');
   CREATE FOREIGN TABLE
   sample=# \q
   ```

2. 添加数据源，配置 DSN(Data Source Name) 为"PostgreSQL35W"，并配置 Database 为"sample"，具体步骤见上一节

3. 新建项目，以 Visual studio 2013 开发环境为例，点击工具栏【文件】->【新建】->【项目】，新建一个 Visual C# 的控制台应用程序

4. 输入示例代码

   ```lang-cs
   using System;
   using System.Collections.Generic;
   using System.Linq;
   using System.Text;
   using System.Data.Odbc;
   
   namespace ConsoleApplication
   {
       class Program
       {
           static void Main(string[] args)
           {
               string connStr = "DSN=PostgreSQL35W";
               OdbcConnection conn = new OdbcConnection(connStr);
               conn.Open();
   
               // Insert
               OdbcCommand cmd = conn.CreateCommand();
               cmd.CommandText = "INSERT INTO test(name, id) VALUES ('Steven', 1)";
               cmd.ExecuteNonQuery();
   
               // Query
               cmd.CommandText = "SELECT * FROM test";
               OdbcDataReader odr = cmd.ExecuteReader();
               while (odr.Read())
               {
                   for (int i = 0; i < odr.FieldCount; i++)
                   {
                       Console.Write("{0} ", odr[i]);
                   }
                   Console.WriteLine();
               }
               conn.Close();
   
               Console.Read();
           }
       }
   }
   ```

5. 点击 【调试】->【开始执行】，得到运行结果

   ```lang-text
   Steven 1
   ```


[^_^]:
     本文使用的所有引用和链接
[download]:manual/Database_Instance/Relational_Instance/PostgreSQL_Instance/Development/engine_download.md
[添加数据源]:images/Database_Instance/Relational_Instance/PostgreSQL_Instance/Development/add.png
[创建数据源]:images/Database_Instance/Relational_Instance/PostgreSQL_Instance/Development/create.png
[配置数据源]:images/Database_Instance/Relational_Instance/PostgreSQL_Instance/Development/config.png
