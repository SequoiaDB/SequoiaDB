
##NAME##

getOmaInstallFile - Get the installation information file.

##SYNOPSIS##

**oma.getOmaInstallFile()**

##CATEGORY##

Oma

##DESCRIPTION##

Get the installation information file. After SequoiaDB is installed in the host, the installation information is wrote the file /etc/default/sequoiadb. When SequoiaDB is uninstalled from host, this file will be remove.

##PARAMETERS##

NULL

##RETURN VALUE##

return the installation information file name.

##ERRORS##

NULL

##HISTORY##

Since v1.0.

##EXAMPLES##

1. Get the installation information file name.

	```lang-javascript
	> Oma.getOmaInstallFile()
	/etc/default/sequoiadb
 	```