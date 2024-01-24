##NAME##

jsonFormat-  Set BSON output format.When out of memory error happen, we can use jsonFormat( false ) to disable BSON formatted output.

##SYNOPSIS##

**jsonFormat(\<pretty\>)**

##CATEGORY##

Global

##DESCRIPTION##

 Set BSON output format.

##PARAMETERS##

* `pretty` ( *Bool*, *Required* * )

  The flags of BSON formatted output.

##RETURN VALUE##

NULL.

##HISTORY##

Since v2.6

##EXAMPLES##

1. Use jsonFormat( false ) to disable BSON formatted output.。

	```lang-javascript
  	> db.sample.employee.find()
    {
      "_id": {
      "$oid": "59fac185e610b8510e000001"
      },
      "a": 1,
      "b": 2
    }
    Return 1 row(s).
    Takes 0.024873s.
    > jsonFormat( false )
    Takes 0.000225s.
    > db.sample.employee.find()
    { "_id": { "$oid": "59fac185e610b8510e000001" }, "a": 1, "b": 2 }
    Return 1 row(s).
    Takes 0.002948s.
  	```