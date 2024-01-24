DB2
	username: db2inst1
	password: huawei
	
	1. Create Database
	
	2. Create Table
		
		db2batch -d hsdb -f sql/example/db2/create_table.sql
	
	3. Load Data

		java -cp bin/:lib/db2jcc4.jar iomark.TPCCRunner.Loader conf/example/db2/loader.properties

	4. Create index
		
		db2batch -d hsdb -f sql/example/db2/create_index.sql

	5. Run Master
		
                java -cp bin/:lib/db2jcc4.jar iomark.TPCCRunner.Master conf/example/db2/master.properties

    6. Run Slaves on Clients

                java -cp bin/:lib/db2jcc4.jar iomark.TPCCRunner.Slave conf/example/db2/slave1.properties
                java -cp bin/:lib/db2jcc4.jar iomark.TPCCRunner.Slave conf/example/db2/slave2.properties

MySQL

	username: root
	password: huawei

	1. Create Database
		
		mysql -uroot -phuawei -vvv -n < sql/example/mysql/create_database.sql

	2. Create Table

		mysql -uroot -phuawei -vvv -n < sql/example/mysql/create_table.sql

	3. Load Data
		
		java -cp bin/:lib/mysql-connector-java-5.1.7-bin.jar iomark.TPCCRunner.Loader conf/example/mysql/loader.properties

	4. Create index

		mysql -uroot -phuawei -vvv -n < sql/example/mysql/create_index.sql

	5. Run Master

		java -cp bin/:lib/mysql-connector-java-5.1.7-bin.jar iomark.TPCCRunner.Master conf/example/mysql/master.properties

	6. Run Slaves on Clients

		java -cp bin/:lib/mysql-connector-java-5.1.7-bin.jar iomark.TPCCRunner.Slave conf/example/mysql/slave1.properties
		java -cp bin/:lib/mysql-connector-java-5.1.7-bin.jar iomark.TPCCRunner.Slave conf/example/mysql/slave2.properties


SQLServer

	1. Create Database

		sqlcmd -e -p 1 -y 30 -Y 30 -i sql\example\sqlserver\create_database.sql

	2. Create User

		sqlcmd -e -p 1 -y 30 -Y 30 -i sql\example\sqlserver\create_user.sql

	3. Create Tables

		sqlcmd -e -p 1 -y 30 -Y 30 -i sql\example\sqlserver\create_table.sql
		
	4. Load Data
		
		java -cp bin;lib\sqljdbc4.jar iomark.TPCCRunner.Loader conf\example\sqlserver\loader.properties

	5. Create index

		sqlcmd -e -p 1 -y 30 -Y 30 -i sql\example\sqlserver\create_index.sql

	6. Run Master

		java -cp bin;lib\sqljdbc4.jar iomark.TPCCRunner.Master conf\example\sqlserver\master.properties

	7. Run Slaves on Clients

		java -cp bin;lib\sqljdbc4.jar iomark.TPCCRunner.Slave conf\example\sqlserver\slave1.properties
		java -cp bin;lib\sqljdbc4.jar iomark.TPCCRunner.Slave conf\example\sqlserver\slave2.properties


Oracle

	1. Create Tablespace

		sqlplus / as sysdba @sql/example/oracle/create_tablespace.sql
		
	2. Create User

		sqlplus / as sysdba @sql/example/oracle/create_user.sql

	3. Create Tables

		sqlplus user1/pswd @sql/example/oracle/create_table.sql

	4. Load Data
		
		java -cp bin/:lib/ojdbc14-10.2.jar iomark.TPCCRunner.Loader conf/example/oracle/loader.properties

	5. Create index

		sqlplus user1/pswd @sql/example/oracle/create_index.sql

	6. Run Master

		java -cp bin/:lib/ojdbc14-10.2.jar iomark.TPCCRunner.Master conf/example/oracle/master.properties

	7. Run Slaves on Clients

		java -cp bin/:lib/ojdbc14-10.2.jar iomark.TPCCRunner.Slave conf/example/oracle/slave1.properties
		java -cp bin/:lib/ojdbc14-10.2.jar iomark.TPCCRunner.Slave conf/example/oracle/slave2.properties

Informix

	username: informix
	password: huawei

	1. Create dbspace

		onspaces -c -d data1dbs -p $INFORMIXDIR/data/data1dbs -o 0 -s `cat /proc/partitions | awk '/sdc/{printf "%d000",$3/1000}'` -ef 1024 -en 8192 -k 4
		printf "\n" | ontape -s -L 0

	2. Create Database

		dbaccess - sql/example/informix/create_database.sql

	3. Create Tables

		dbaccess data1 sql/example/informix/create_table.sql
		
	4. Load Data
		
		java -cp bin/:lib/ifxjdbc.jar iomark.TPCCRunner.Loader conf/example/informix/loader.properties

	5. Create index

		dbaccess data1 sql/example/informix/create_index.sql

	6. Run Master

		java -cp bin/:lib/ifxjdbc.jar iomark.TPCCRunner.Master conf/example/informix/master.properties

	7. Run Slaves on Clients

		java -cp bin/:lib/ifxjdbc.jar iomark.TPCCRunner.Slave conf/example/informix/slave1.properties
		java -cp bin/:lib/ifxjdbc.jar iomark.TPCCRunner.Slave conf/example/informix/slave2.properties

