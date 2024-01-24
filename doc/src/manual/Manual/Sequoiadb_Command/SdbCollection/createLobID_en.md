
##NAME##

createLobID - Create a lob ID, not a lob

##SYNOPSIS##
***db.collectionspace.collection.createLobID([Time])***

##CATEGORY##

Collection

##DESCRIPTION##

Create a lob ID from Server

##PARAMETERS##

* `Time`( *String*， *Optional* )
  
    Create a lob ID by Time, the minimum precision is second. The valid format of Time is:"YYYY-MM-DD-HH.mm.ss", example:"2019-08-01-12.00.00".

* When no parameter is specified, Lob ID will be created by the Time in server side.

##RETURN VALUE##

On success, return a lob ID.

On error, exception will be thrown.

##ERRORS##

When error happens, use [getLastErrMsg()](manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md)
to get the error message or use [getLastError()](manual/Manual/Sequoiadb_Command/Global/getLastError.md)
to get the error code. See [troubleshooting](manual/FAQ/faq_sdb.md) for more details.


##EXAMPLES##

* Create a lob ID by the Time in server side.

    ```lang-javascript
    > db.sample.employee.createLobID()
    00005d36d096350002de7f3a
    Takes 0.329455s.
    ```

* Create a lob ID by the specified Time.

    ```lang-javascript
    > db.sample.employee.createLobID( "2015-06-05-16.10.33.000000" )
    00005571c9f93f03e8d8dd57
    Takes 0.108214s.
    ```