
##NAME##

getSelfPath - Get the path of the js script which is being executed.

##SYNOPSIS##

**getSelfPath()**

##CATEGORY##

Global

##DESCRIPTION##

Get the path of the js script which is being executed.

##PARAMETERS##

NULL.

##RETURN VALUE##

The path of the js script which is being executed.

##ERRORS##

NULL.

##HISTORY##

Since v3.0.

##EXAMPLES##

We assume:  
SequoiaDB is installed in: /opt/sequoiadb.  
The user and user group of SequoiaDB is: sdbadmin:sdbadmin_group.  
Current work path is in sdbadmin's Home Path: /home/users/sdbadmin.  
We have file "/opt/sequoiadb/bin/test/a.js" with the contents:

```lang-bash
sdbadmin@ubuntu-dev1:~$ pwd
/home/users/sdbadmin
sdbadmin@ubuntu-dev1:~$ cat /opt/sequoiadb/bin/test/a.js
println( 'exePath: ' + getExePath() ) ;
println( 'rootPath:' + getRootPath() ) ;
println( 'selfPath:' + getSelfPath() ) ;
```

Start sdb shell:

```lang-bash
sdbadmin@ubuntu-dev1:~$ /opt/sequoiadb/bin/sdb
Welcome to SequoiaDB shell!
help() for help, Ctrl+c or quit to exit
>
```

1. getRootPath(): get the working path of the program(e.g. sdb shell) which is executing the current js script.

	```lang-javascript
	> getRootPath()
	/home/users/sdbadmin
	Takes 0.000122s.
	>
 	```

2. getExePath(): get the path of the program(e.g. sdb shell).

	```lang-javascript
	> getExePath()
	/opt/sequoiadb/bin
	Takes 0.000122s.
	>
 	```

3. getSelfPath(): get the path of the js script which is being executed.

	```lang-javascript
	> getSelfPath()
	/home/users/sdbadmin
	Takes 0.000297s.
	>
 	```

4. Import a file, the get some paths. Pay more attension to the output of  getSelfPath().


	```lang-javascript
	> import( '/opt/sequoiadb/bin/test/a.js')
	exePath: /opt/sequoiadb/bin
	rootPath:/home/users/sdbadmin
	selfPath:/opt/sequoiadb/bin/test
	Takes 0.000401s.
	```