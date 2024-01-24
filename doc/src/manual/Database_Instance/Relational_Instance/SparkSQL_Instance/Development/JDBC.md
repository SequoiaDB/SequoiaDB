 [^_^]:
    JDBC 驱动

本文档将介绍 SparkSQL 通过 JDBC 驱动对接 SequoiaDB 巨杉数据库的示例。

##SparkSQL 对接 SequoiaDB##

SparkSQL 可以通过 JDBC 驱动对 SequoiaDB 进行操作。

###对接前准备###

1. 下载安装 Spark 和 SequoiaDB 数据库，将 Spark-SequoiaDB 连接组件和 SequoiaDB Java 驱动的 jar 包复制到 Spark 安装路径下的 `jars` 目录下

2. 新建一个 java 项目，并导入 sparkSQL 的 JDBC 驱动程序依赖包，可使用 maven 下载，参考配置如下：

    ```lang-java
      <dependencies>
        <dependency>
             <groupId>org.apache.hive</groupId>
             <artifactId>hive-jdbc</artifactId>
             <version>$version</version>
        </dependency>
        <dependency>
             <groupId>org.apache.hadoop</groupId>
             <artifactId>hadoop-common</artifactId>   
             <version>$version</version>     
         </dependency>
       </dependencies>
    ```

###示例###

假设 SequoiaDB 存在集合 test.test，且保存数据如下：

```lang-javascript
> db.test.test.find()
{
  "_id": {
    "$oid": "5d5911f41125bc9c9aa2bc0b"
  },
  "c1": 0,
  "c2": "mary",
  "c3": 15
}
{
  "_id": {
    "$oid": "5d5912041125bc9c9aa2bc0c"
  },
  "c1": 1,
  "c2": "lili",
  "c3": 25
}
```

编写并执行示例代码

```lang-java
package com.spark.samples;

import java.sql.Connection;
import java.sql.DriverManager;
import java.sql.ResultSet;
import java.sql.SQLException;
import java.sql.Statement;

public class HiveJdbcClient {
    public static void main(String[] args) throws ClassNotFoundException {
        //JDBC Driver程序的类名
        Class.forName("org.apache.hive.jdbc.HiveDriver");
        try {
            //连接SparkSQL，假设spark服务所在主机名为sparkServer
            Connection connection = DriverManager.getConnection("jdbc:hive2://sparkServer:10000/default", "", "");
            System.out.println("connection success!");
            Statement statement = connection.createStatement();
            // 创建表，该表映射SequoiaDB中表test.test
            String crtTableName = "test";
            statement.execute("CREATE TABLE" + crtTableName
                    + "( c1 int, c2 string, c3 int ) USING com.sequoiadb.spark OPTIONS ( host 'server1:11810,server2:11810', "
                    + "collectionspace 'test', collection 'test',username '',password '')");
            // 查询表test数据,返回sequoiaDB中test.test表中的数据信息
            String sql = "select * from " + crtTableName;
            System.out.println("Running:" + sql);
            ResultSet resultSet = statement.executeQuery(sql);
            while (resultSet.next()) {
                System.out.println(
                        String.valueOf(resultSet.getString(1)) + "\t" + String.valueOf(resultSet.getString(2)));

            }
            statement.close();
            connection.close();
        } catch (SQLException e) {
            e.printStackTrace();
        }
    }
}
```

运行结果如下：

```lang-text
connection success!
Running:select * from test
1	lili	25
0	mary	15
```

##SparkSQL 对接 SequoiaSQL##

SparkSQL 可以通过 DataFrames 使用 JDBC 对 SequoiaSQL-MySQL 或 SequoiaSQL-PGSQL 进行读写操作。

###对接前准备###

1. 下载相应的 JDBC 驱动，将其拷贝到 spark 集群 `SPARK_HOME/jars` 目录下

2. 在读实例执行创建测试库、测试用户、授权及准备数据，在写实例执行创建测试库、测试用户及授权

   ```lang-sql
   -- Create test database
   create database sparktest;
   
   -- Create a user representing your Spark cluster
   create user 'sparktest'@'%' identified by 'sparktest';
   
   -- Add privileges for the Spark cluster
   grant create, delete, drop, insert, select, update on sparktest.* to 'sparktest'@'%';
   flush privileges;
   
   -- Create a test table of physical characteristics.
   use sparktest;
   create table people (
     id int(10) not null auto_increment,
     name char(50) not null,
     is_male tinyint(1) not null,
     height_in int(4) not null,
     weight_lb int(4) not null,
     primary key (id),
     key (id)
   );
   
   -- Create sample data to load into a DataFrame
   insert into people values (null, 'Alice', 0, 60, 125);
   insert into people values (null, 'Brian', 1, 64, 131);
   insert into people values (null, 'Charlie', 1, 74, 183);
   insert into people values (null, 'Doris', 0, 58, 102);
   insert into people values (null, 'Ellen', 0, 66, 140);
   insert into people values (null, 'Frank', 1, 66, 151);
   insert into people values (null, 'Gerard', 1, 68, 190);
   insert into people values (null, 'Harold', 1, 61, 128);
   ```

###示例###

1. 编写示例代码

   ```lang-java
   package com.sequoiadb.test;
   
   import org.apache.spark.sql.Dataset;
   import org.apache.spark.sql.Row;
   import org.apache.spark.sql.SparkSession;
   
   import java.io.File;
   import java.io.FileInputStream;
   import java.util.Properties;
   
   public final class JDBCDemo {
   
   	public static void main(String[] args) throws Exception {
           String readUrl = "jdbc:mysql://192.168.30.81/sparktest" ;
           String writeUrl = "jdbc:mysql://192.168.30.82/sparktest" ;
   
   		SparkSession spark = SparkSession.builder().appName("JDBCDemo").getOrCreate();
   
   		Properties dbProperties = new Properties();
           dbProperties.setProperty("user", "sparktest") ;
           dbProperties.setProperty("password", "sparktest" );
   
   		System.out.println("A DataFrame loaded from the entire contents of a table over JDBC.");
   		String where = "sparktest.people";
   		Dataset<Row> entireDF = spark.read().jdbc(readUrl, where, dbProperties);
   		entireDF.printSchema();
   		entireDF.show();
   
   		System.out.println("Filtering the table to just show the males.");
   		entireDF.filter("is_male = 1").show();
   
   		System.out.println("Alternately, pre-filter the table for males before loading over JDBC.");
   		where = "(select * from sparktest.people where is_male = 1) as subset";
   		Dataset<Row> malesDF = spark.read().jdbc(readUrl, where, dbProperties);
   		malesDF.show();
   
   		System.out.println("Update weights by 2 pounds (results in a new DataFrame with same column names)");
   		Dataset<Row> heavyDF = entireDF.withColumn("updated_weight_lb", entireDF.col("weight_lb").plus(2));
   		Dataset<Row> updatedDF = heavyDF.select("id", "name", "is_male", "height_in", "updated_weight_lb")
   			.withColumnRenamed("updated_weight_lb", "weight_lb");
   		updatedDF.show();
   
   		System.out.println("Save the updated data to a new table with JDBC");
   		where = "sparktest.updated_people";
   		updatedDF.write().mode("error").jdbc(writeUrl, where, dbProperties);
   
   		System.out.println("Load the new table into a new DataFrame to confirm that it was saved successfully.");
   		Dataset<Row> retrievedDF = spark.read().jdbc(writeUrl, where, dbProperties);
   		retrievedDF.show();
   
   		spark.stop();
   	  }
   }
   ```

2. 编译并提交任务

   ```lang-bash
   mkdir -p target/java
   javac src/main/java/com/sequoiadb/test/JDBCDemo.java -classpath "$SPARK_HOME/jars/*" -d target/java
   cd target/java
   jar -cf ../JDBCDemo.jar *
   cd ../..
   APP_ARGS="--class com.sequoiadb.test.JDBCDemo target/JDBCDemo.jar"
   #本地提交
   $SPARK_HOME/bin/spark-submit --driver-class-path lib/mysql-connector-java-5.1.38.jar $APP_ARGS
   #集群提交
   $SPARK_HOME/bin/spark-submit --master spark://ip:7077 $APP_ARGS
   ```

   运行结果如下：

   ```lang-text
   A DataFrame loaded from the entire contents of a table over JDBC.
   root
    |-- id: integer (nullable = true)
    |-- name: string (nullable = true)
    |-- is_male: boolean (nullable = true)
    |-- height_in: integer (nullable = true)
    |-- weight_lb: integer (nullable = true)
   
   +---+-------+-------+---------+---------+
   | id|   name|is_male|height_in|weight_lb|
   +---+-------+-------+---------+---------+
   |  1|  Alice|  false|       60|      125|
   |  2|  Brian|   true|       64|      131|
   |  3|Charlie|   true|       74|      183|
   |  4|  Doris|  false|       58|      102|
   |  5|  Ellen|  false|       66|      140|
   |  6|  Frank|   true|       66|      151|
   |  7| Gerard|   true|       68|      190|
   |  8| Harold|   true|       61|      128|
   +---+-------+-------+---------+---------+
   
   Filtering the table to just show the males.
   +---+-------+-------+---------+---------+
   | id|   name|is_male|height_in|weight_lb|
   +---+-------+-------+---------+---------+
   |  2|  Brian|   true|       64|      131|
   |  3|Charlie|   true|       74|      183|
   |  6|  Frank|   true|       66|      151|
   |  7| Gerard|   true|       68|      190|
   |  8| Harold|   true|       61|      128|
   +---+-------+-------+---------+---------+
   
   Alternately, pre-filter the table for males before loading over JDBC.
   +---+-------+-------+---------+---------+
   | id|   name|is_male|height_in|weight_lb|
   +---+-------+-------+---------+---------+
   |  2|  Brian|   true|       64|      131|
   |  3|Charlie|   true|       74|      183|
   |  6|  Frank|   true|       66|      151|
   |  7| Gerard|   true|       68|      190|
   |  8| Harold|   true|       61|      128|
   +---+-------+-------+---------+---------+
   
   Update weights by 2 pounds (results in a new DataFrame with same column names)
   +---+-------+-------+---------+---------+
   | id|   name|is_male|height_in|weight_lb|
   +---+-------+-------+---------+---------+
   |  1|  Alice|  false|       60|      127|
   |  2|  Brian|   true|       64|      133|
   |  3|Charlie|   true|       74|      185|
   |  4|  Doris|  false|       58|      104|
   |  5|  Ellen|  false|       66|      142|
   |  6|  Frank|   true|       66|      153|
   |  7| Gerard|   true|       68|      192|
   |  8| Harold|   true|       61|      130|
   +---+-------+-------+---------+---------+
   
   Save the updated data to a new table with JDBC
   Load the new table into a new DataFrame to confirm that it was saved successfully.
   +---+-------+-------+---------+---------+
   | id|   name|is_male|height_in|weight_lb|
   +---+-------+-------+---------+---------+
   |  1|  Alice|  false|       60|      127|
   |  2|  Brian|   true|       64|      133|
   |  3|Charlie|   true|       74|      185|
   |  4|  Doris|  false|       58|      104|
   |  5|  Ellen|  false|       66|      142|
   |  6|  Frank|   true|       66|      153|
   |  7| Gerard|   true|       68|      192|
   |  8| Harold|   true|       61|      130|
   +---+-------+-------+---------+---------+
   ```
 