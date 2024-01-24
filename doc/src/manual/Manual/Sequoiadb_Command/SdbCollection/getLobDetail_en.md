
##NAME##

getLobDetail - Get lob's runtime detail information

##SYNOPSIS##
***db.collectionspace.collection.getLobDetail(\<Oid\>)***

##CATEGORY##

Collection

##DESCRIPTION##

Get lob's runtime detail information

##PARAMETERS##

* `Oid`( *String*， *Required* )
  
    Lob's ID

##RETURN VALUE##

On success, return the lob's runtime detail information.

On error, exception will be thrown.

##ERRORS##

When error happens, use [getLastErrMsg()](manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md)
to get the error message or use [getLastError()](manual/Manual/Sequoiadb_Command/Global/getLastError.md)
to get the error code. See [troubleshooting](manual/FAQ/faq_sdb.md) for
more details.

##EXAMPLES##

1. Get the lob's runtime detail information which lob's ID is 00005deb85c5350004743b09

	```lang-javascript
    > db.sample.employee.getLobDetail('00005deb85c5350004743b09')
	{
	  "Oid": "00005deb85c5350004743b09",
	  "AccessInfo": {
	    "RefCount": 3,
	    "ReadCount": 0,
	    "WriteCount": 1,
	    "ShareReadCount": 2,
	    "LockSections": [
	      {
	        "Begin": 10,
	        "End": 30,
	        "LockType": "X",
	        "Contexts": [
	          11
	        ]
	      },
	      {
	        "Begin": 30,
	        "End": 50,
	        "LockType": "S",
	        "Contexts": [
	          12
	        ]
	      }
	    ]
	  },
	  "ContextID": 14
	}
	```