[^_^]:
    MySQL 实例-JDBC 驱动


用户下载 [JDBC 驱动][download]并导入 jar 包后，即可以使用 JDBC 提供的 API。下述示例为通过 maven 工程使用 JDBC 进行简单的增删改查操作。

1. 在 `pom.xml` 中添加 MySQL JDBC 驱动的依赖，以 mysql-connector-java-5.1.38 为例

    ```lang-xml
    <dependencies>
        <dependency>
            <groupId>mysql</groupId>
            <artifactId>mysql-connector-java</artifactId>
            <version>5.1.38</version>
        </dependency>
    </dependencies>
    ```

2. 假设本地有默认安装的 MySQL 实例，存在 sdbadmin 用户，密码为 123456，连接到该实例并准备样例使用的数据库 db 和表 tb

    ```lang-sql
    CREATE DATABASE db;
    USE db;
    CREATE TABLE tb (id INT, first_name VARCHAR(128), last_name VARCHAR(128));
    ```

3. 在工程的 `src/main/java/com/sequoiadb/sample` 目录下添加 `JdbcSample.java` 文件。

    ```lang-java
    package com.sequoiadb.sample;
    
    import java.sql.*;
    
    public class JdbcSample {
    	static {
    		try {
    			Class.forName("com.mysql.jdbc.Driver");
    		} catch (ClassNotFoundException e) {
    			e.printStackTrace();
    		}
    	}
    
    	public static void main(String[] args) throws SQLException {
    		String hostName = "127.0.0.1";
    		String port = "3306";
    		String databaseName = "db";
    		String myUser = "sdbadmin";
    		String myPasswd = "123456";
    		String url = "jdbc:mysql://" + hostName + ":" + port + "/" + databaseName + "?useSSL=false";
    		Connection conn = DriverManager.getConnection(url, myUser, myPasswd);
    
    		System.out.println("---INSERT---");
    		String sql = "INSERT INTO tb VALUES(?,?,?)";
    		PreparedStatement ins = conn.prepareStatement(sql);
    		ins.setInt(1, 1);
    		ins.setString(2, "Peter");
    		ins.setString(3, "Parcker");
    		ins.executeUpdate();
    
    		System.out.println("---UPDATE---");
    		sql = "UPDATE tb SET first_name=? WHERE id = ?";
    		PreparedStatement upd = conn.prepareStatement(sql);
    		upd.setString(1, "Stephen");
    		upd.setInt(2, 1);
    		upd.executeUpdate();
    
    		System.out.println("---SELECT---");
    		Statement stmt = conn.createStatement();
    		sql = "SELECT * FROM tb";
    		ResultSet rs = stmt.executeQuery(sql);
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
    			}
    			System.out.println();
    			System.out.print("|");
    			for (int i = 1; i <= col_num; i++) {
    				System.out.print(rs.getString(i) + "\t|");
    			}
    			System.out.println();
    		}
    		stmt.close();
    
    		System.out.println("---DELETE---");
    		sql = "DELETE FROM tb WHERE id = ?";
    		PreparedStatement del = conn.prepareStatement(sql);
    		del.setInt(1, 1);
    		del.executeUpdate();
    
    		conn.close();
    	  }
    }
    ```

4. 使用 maven 编译及运行

    ```lang-bash
    $ mvn compile 
    $ mvn exec:java -Dexec.mainClass="com.sequoiadb.sample.JdbcSample"
    ```
    得到如下运行结果：
    
    ```lang-text
    ---INSERT---
    ---UPDATE---
    ---SELECT---
    |id	|first_name	|last_name	|
    |1	|Stephen	|Parcker	|
    ---DELETE---
    ```

[^_^]:
    本文使用到的所有连接及引用。
[download]:manual/Database_Instance/Relational_Instance/MySQL_Instance/Development/engine_download.md
