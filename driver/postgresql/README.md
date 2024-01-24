SequoiaSQL-PostgreSQL 
=================  
PostgreSQL is an open source relational database that supports standard SQL.   
SequoiaDB supports creating PostgreSQL instances and is fully compatible with PostgreSQL syntax. You can use SQL statements to access SequoiaDB to add, delete, query, modify, and perform other operations.

Build Prerequisites:
-----------------
	openSSL is installed, libcrypto.so can be found in lib

Build PostgreSQL
-----------------
1.Download postgresql 9.3.4  
	
	git clone http://gitlab.sequoiadb.com/sequoiadb/sequoiasql-postgresql.git
	

2.Specify the installation path  

	cd sequoiasql-postgresql/postgresql-9.3.4
	#if choose default path: /usr/local/pgsql/
	sudo ./configure
	#if choose custom path 
	sudo ./configure --prefix=/opt/postgresql

3.Compile and install  

	sudo make
	sudo make install

4.Add users and user groups

	sudo groupadd postgres
	sudo useradd -g postgres postgres



Build Connector
-----------------
	
	cd /opt/sequoiadb/driver/postgresql
	PATH=/opt/postgresql/bin:$PATH
	#if build release version
	 sudo make local
	 sudo make all
	#if build debug version, debug version of SSL library will be used.
	 sudo make ver=debug local
	 sudo make ver=debug all
	sudo make install

Test
-----------------
Sdb shell performs the insert operation, and pgsql performs the query operation to verify that whether pgsql can access Sequoiadb properly.

1.Prepare data in sdb shell

	db.createCS("cs")
	db.cs.createCL("cl")
	db.cs.cl.insert({ a: 1, b: 2, c: "hello world" })
	db.cs.cl.insert({ d: 1 })
	db.cs.cl.insert({ a: "hello", b: "world", c: 1 })
	db.cs.cl.insert({ a: { A:1 }, b: 1.5, c: "abc" })

2.Create the database file directory

	sudo mkdir /opt/postgresql/data

3.Change the owner and group of the directory

	sudo chown -R postgres:postgres /opt/postgresql

4.Initial

	cd /opt/postgresql
	sudo su postgres
	/opt/postgresql/bin/initdb -D /opt/postgresql/data/
	/opt/postgresql/bin/postgres -D /opt/postgresql/data/ >logfile 2>&1 &


5.Start the pgsql instance

	/opt/postgresql/bin/createdb test
	/opt/postgresql/bin/psql test

6.Test operations in pg shell
	
	#register wrapper for driver  
	create extension sdb_fdw;

	#register server as remote host 
	create server sdb_server foreign data wrapper sdb_fdw options ( address 'localhost', service '11810' );

	create foreign table bar ( a integer, b integer, c text ) server sdb_server options ( collectionspace 'cs', collection 'cl' ) ;

	analyze bar;

	select * from bar;

	#result
	 a | b |      c
	---+---+-------------
	 1 | 2 | hello world
	   |   |
	   |   |
	   | 1 | abc
	(4 rows)

	create foreign table bar3 ( "a.A" integer, b integer, c text ) server sdb_server options ( collectionspace 'cs', collection 'cl' ) ;
	
	select * from bar3;

	#result
	 a.A | b |      c
    -----+---+-------------
     	 | 2 | hello world
     	 |   |
     	 |   |
       1 | 1 | abc
	(4 rows)

7.Prepare another set of data in sdb shell  

	db.cs.cl.remove()
	db.cs.cl.insert({a:100,b:100,c:[1,2,3,4,5,6,7,8,9,10]})

8.Test operation in pg shell

	create foreign table bar4 ( a integer, b integer, c integer[] ) server sdb_server options ( collectionspace 'cs', collection 'cl' ) ;  

	select * from bar4;

	#result
	  a  |  b  |           c
	-----+-----+------------------------
	 100 | 100 | {1,2,3,4,5,6,7,8,9,10}
	(1 row)

	select a,b,unnest(c) from bar4;

	#result
	  a  |  b  | unnest
	-----+-----+--------
	 100 | 100 |      1
	 100 | 100 |      2
	 100 | 100 |      3
	 100 | 100 |      4
	 100 | 100 |      5
	 100 | 100 |      6
	 100 | 100 |      7
	 100 | 100 |      8
	 100 | 100 |      9
	 100 | 100 |     10
	(10 rows)


Restart Postgresql
-----------------
	su - postgres
	
	#find out something like "/opt/pgsql/bin/postgres -D /opt/pgsql/data"
	ps -elf | grep postgres 

	kill -15 <pid>
	
	/opt/pgsql/bin/postgres -D /opt/pgsql/data >logfile 2>&1 &


