##NAME##

listLobs - list the LOBs in the collection

##SYNOPSIS##

**db.collectionspace.collection.listLobs([SdbQueryOption])**

##CATEGORY##

Collection

##DESCRIPTION##

Users can use this function to get the LOBs in the collection, and the obtained result is returned by cursor.

##PARAMETERS##

SdbQueryOption( *Object*， *Optional* )
    
Use an object to specify record query parameters, and the usage can refer to [SdbQueryOption](reference/Sequoiadb_command/AuxiliaryObjects/SdbQueryOption.md).
    

>**Note:**
>
> When using SdbQueryOption to specify 	hint as {"ListPieces": 1}, users can get 	detailed sharding information about the Lob.

##RETURN VALUE##

When the function executes successfully, it returns the DBCursor object.

When the function fails, an exception will be thrown and an error message will be printed.

##ERROR##

The common exceptions of `listLobs()` function are as follows：

| Error Code | Error Type | Description | Solution |
| ------ | ------ | --- | ------ |
| -6 | SDB_INVALIDARG | Parameter error | Check whether the parameter are filled in currectly. |
| -34 | SDB_DMS_CS_NOTEXIST | Collection space does not exist | Check whether the colletion space exists. |
| -23 | SDB_DMS_NOTEXIST| Collection does not exist | Check whether the colletion exists.|

When the exception happens, use [getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md) to get the error message or use [getLastError()](reference/Sequoiadb_command/Global/getLastError.md) to get error code. For more details, refer to [Troubleshooting](troubleshooting/general/general_guide.md).

##VERSION##

The function is applicable to v2.0 and above, of which v3.2 and above support obtaining the specified LOB through input parameters.

##EXAMPLES##

* List all LOBs in sample.employee.

    ```lang-javascript
    > db.sample.employee.listLobs()
    {
       "Size": 2,
       "Oid": {
         "$oid": "00005d36c8a5350002de7edc"
       },
       "CreateTime": {
         "$timestamp": "2019-07-23-16.43.17.360000"
       },
       "ModificationTime": {
         "$timestamp": "2019-07-23-16.43.17.508000"
       },
       "Available": true,
       "HasPiecesInfo": false
     }
     {
       "Size": 51717368,
       "Oid": {
         "$oid": "00005d36cae8370002de7edd"
       },
       "CreateTime": {
         "$timestamp": "2019-07-23-16.52.56.278000"
       },
       "ModificationTime": {
         "$timestamp": "2019-07-23-16.52.56.977000"
       },
       "Available": true,
       "HasPiecesInfo": false
    }
    Return 2 row(s).
    ```

* List LOBs with size greater than 10 in sample.employee.

    ```lang-javascript
    > db.sample.employee.listLobs( SdbQueryOption().cond( { "Size": { $gt: 10 } } ) )
    {
       "Size": 51717368,
       "Oid": {
         "$oid": "00005d36cae8370002de7edd"
       },
       "CreateTime": {
         "$timestamp": "2019-07-23-16.52.56.278000"
       },
       "ModificationTime": {
         "$timestamp": "2019-07-23-16.52.56.977000"
       },
       "Available": true,
       "HasPiecesInfo": false
    }
    Return 1 row(s).
    ```