##NAME##

createIndexAsync - create index asynchronously

##SYNOPSIS##

**db.collectionspace.collection.createIndexAsync\(\<name\>, \<indexDef\>, \[indexAttr\], \[option\])**

##CATEGORY##

SdbCollection

##DESCRIPTION##

This function is used to create an index for the collection asynchronously to improve query speed.

##PARAMETERS##

- name ( *string, required* )

    Index name. It should be unique in a collection.

- indexDef ( *object, required* )

    Index key. It contains one or more objects that specify index fields and order direction. "1" means ascending order. "-1" means descending order.

- indexAttr ( *object, optional* )

    Index attributes can be set through the parameter:
    
    - Unique ( *boolean* ): Whether the index is unique. The defalut value is false.
    
    - Enforced ( *boolean* ): Whether the index is mandatorily unique. The defalut value is false.
    
    - NotNull ( *boolean* ): Whether any filed of index can be null or not exist. The defalut value is false.
    
    - NotArray ( *boolean* ): Whether any filed of index can array. The defalut value is false.

- options ( *object, optional* )

    Other optional parameters can be set through the options parameter:

    - SortBufferSize ( *number* ): The size of sort buffer used when creating index. The defalut value is 64 MB.


> **Note:**
>
> - There should not be any exactly same records in the fields that are specified by the unique index in a collection.
> - Index name should not be null string. It should not contain "." or "$". The length of it should be no more than 127B.
> - When the collection record data volume is large(more than 10 million records), appropriately increasing the sort cache size can increase the speed of index creation.
> - For text index, the parameters isUnique, enforced and sortBufferSize are meaningless.

##RETURN VALUE##

When the function executes successfully, it will return an object of type number.  Users can get a task ID through this object.

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the [error code][error_code]. For more details, refer to [Troubleshooting][faq].

##VERSION##

v3.6 and above

##EXAMPLES##

1. Create a unique index in the collection sample.employee.

    ```lang-javascript
     > db.sample.employee.createIndexAsync("ab", {a: 1, b: 1}, {Unique: true})
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
[standalone]:manual/Distributed_Engine/Architecture/Data_Model/index.md#创建索引
