##NAME##

close - close the current cursor

##SYNOPSIS##

**cursor.close()**

##CATEGORY##

SdbCursor

##DESCRIPTION##

This function is used to close the current cursor. After closing, the cursor will not be able to get records.

##PARAMETERS##

None

##RETURN VALUE##

When the function executes successfully, there is no return value.

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the [error code][error_code]. For more details, refer to [Troubleshooting][faq].

##VERSION##

v3.0 and above

##EXAMPLES##

1. Insert 10 records.

    ```lang-javascript
    > for(i = 0; i < 10; i++) {db.sample.employee.insert({a: i})}
    ```

2. Query all records in the collection "sample.employee".

    ```lang-javascript
    > var cur = db.sample.employee.find()
    ```

3. Use the cursor to fetch a record.

    ```lang-javascript
    > cur.next()
    {
         "_id": {
         "$oid": "53b3c2d7bb65d2f74c000000"
         },
         "a": 0
    }
    ```

4. Close cursor.

    ```lang-javascript
    > cur.close()
    ```

5. Get the next record again, no result is returned.

    ```lang-javascript
    > cur.next()
    ```

[^_^]:
    Links
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md