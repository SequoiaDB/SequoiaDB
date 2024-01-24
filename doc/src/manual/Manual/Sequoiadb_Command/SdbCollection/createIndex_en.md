##NAME##

createIndex - create index

##SYNOPSIS##

**db.collectionspace.collection.createIndex(\<name\>, \<indexDef\>, [isUnique], [enforced], [sortBufferSize])**

**db.collectionspace.collection.createIndex(\<name\>, \<indexDef\>, [indexAttr], [option])**

##CATEGORY##

SdbCollection

##DESCRIPTION##

This function is used to create an [index][index] for the collection to improve query speed.User need to understand the [limitations][limitation] of indexes before careating.

##PARAMETERS##

- name ( *string, required* )

    Index name. It should be unique in a collection.

- indexDef ( *object, required* )

    Index key. The format is `{<index field>:<value>, ...}`. The optional type values are as follows:

    - 1: sort by field in ascending order. 
    - -1: sort by field in descending order.
    - "text": create [text index][text_index].

- isUnique ( *boolean, optional* )

    Whether the index is unique. The default value is false, means the index is not unique.

- enforced ( *boolean, optional* )

    Whether the index is mandatorily unique or not. The default value is false.

    - When it is true, cannot repeatedly insert records whose index field value is null.
    - It only becomes effective when the parameter "isUnique" is true.

- sortBufferSize ( *number, optional* )

    The size of sort buffer. The default value is 64, the unit is MB.

    - Zero means don't use sort buffer.
    - When the collection record data volume more than 10 million records, appropriately increasing the sort cache size can increase the speed of index creation. 

> **Note:**
>
> For text index, the parameters "isUnique", "enforced" and "sortBufferSize" are invalid.

- indexAttr ( *object, optional* )

    Index attributes can be set through the parameter "indexAttr":
    
    - Unique ( *boolean* ): Whether the index is unique. The defalut value is false.
    
        Format: `Unique: true`

    - Enforced ( *boolean* ): Whether the index is mandatorily unique. The defalut value is false.

        - When it is true, cannot repeatedly insert records whose index field value is null.
        - It only becomes effective when the parameter "Unique" is true.

        Format: `Enforced: true`
    
    - NotNull ( *boolean* ): Whether to allowed the index field to not-existent or be null when inserting a record. The defalut value is false.

        The values are as follows:

        - true: The index field must exist and the value cannot be null.
        - false: Allows the index field to not exist or the value be null.
    
        Format: `NotNull: true`

    - NotArray ( *boolean* ): Whether any field of index is allowed to be an array when inserting a record. The defalut value is false.

        - true: The value of the index field is not allowed to be an array.
        - false: The value of the index field is allowed to be an array.

        Format: `NotArray: true`
    
    - Standalone ( *boolean* ): Whether it is a standalone index. The defalut value is false, means is not a standalone index.

        When it is true, the parameter "NodeName", "NodeID" or "InstanceID" must be specified.

        Format: `Standalone: true`

- option ( *object, optional* )

    Control parameters can be set through the parameter "option":

    - SortBufferSize ( *number* ): The size of sort buffer. The default value is 64, the unit is MB.
    
        - Zero means don't use sort buffer.
        - When the collection record data volume more than 10 million records, appropriately increasing the sort cache size can increase the speed of index creation.

        Format: `SortBufferSize: 80`

    - NodeName ( *string/array* ): Data node name

        Format: `NodeName: "sdbserver:11820"`
    
    - NodeID ( *number/array* ): Data node ID

        Format: `NodeID: 1001`
    
    - InstanceID ( *number/array* ): Data node instance ID

        Format: `InstanceID: 100`

##RETURN VALUE##

When the function executes successfully, there is no return value.

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the [error code][error_code]. For more details, refer to [Troubleshooting][faq].

##VERSION##

v2.0 and above

##EXAMPLES##

* Create an unique index named "ageIndex", and specify the records are in ascending order on the field "age".

    ```lang-javascript
    > db.sample.employee.createIndex("ageIndex", {age: 1}, true)
    ```

* Create a text index named "addr_tag".

    ```lang-javascript
    > db.sample.employee.createIndex("addr_tags", {address: "text", tags: "text"})
    ```
   
* Create an unique index named "ab", and specify the parameter "NotNull" is true.

    ```lang-javascript
    > db.sample.employee.createIndex("ab", {a: 1, b: 1}, {Unique: true, NotNull: true})
    ```

    When the field "b" does not exist or the value is null, an error message will be returned.

    ```lang-javascript
    > db.sample.employee.insert({a: 1})
    sdb.js:625 uncaught exception: -339
    Any field of index key should exist and cannot be null
    
    > db.sample.employee.insert({a: 1, b: null})
    sdb.js:625 uncaught exception: -339
    Any field of index key should exist and cannot be null
    ```

* Create an index named "ab", and specify the parameter "NotArray" is true.

    ```lang-javascript
    > db.sample.employee.createIndex("ab", {a: 1, b: 1}, {NotArray: true})
    ```

    When the field "a" is array, an error message will be returned.

    ```lang-javascript
    > db.sample.employee.insert({a: [1], b: 10})
    sdb.js:645 uncaught exception: -364
    Any field of index key cannot be array
    ```

* Create a standalone index named "a" on the data node `sdbserver:11850`.

    ```lang-javascript
    > db.sample.employee.createIndex("a", {a: 1}, {Standalone: true}, {NodeName: "sdbserver:11850"})
    ```

[^_^]:
    Links
[index]:manual/Distributed_Engine/Architecture/Data_Model/index.md
[limitation]:manual/Manual/sequoiadb_limitation.md#索引
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
[text_index]:manual/Distributed_Engine/Architecture/Data_Model/text_index.md
