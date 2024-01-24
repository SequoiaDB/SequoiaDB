本文档主要介绍数据库连接配置、创建 JDBC 提供程序和创建数据源。

##数据库连接配置##

服务启动成功后，通过浏览器登录控制台，输入【用户标识】和【密码】，点击 **登录**

   > **Note:**  
   >
   > - URL 中的 IP 必须为 websphere 服务器 IP。  
   > - 用户标识和密码为安装时设置的用户名和密码。   
   > - 下述示例中的主机名以 suse113-3Node01 为例。

   ![ds-1][ds_1]

##创建JDBC提供程序##

1. 选择【资源】->【JDBC】->【JDBC提供程序】，在【作用域】下拉列表中选择【节点=suse113-2Node01】

   ![ds-2][ds_2]

2. 点击 **新建**

   ![ds-3][ds_3]

3.  【】数据库类型】选择【用户定义的】，【实现类名】输入“org.postgresql.jdbc2.optional.ConnectionPool”，【名称】输入“sdb JDBC Provider”，点击 **下一步**

   ![ds-4][ds_4]

4. 输入 “/opt/postgresql-9.3-1102.jdbc41.jar”（文件在服务器 `/opt` 目录下必须存在），点击 **下一步**

   ![ds-5][ds_5]

5. 点击 **完成**

   ![ds-6][ds_6]

##创建数据源##

1. 点击 **sdb JDBC Provider**

   ![ds-7][ds_7]

2. 进入【sdb JDBC Provider】页面，点击 **数据源**

   ![ds-8][ds_8]

3. 进入【数据源】页面，点击 **新建**

   ![ds-9][ds_9]

4. 【数据源名】输入“sdb DataSource”，【JNDI 名称】输入“jdbc/sdb DataSource”，点击 **下一步**

   ![ds-10][ds_10]

5. 点击 **下一步**

   ![ds-11][ds_11]

6. 点击 **下一步**

   ![ds-12][ds_12]

7. 点击 **完成**

   ![ds-13][ds_13]

8. 点击 **sdb DataSource**

   ![ds-28][ds_28]

9. 点击 **JAAS － J2C 认证数据**

   ![ds-14][ds_14]

10. 点击 **新建**

   ![ds-15][ds_15]

11. 输入别名、用户标识、密码， 点击 **确定**  

   > **Note:**  
   >
   > 这里的用户标识和密码是安装 SequoiaSQL 时的用户名和密码；   

   ![ds-16][ds_16]

12. 点击 **sdb DataSource**，返回上一级页面

   ![ds-17][ds_17]

13. 点击 **定制属性**，进入定制属性页面

   ![ds-19][ds_19]

14. 选择配置【databaseName】、【password】和【portNumber】

   ![ds-20][ds_20]

15. 翻页，选择配置【serverName】和【user】

   ![ds-21][ds_21]

16. 点击属性名（如【databaseName】）进入配置页面配置对应属性值，修改后显示如下：

   ![ds-22][ds_22]

17. 点击 **保存** 到主配置，保存配置

   ![ds-23][ds_23]

18. 点击 **数据源** ，勾选新建的数据源，点击 **测试连接**

   ![ds-24][ds_24]

19. 在页面顶部显示测试连接成功，数据源配置成功

   ![ds-25][ds_25]


[^_^]:
     本文使用的所有引用和链接
[ds_1]:images/Manual/Webserverapp/Websphere/ds_1.jpg
[ds_2]:images/Manual/Webserverapp/Websphere/ds_2.jpg
[ds_3]:images/Manual/Webserverapp/Websphere/ds_3.jpg
[ds_4]:images/Manual/Webserverapp/Websphere/ds_4.jpg
[ds_5]:images/Manual/Webserverapp/Websphere/ds_5.jpg
[ds_6]:images/Manual/Webserverapp/Websphere/ds_6.jpg
[ds_7]:images/Manual/Webserverapp/Websphere/ds_7.jpg
[ds_8]:images/Manual/Webserverapp/Websphere/ds_8.jpg
[ds_9]:images/Manual/Webserverapp/Websphere/ds_9.jpg
[ds_10]:images/Manual/Webserverapp/Websphere/ds_10.jpg
[ds_11]:images/Manual/Webserverapp/Websphere/ds_11.jpg
[ds_12]:images/Manual/Webserverapp/Websphere/ds_12.jpg
[ds_13]:images/Manual/Webserverapp/Websphere/ds_13.jpg
[ds_28]:images/Manual/Webserverapp/Websphere/ds_28.jpg
[ds_14]:images/Manual/Webserverapp/Websphere/ds_14.jpg
[ds_15]:images/Manual/Webserverapp/Websphere/ds_15.jpg
[ds_16]:images/Manual/Webserverapp/Websphere/ds_16.jpg
[ds_17]:images/Manual/Webserverapp/Websphere/ds_17.jpg
[ds_19]:images/Manual/Webserverapp/Websphere/ds_19.jpg
[ds_20]:images/Manual/Webserverapp/Websphere/ds_20.jpg
[ds_21]:images/Manual/Webserverapp/Websphere/ds_21.jpg
[ds_22]:images/Manual/Webserverapp/Websphere/ds_22.jpg
[ds_23]:images/Manual/Webserverapp/Websphere/ds_23.jpg
[ds_24]:images/Manual/Webserverapp/Websphere/ds_24.jpg
[ds_25]:images/Manual/Webserverapp/Websphere/ds_25.jpg

