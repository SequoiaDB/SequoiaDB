# Spark-SequoiaDB connector

Spark-SequoiaDB connector is a library that allows users to read/write data with [Spark SQL](http://spark.apache.org/docs/latest/sql-programming-guide.html) from/into SequoiaDB collections.

[SequoiaDB](http://www.sequoiadb.com/ "SequoiaDB website") is a document-oriented NoSQL database and provides a JSON storage model. [Spark](http://spark.apache.org/ "Spark website") is a fast and general-purpose cluster computing system.

Spark-SequoiaDB connector is used to integrate SequoiaDB and Spark, in order to give users a system that combines the advantages of schema-less storage model with dynamic indexing and Spark cluster.

## Requirements

This library requires Spark 2.0+, Scala 2.11.8+ and sequoiadb-driver-2.8+

## Build

You can use the following command to build the library:
```
mvn clean package
```

## Using

You can link against this library by putting the following lines in your program:
```
<groupId>com.sequoiadb</groupId>
<artifactId>spark-sequoiadb_2.11</artifactId>
<version>LATEST</version>
```

You can load the library into spark-shell/spark-sql by using --jars command line option.
$ bin/spark-sql --jars /Users/sequoiadb/spark-sequoiadb/lib/sequoiadb-driver-2.8.1.jar,/Users/sequoiadb/spark-sequoiadb/target/spark-sequoiadb_2.11-2.8.1.jar

You can load SequoiaDB collection using the following SQL:
```
create table foo(hello string, rangekey int, key1 int) using com.sequoiadb.spark OPTIONS(host 'localhost:11810', collectionspace 'mycs', collection 'mycl');
```

## License

Copyright SequoiaDB Inc.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
