
##NAME##

stopNodes - Stop one or more nodes in target host of sdbcm.

##SYNOPSIS##

**oma.stopNodes(\<svcname\>)**

##CATEGORY##

Oma

##DESCRIPTION##

Stop one or more nodes in target host of sdbcm.

**Note:**

* Oma object is a connect object 

##PARAMETERS##

* `svcname` ( *String | Int | Array*, *Required* )

    The service name of the node.

##RETURN VALUE##

On success, no return value.

On error, exception will be thrown.

##ERRORS##

the exceptions of `stopNodes()` are as below:

| Error Code | Error Type | Description | Solution |
| ------ | ------ | --- | ------ |
| -264 | SDB_COORD_NOT_ALL_DONE| One or more nodes did not complete successfully | Use getLastErrObj() to see which node is failed
| -146 | SDBCM_NODE_NOTEXISTED | Node does not exist | Check the node's configuration file |
| -6   | SDB_INVALIDARG | Invalid Argument | Check the value of the svcname parameter |

##HISTORY##

Since v3.0.2.

##EXAMPLES##

1. Stop a node with the service name 11810.

 	```lang-javascript
	> var oma = new Oma()
	> oma.stopNodes( 11810 )
 	```

2. Stop a node with the service name 11820.

 	```lang-javascript
	> var oma = new Oma()
	> oma.stopNodes( "11820" )
    ```

3. The nodes with the service names 11810, 11820, and 11830 are concurrently stoped.

 	```lang-javascript
	> var oma = new Oma()
	> oma.stopNodes( [ 11810, 11820, 11830 ] )
 	```