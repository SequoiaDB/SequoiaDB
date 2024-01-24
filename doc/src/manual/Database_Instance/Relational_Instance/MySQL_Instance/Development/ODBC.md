[^_^]:
    MySQL 实例-ODBC 驱动

本文档将介绍驱动安装配置及对接 ODBC 驱动示例。

##驱动安装配置##

用户下载 [ODBC 驱动][download] 后，需安装配置才能使用。下述以 Windows 系统为例介绍安装配置步骤。

> **Note:**
>
> 不同版本 Windows 间命名与界面可能存在差异。

1. 安装 MySQL ODBC 驱动，双击 msi 文件，根据指示信息完成安装

2. 添加数据源，找到【控制面板】->【管理工具】->【数据源(ODBC)】并打开，点击 **添加** 按钮

 ![add_ds]

3. 选择驱动程序，驱动程序提供 ANSI 和 Unicode 两个版本，一般推荐使用支持更多字符的 Unicode 版本

 ![choose_driver]

4. 配置数据源，输入 MySQL 有关信息

 ![config_ds]

##示例##

以下示例通过 C# 对接 ODBC 进行增删改查的基本操作。

1. 连接到 MySQL 实例并准备样例使用的数据库 db 和表 tb 

    ```lang-sql
    CREATE DATABASE db;
    USE db;
    CREATE TABLE tb (id INT, first_name VARCHAR(128), last_name VARCHAR(128));
    ```

2. 添加数据源，配置 DSN（Data Source Name）为"SequoiaSQL-MySQL"，并配置 Database 为"db"，具体步骤见上一节

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
                string connStr = "DSN=SequoiaSQL-MySQL";
                OdbcConnection conn = new OdbcConnection(connStr);
                conn.Open();
    
                Console.WriteLine("---INSERT---");
                OdbcCommand cmd = conn.CreateCommand();
                cmd.CommandText = "INSERT INTO tb(id, first_name, last_name) VALUES (1, 'Peter', 'Packer')";
                cmd.ExecuteNonQuery();
    
                Console.WriteLine("---UPDATE---");
                cmd.CommandText = "UPDATE tb SET first_name = 'Tony' WHERE id = 1";
                cmd.ExecuteNonQuery();
    
                Console.WriteLine("---SELECT---");
                cmd.CommandText = "SELECT * FROM tb";
                OdbcDataReader odr = cmd.ExecuteReader();
                while (odr.Read())
                {
                    for (int i = 0; i < odr.FieldCount; i++)
                    {
                        Console.Write("{0}\t", odr[i]);
                    }
                    Console.WriteLine();
                }
                conn.Close();
    
                Console.WriteLine("---DELETE---");
                cmd.CommandText = "DELETE FROM tb WHERE id = 1";
                cmd.ExecuteNonQuery();
    
                Console.Read();
            }
        }
    }
    ```

5. 点击【调试】->【开始执行】，得到运行结果

    ```lang-text
    ---INSERT---
    ---UPDATE---
    ---SELECT---
    1       Tony    Parker
    ---DELETE---
    ```

[^_^]:
     本文使用的所有引用和链接
[download]:manual/Database_Instance/Relational_Instance/MySQL_Instance/Development/engine_download.md
[add_ds]:images/Database_Instance/Relational_Instance/MySQL_Instance/Development/add_ds.png
[choose_driver]:images/Database_Instance/Relational_Instance/MySQL_Instance/Development/choose_driver.png
[config_ds]:images/Database_Instance/Relational_Instance/MySQL_Instance/Development/config_ds.png
