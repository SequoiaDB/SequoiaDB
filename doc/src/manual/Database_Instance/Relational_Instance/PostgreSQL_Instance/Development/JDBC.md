[^_^]:
    JDBC驱动
    作者：赵育
    时间：20190817
    评审意见
    王涛：时间：
    许建辉：时间：
    市场部：时间：20190905


用户下载 [JDBC 驱动][download] 并导入 jar 包后，即可以使用 JDBC 提供的 API。

示例
----

以下示例为通过 maven 工程使用 JDBC 进行简单的增删改查操作。

1. 在 `pom.xml` 中添加 PostgreSQL JDBC 驱动的依赖，以 postgresql-9.3-1104-jdbc41 为例

   ```lang-xml
   <dependencies>
       <dependency>
           <groupId>org.postgresql</groupId>
           <artifactId>postgresql</artifactId>
           <version>9.3-1104-jdbc41</version>
       </dependency>
   </dependencies>
   ```

2.  假设本地有默认安装的 PostgreSQL 实例，连接到该实例并准备数据库 sample 和表 test，将其映射到 SequoiaDB 已存在的集合 sample.employee

   ```lang-bash
   $ bin/psql -p 5432 sample
   psql (9.3.4)
   Type "help" for help.
   
   sample=# create foreign table test (name text, id numeric) server sdb_server options (collectionspace 'sample', collection 'employee', decimal 'on');
   CREATE FOREIGN TABLE
   sample=# \q
   ```

3. 在工程的 `src/main/java/com/sequoiadb/sample` 目录下添加 `Sample.java` 文件

   ```lang-java
   package com.sequoiadb.sample;
   
   import java.sql.Connection;
   import java.sql.DriverManager;
   import java.sql.PreparedStatement;
   import java.sql.ResultSet;
   import java.sql.ResultSetMetaData;
   import java.sql.SQLException;
   
   public class Sample {
       static {
           try {
               Class.forName("org.postgresql.Driver");
           } catch (ClassNotFoundException e) {
               e.printStackTrace();
           }
       }
   
       public static void main(String[] args) throws SQLException {
           String pghost = "127.0.0.1";
           String port = "5432";
           String databaseName = "sample";
           // postgresql process is running in which user
           String pgUser = "sdbadmin";
           String url = "jdbc:postgresql://" + pghost + ":" + port + "/" + databaseName;
           Connection conn = DriverManager.getConnection(url, pgUser, null);
   
           // insert
           String sql = "INSERT INTO test(name, id) VALUES(?, ?)";
           PreparedStatement pstmt = conn.prepareStatement(sql);
           for (int i = 0; i < 5; i++) {
               pstmt.setString(1, "Jim" + i);
               pstmt.setLong(2, i);
               pstmt.addBatch();
           }
           pstmt.executeBatch();
   
           // select
           sql = "SELECT * FROM test";
           pstmt = conn.prepareStatement(sql);
           ResultSet rs = pstmt.executeQuery();
           boolean isHeaderPrint = false;
           while (rs.next()) {
               ResultSetMetaData md = rs.getMetaData();
               int col_num = md.getColumnCount();
               if (!isHeaderPrint) {
                   System.out.print("|");
                   for (int i = 1; i <= col_num; i++) {
                       System.out.print(md.getColumnName(i) + "\t|");
                       isHeaderPrint = true;
                   }
                   System.out.println();
               }
               System.out.print("|");
               for (int i = 1; i <= col_num; i++) {
                   System.out.print(rs.getString(i) + "\t|");
               }
               System.out.println();
           }
           pstmt.close();
           conn.close();
       }
   }
   ```

4. 使用 maven 编译及运行

   ```lang-bash
   $ mvn compile 
   $ mvn exec:java -Dexec.mainClass="com.sequoiadb.sample.Sample"
   ```

   得到运行结果如下:

   ```lang-text
   |name	|id	|
   |Jim0	|0	|
   |Jim1	|1	|
   |Jim2	|2	|
   |Jim3	|3	|
   |Jim4	|4	|
   ```


[^_^]:
     本文使用的所有引用和链接
[download]:manual/Database_Instance/Relational_Instance/PostgreSQL_Instance/Development/engine_download.md