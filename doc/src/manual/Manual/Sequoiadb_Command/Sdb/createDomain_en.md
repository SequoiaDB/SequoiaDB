##NAME##

createDomain - create a domain

##SYNOPSIS##

**db.createDomain(\<name\>, \<groups\>, [options])**

##CATEGORY##

Sdb

##DESCRIPTION##

This function is used to create a domian, which can contain several replication groups.

##PARAMETERS##

| Name | Type | Description | Is it require |
| ------ | ------ | ------ | ------ |
| name | string | Domain name, globally unique. | yes |
| groups | array | Replication groups contained in the domian. | no |
| options | object | Users can set other attributes through options when creating a domain. | no |

The attributes that can be set through options are as follows:

| Name | Description | Format |
| ------ | ------ | ------ |
| AutoSplit | Whether to automatically split the Hash Partitioning collection. | AutoSplit: true |

> **Note:**
>
> Users cannot create a collection space in the airspace(not including the replication group).

##RETURN VALUE##

When the function executes successfully, it will return an object of type SdbDomain.

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the [error code][error_code]. For more details, refer to [Troubleshooting][faq].

##VERSION##

v2.0 and above

##EXAMPLES##

* Create a domain containing two replication groups.

    ```lang-javascript
    > db.createDomain('mydomain', ['group1', 'group2'])
    ```

* Create a domain containing two replication groups, and specify automatic split.

    ```lang-javascript
    > db.createDomain('mydomain', ['group1', 'group2'], {AutoSplit: true})
    ```


[^_^]:
   links
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md