##NAME##

createProcedure - create a stored procedure

##SYNOPSIS##

**db.createProcedure(\<code\>)**

##CATEGORY##

Sdb

##DESCRIPTION##

This function is used to create a stored procedure in a database object.

##PARAMETERS##

code ( *function, required* )

Specify a custom function that conforms to JavaScript syntax. The format is as follows:

```lang-javascript
function functionName(parameters) {
  function body
}
```

- Other functions can be called in the function body. If users call a function that does not exist, users need to ensure that the relevant function already exists when the stored procedure is running.

- The function name is globally unique and does not support overloading.

- Each function is available globally, and random deletion of a function may cause the associated stored procedure to fail.

- The stored procedure will shield all standard output and standard error. At the same time, it is not recommended to add output statements during function definition or execution. A large amount of output may cause the stored procedure to fail. 

- The return value of a function can be any data type except Sdb.

> **Note:**
>
> The cluster deployed in standalone mode does not support the creation of stored procedures.

##RETURN VALUE##

When the function executes successfully, there is no return value.

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError][getLastError] to get the [error code][error_code]. For more details, refer to [Troubleshooting][faq].

##VERSION##

v2.0 and above

##EXAMPLES##

The following records exist in the collection sample.employee:

```lang-json
{id: 1, Name: "Jack", age: 25}
```

1. Connect the coordination node.

    ```lang-javascript
    > var coord = new Sdb("sdbserver", 11810)
    ```

2. Create a stored procedure.

    ```lang-javascript
    > coord.createProcedure(function getAll() { return db.sample.employee.find(); })
    ```

    > **Note:**
    >
    > db has been initialized in the stored procedure. Users can use the global db to refer to the authentication information corresponding to the session  in which the stored procedure is executed.

3. Execute the stored procedure through [eval()][eval].

    ```lang-javascript
    > coord.eval("getAll()")
    {
      "_id": {
        "$oid": "60cd4c2e1a52b21546a15826"
      },
      "id": 1,
      "Name": "Jack",
      "age": 25
    }
    ```

Users can view the created stored procedure information through [listProcedures()][listProcedures].


[^_^]:
   links
[listProcedures]:manual/Manual/Sequoiadb_Command/Sdb/listProcedures.md
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[eval]:manual/Manual/Sequoiadb_Command/Sdb/eval.md
[error_code]:manual/Manual/Sequoiadb_error_code.md