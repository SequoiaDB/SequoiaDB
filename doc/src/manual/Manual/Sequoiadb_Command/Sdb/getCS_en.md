##NAME##

getCS - get the specified collecion space

##SYNOPSIS##

**db.getCS(\<name\>)**

##CATEGORY##

Sdb

##DESCRIPTION##

This function is used to get the specified collecion space.

##PARAMETERS##

|Name      |type        |Description  |Required or not |
|--------- |----------- |------------ |----------|
| name | string | Collection space name. Within the same database, the collection space name is unique. | required |

> **Note:**
>
> * The value of the name field cannot be an empty string, and it cannot contain dots(.) or dollar signs($). The length of name field cannot exceed 127B.
> * The collection space exists in the data object.

##RETURN VALUE##

When the function executes successfully,  it will return an object of type SdbCS. 

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the [error code][error_code]. For more details, refer to [Troubleshooting][faq].

##VERSION##

v2.0 and above

##EXAMPLES##

Return the reference of the collection space "sample", assuming that the "sample" already exists.

```lang-javascript
> db.getCS("sample")
```


[^_^]:
   links
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md