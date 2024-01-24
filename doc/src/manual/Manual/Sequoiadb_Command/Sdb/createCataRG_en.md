
##NAME##

createCataRG - create a new catalog replication group

##SYNOPSIS##
**db.createCataRG( \<host\>, \<service\>, \<dbpath\>, [config] )**

##CATEGORY##

Sdb

##DESCRIPTION##

This function is used to create a new catalog replication group, and at the same time, create and start a catalog ndoe. Only one catalog replication group can be created in the same cluster, and the group name is "SYSCatalogGroup" by default.

##PARAMETERS##

- host ( *string, required* )

   Specify the host name of the catalog node.

- service ( *int32/string, required* )

   Specify the service port of the catalog node, make sure that the port number and the next five port numbers  are not occupied; if the designated service port is 11800, make sure that none of the 11800/11801/11802/11803/11804/11805 ports are occupied.

- dbpath ( *string, required* )

   Specify the data file path for storing catalog data files. It is recommended that users fill in the absolute path, and ensure that the database administrator user ( created during installation with the id sdbadmin by default ) has the written permission for the path. 

- config ( *object, optional* )

   Specify the detailed parameters that need to be configured. For detailed parameters, refer to [Database Configuration][configuration].


##RETURN VALUE##

When the function executes successfully, there is no return value.

When the function fails, an exception will be thrown and an error message will be printed.
 
##ERRORS##

When the exception happens，use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the [error code][error_code]. For more details, refer to [Troubleshooting][faq].

##HISTORY##

v2.0 and above

##EXAMPLES##

Create a catalog replication group on the host named "hostname1", specify the service port as 11800, specify the data file storage path as `/opt/sequoiadb/database/cata/11800`, and configure the log file size as 64MB.

```lang-javascript
> db.createCataRG("hostname1",11800,"/opt/sequoiadb/database/cata/11800",{logfilesz:64})
```


[^_^]:
    links
[configuration]:manual/Distributed_Engine/Maintainance/Database_Configuration/configuration_parameters.md
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md