##NAME##

getNode - get the specified node of the current replication group

##SYNOPSIS##

**rg.getNode(\<nodename\>|\<hostname\>, \<servicename\>)**

##CATEGORY##

SdbReplicaGroup

##DESCRIPTION##

This function is used to get the specified node of the current replication group.

##PARAMETERS##

| Name | Type| Description | Required or not |
| ------ | -------- | ------ | -------- |
| nodename | string | Node name| Only one of the "nodename" and "hostname" can be selected. |
| hostname | string | Hostname  | Only one of the "hostname" and "nodename" can be selected. |
| servicename | string | Service name | required |

> **Note:** 
> 
> rg.getNode() defines two parameters, The first parameter is the "nodename " or "hostname", and the second parameter is the "servicename". The types of the two parameters are both string types and are required.  
> Format: ("\<nodename\>|\<hostname\>", "\<servicename\>").

##RETURN VALUE##

When the function executes successfully, it will return an object of type SdbNode.

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the [error code][error_code]. For more details, refer to [Troubleshooting][faq].

##VERSION##

v2.0 and above

##EXAMPLES##

Specify the host name and server name to get the specified node.

```lang-javascript
> var rg = db.getRG("group1")
> rg.getNode("hostname1", "11830")
hostname1:11830
```

[^_^]:
    Links
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md