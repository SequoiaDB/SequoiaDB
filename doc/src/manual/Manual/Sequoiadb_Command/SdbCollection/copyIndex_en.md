##NAME##

copyIndex - copy index

##SYNOPSIS##

**db.collectionspace.collection.copyIndex([subCLName], [indexName])**

##CATEGORY##

SdbCollection

##DESCRIPTION##

This function is used to copy the indexes of main collection into the subcollection.

##PARAMETERS##

- subCLName ( *string, optional* )

    Specify the name of subcollection. The fomat is \<csname\>.\<clname\>, and the default value is null, which means all subcollections of the main collection.

- indexName ( *string, optional* )

    Specify the name of index. The default value is null, which means all indexes of the main collection.

##RETURN VALUE##

When the function executes successfully, there is no return value.

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

When the exception happens，use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the [error code][error_code]. For more details, refer to [Troubleshooting][faq].

##VERSION##

v3.6 and above

##EXAMPLES##

1. Create an index named "IDIdx" in the main collection sample.employee.

    ```lang-javascript
    > db.sample.employee.createIndex("IDIdx", {ID: 1})
    ```

2. Copy the index of main collection into the subcollection.

    ```lang-javascript
    > db.sample.employee.copyIndex()
    ```

3. Check the index of the subcollection sample.January. It indicates that the index  "IDIdx" has been added.

    ```lang-javascript
    > db.sample.January.listIndexes()
    {
      "IndexDef": {
        "name": "ID",
        "key": {
          "ID": 1
        },
        ...
      },
      ...
    }
    ```

[^_^]:
   links
[limitation]:manual/Manual/sequoiadb_limitation.md
[datasource]:manual/Distributed_Engine/Architecture/datasource.md
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md