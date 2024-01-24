
1. 部署 web 应用使用的 JNDI 数据源，将 PostgreSQL 对应的驱动 jar 包放到 tomcat 服务器的 `/usr/local/apache-tomcat-7.0.68/lib` 目录下

   > **Note:**
   >
   > 用户可以去 [PostgreSQL 官网][download]下载对应版本

2. 配置 JNDI，在 `/usr/local/apache-tomcat-7.0.68/conf/context.xml` 文件中新增如下内容：

   ```lang-text
   <Resource 
            name="jdbc/pg"
            auth="Container"
            type="javax.sql.DataSource"
            maxActive="100"
            maxIdle="30"
            maxWait="10000"
            username="sdbadmin"
            password="sdbadmin"
            driverClassName="org.postgresql.Driver"
            url="jdbc:postgresql://localhost:5432/sample"/>
   ```

   - name：表示以后要查找的名称，通过此名称可以找到 DataSource，此名称任意更换，但是程序中最终要查找的就是此名称；为了不与其他的名称混淆，所以使用 jdbc/pg，现在配置的是一个 jdbc 的关于 pg 的命名服务
   - auth：由容器进行授权及管理，指的用户名和密码是否可以在容器上生效
   - type：此名称所代表的类型，现在为 javax.sql.DataSource
   - maxActive：表示一个数据库在此服务器上所能打开的最大连接数；
   - maxIdle：表示一个数据库在此服务器上维持的最小连接数
   - maxWait：最大等待时间，单位为毫秒
   - username：数据库连接的用户名
   - password：数据库连接的密码
   - driverClassName：数据库连接的驱动程序
   - url：数据库连接的地址

3. 重启 tomcat 使配置参数生效

   ```lang-javascript
   # /usr/local/apache-tomcat-7.0.68/bin/shutdown.sh
   # /usr/local/apache-tomcat-7.0.68/bin/startup.sh
   ```


[^_^]:
    本文使用的所有引用和链接
[download]:http://jdbc.postgresql.org/download.html