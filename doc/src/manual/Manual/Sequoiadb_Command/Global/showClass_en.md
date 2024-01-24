##NAME##

showClass - List the custom classes built into the sdb shell and all the methods contained in the built-in custom classes.

##SYNOPSIS##

**showClass([className])**

##CATEGORY##

Global

##DESCRIPTION##

This method is used to list all the built-in custom classes supported by the sdb shell or list all the methods contained in the specified built-in custom classes.

##PARAMETERS##

* `className` ( *String*， *Optional* )

   Class name to be enumerated.

##RETURN VALUE##

When classname is empty, retuen all the built-in custom classes supported by the sdb shell; when classname is not empty, return all the methods contained in the specified built-in custom class.

##HISTORY##

Since v2.8

##EXAMPLES##

1. List all the built-in custom classes supported by the sdb shell.

	```lang-javascript
	> showClass()
	All classes:
   	   BSONArray
   	   BSONObj
   	   BinData
   	   CLCount
   	   Cmd
   	   ...
	Global functions:
       catPath()
       forceGC()
       getExePath()
       getLastErrMsg()
       ...
	Takes 0.000518s.
	```

2. List all methods contained in class SdbDate.

	```lang-javascript
	> showClass("SdbDate")
	SdbDate's static functions:
	   help()
	SdbDate's member functions:
   	   help()
       toString()
	Takes 0.000218s.
	```

