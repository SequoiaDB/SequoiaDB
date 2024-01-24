##NAME##

dropIndexAsync - drop the index asynchronously

##SYNOPSIS##

**db.collectionspace.collection.dropIndexAsync(\<name\>)**

##CATEGORY##

SdbCollection

##DESCRIPTION##

This function is used to asynchronously drop the specified index in the collection.

##PARAMETERS##

name ( *string, required* )

Specify the name of index to drop. 

##RETURN VALUE##

When the function executes successfully, it will return an object of type number. Users can get a task ID through this object. 

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the [error code][error_code]. For more details, refer to [Troubleshooting][faq].

##VERSION##

v3.6 and above

##EXAMPLES##

1. Drop the index named "ageIndex" in the collection sample.employee.

    ```lang-javascript
    > db.sample.employee.dropIndexAsync("ageIndex")
    1051
    ```

2. Get the task information.

    ```lang-javascript
    > db.getTask(1051)
    ```

[^_^]:
    Links
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
