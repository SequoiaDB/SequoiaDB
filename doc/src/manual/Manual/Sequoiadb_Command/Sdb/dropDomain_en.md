##NAME##

dropDomain - drop a domain

##SYNOPSIS##

**db.dropDomain(\<name\>)**

##CATEGORY##

Sdb

##DESCRIPTION##

This function is used to drop the specified domain.

##PARAMETERS##

| Name | Type | Description | Required or not |
| ------ | ------ | ------ | ------ |
| name | string | Domain name. | required |

> **Note:**
>
> * The definition format of the dropDomain() function must specify the name parameter, and the value of name exists in the system, otherwise the operation is abnormal. 
> * Before dropping the domain, users must ensure that there is no data in the domain.
> * Does not support dropping system domain "SYSDOMAIN".

##RETURN VALUE##

When the function executes successfully, there is no return value.

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the [error code][error_code]. For more details, refer to [Troubleshooting][faq].

##VERSION##

v2.0 and above

##EXAMPLES##

* Drop an existing domain.

    ```lang-javascript
    > db.dropDomain('mydomain')
    ```

* Drop a domain containing the collection space and return an error:

    ```lang-javascript
    > db.dropDomain('hello')
    (nofile):0 uncaught exception: -256
    > getLastErrMsg(-256)
    Domain is not empty
    ```


[^_^]:
   links
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md