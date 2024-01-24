##NAME##

help - dispaly the help information

##SYNOPSIS##

**help()**

**help(\<method\>)**

##CATEGORY##

Global

##DESCRIPTION##

Display help information in global scope.

* help() - display the help information in global scope.

* help(\<method\>) - display the detail of a method in global scope.

Display the help information of a class or an object:

* \<class\>.help() - display all the methods in specified class or object.

* \<class\>.help(\<method\>) - display the detail of method in specified class or object.

##PARAMETERS##

* `class` ( *object*， *Requird* )

	javascript class or object.

* `method` ( *string*， *Requird* )

	The name of class or object.

##RETURN VALUE##

NULL.

##ERRORS##

NULL.

##HISTORY##

v1.0 and above.

##EXAMPLES##

- Show the global help information.

	```lang-javascript
	> help()
 	```

- Show the all the methods of a classs.

	```lang-javascript
	> Oma.help()
   		--Constructor methods for class "Oma":
   		var oma = new Oma([hostname],[svcname])
                              - Class for cluster management.
   		--Static methods for class "Oma":
   		Oma.addAOmaSvcName(<hostname>,<svcname>,[isReplace],[confFile])
                              - Specify the service name of sdbcm in target
                                host.
   		Oma.delAOmaSvcName(hostname,[confFile])
                              - Delete the service name of sdbcm from its
                                configuration file in target host.
   		Oma.getAOmaSvcName(hostname,[confFile])
                              - Get the service name of sdbcm in target host.
		...
 	```

- Show the all the methods of an object.

	```lang-javascript
	> var oma = new Oma()
	> oma.help()
   		--Instance methods for class "Oma":
	   	close()              - Close the Oma object.
   		createCoord(<svcname>,<dbpath>,[config])
                             - Create a coord node in target host of sdbcm.
   		createData(<svcname>,<dbpath>,[config])
                             - Create a standalone node in target host of
                               sdbcm.
		...
 	```


- Show the manpage of the specified method.

	```lang-javascript
	> help("createCS")

	createCS(1)                       Version 2.8                      createCS(1)

	NAME
       	createCS - Create a collection space in current database.

	SYNOPSIS
       	db.createCS(<name>,[options])

	CATEGORY
       	Sequoiadb

	DESCRIPTION
       	Create a collection space in a database instance.


       	name (string)
              	Collection space name. Collection space name should be
              	unique to each other in a database instance.
	...
 	```

- Show the manpage of the specified method in a class or an object.

	```lang-javascript
	> var db = new Sdb()
	> db.sample.help("createCL") // or SdbCS.help("createCL")

	createCL(1)                       Version 2.8                      createCL(1)

	NAME
       	createCL - create a new collection.

	SYNOPSIS
       	db.collectionspace.createCL(<name>,[option])

	CATEGORY
       	Collection Space

	DESCRIPTION
       	Create a collection in a specified collection space.  Collection is a
       	logical object which stores records.  Each record should belong to one
       	and only one collection.

	PARAMETERS
       	* name ( String , Required )

         	The name of the collection, should be unique to each other in a
         	collection space.

	...
 	```
