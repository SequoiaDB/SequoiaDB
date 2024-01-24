##NAME##

putLob - put LOB in the collection

##SYNOPSIS##

**db.collectionspace.collection.putLob\(\<filepath\>, [oid]\)**

##CATEGORY##

SdbCollection

##DESCRIPTION##

This function is used to insert LOB in the collection.

##PARAMETERS##

| Name | Type| Description | Required or not |
| ---- | --- | ----------- | --------------- |
| filepath | string | The full path of the local file to be uploaded. | required |
| oid | string |  Unique identifier of LOB. | not |

##RETURN VALUE##

When the function executes successfully, it will return an oid string of type String. Users can perform related operations on LOB through oid.

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the [error code][error_code]. For more details, refer to [Troubleshooting][faq].

##VERSION##

v3.4 and above

##EXAMPLES##

- Insert the file `mylob.txt` into the collection "sample.employee" as a LOB, and specify the LOB "oid".

    ```lang-javascript
    > db.sample.employee.putLob('/opt/mylob/mylob.txt', '5bf3a024ed9954d596420256')
    5bf3a024ed9954d596420256
    ```

    >**Note:**
    >
    > Users can create a LOB oid through [createLobID()][createLobID].

- Insert the file `mylob.txt` into the collection "sample.employee" as a LOB, without specifying the LOB "oid".

    ```lang-javascript
    > db.sample.employee.putLob('/opt/mylob/mylob.txt')
    0000604f989a390002db009e
    ```


[^_^]:
     Links
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
[createLobID]:manual/Manual/Sequoiadb_Command/SdbCollection/createLobID.md