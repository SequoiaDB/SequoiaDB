
##NAME##

getSystem - Create a remote System object

##SYNOPSIS##

**remoteObj.getSystem()**

##CATEGORY##

Remote

##DESCRIPTION##

This function is used to open a file or create a new file.

##PARAMETERS##

None

##RETURN VALUE##

When the function executes successfully,  it will return a System object. 

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

When the exception happens，use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the error code. For more details, refer to [Troubleshooting][error_guide].

##VERSION##

v3.2 and above

##EXAMPLES##

* Create a remote object.

```lang-javascript
> var remoteObj = new Remote( "192.168.20.71", 11790 )
```

* Create a remote System object.

```lang-javascript
> var system = remoteObj.getSystem()
```

[^_^]:
     links
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[error_guide]:manual/FAQ/faq_sdb.md