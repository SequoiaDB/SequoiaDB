##NAME##

getLastErrObj - Get the error object of last operation.

##SYNOPSIS##

**getLastErrObj()**

##CATEGORY##

Global

##DESCRIPTION##

Get the error object of last operation.

##PARAMETERS##

NULL.

##RETURN VALUE##

The error object of last operation which contains the fields as follows:

* errno: (Int32) error code.
* description: (String) the description of error code.
* detail: (String) error detail.
* ErrNodes: (BSON object) describes which data nodes have errors, and detailed information about the error
(this field is an expand field, which is only returned when an error occurs on the data node).

##ERRORS##

NULL.

##HISTORY##

Since v2.6.

##EXAMPLES##

Get the error object of last operation. When the error occurs on a data node, the error object returned contains the description of the ErrNodes field.

  	```lang-javascript
    > db.sample.employee.createIndex("A",{"a":1})
    (shell):1 uncaught exception: -247
    Redefine index
  	> var err = getLastErrObj()
	> var obj = err.toObj()
	> println( obj.toString() )
    {
      "errno": -247,
      "description": "Redefine index",
      "detail": "",
      "ErrNodes": [
        {
          "NodeName": "localhost:11830",
          "GroupName": "group2",
          "Flag": -247,
          "ErrInfo": {
            "errno": -247,
            "description": "Redefine index",
            "detail": ""
          }
        }
      ]
    }
    ```