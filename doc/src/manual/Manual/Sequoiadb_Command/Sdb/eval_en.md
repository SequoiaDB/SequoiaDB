##NAME##

eval - call the stored procedure

##SYNOPSIS##

**db.eval(\<code\>)**

##CATEGORY##

Sdb

##DESCRIPTION##

This function is used to call the stored peocedure that has been created in a statement. Users can fill in JavaScript sentences according to needs.

##PARAMETERS##

| Name | Type | Description | Required or not |
| ------ | ------ | ------ | ------ |
| name | string | JavaScript statement or created stored procedure function.  | required |

> **Note:**
>
> The stored procedure will shield all standard output and error. At the same time, it is not recommended to add output statements during function execution. A large amount of output may cause the stored procedure to fail.

##RETURN VALUE##

- When the function executes successfully, the result will be returned according to the statement. The return value can be directly assigned to another variable, such as: `var a = db.eval( 'db.sample.employee' ); a.find(); `. 

- When the function fails, an exception will be thrown and an error message will be printed.

- The operation will not return until the function execution ends. Quitting halfway terminates the entire execution, but the code that has been executed will not be rolled back.

- The length of the custom return value has a certain limit, refer to the maximum length of the SequoiaDB inserted record.

- Support for defining temporary functions, such as: `db.eval( 'function sum(x,y){return x+y;} sum(1,2)' )`.

- The use of global db is same as [createProcedure()][createProcedure].

##ERRORS##

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the [error code][error_code]. For more details, refer to [Troubleshooting][faq].

##VERSION##

v2.0 and above

##EXAMPLES##

* Call the stored procedure function "sum" through eval().

    ```lang-javascript
    //Initially, the sum() method does not exist, and the exception information is returned.
    > var a = db.eval( 'sum(1,2)' );
    { "errmsg": "(nofile):1 ReferenceError: getCL is not defined", "retCode": -152 }
    (nofile):0 uncaught exception: -152
    //Initialize sum().
    > db.createProcedure( function sum(x,y){return x+y;} )
    //Call sum().
    > db.eval( 'sum(1,2)' )
    3
    ```

* Fill in the JavaScript statement through eval() and execute the statement.

    ```lang-javascript
    > var ret = db.eval( "db.sample.employee" )
    > ret.find()
    {
      "_id": {
        "$oid": "5248d3867159ae144a000000"
      },
      "a": 1
    }
    {
      "_id": {
        "$oid": "5248d3897159ae144a000001"
      },
      "a": 2
    }...
    ```


[^_^]:
   links
[createProcedure]:manual/Manual/Sequoiadb_Command/Sdb/createProcedure.md
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
