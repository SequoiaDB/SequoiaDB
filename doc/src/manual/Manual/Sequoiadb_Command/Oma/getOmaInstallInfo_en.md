
##NAME##

getOmaInstallInfo - Get the installation information from the installation file.

##SYNOPSIS##

**oma.getOmaInstallInfo()**

##CATEGORY##

Oma

##DESCRIPTION##

Get the installation information from the installation file. The installation file is specified by the getOmaInstallFile() interface.

##PARAMETERS##

NULL

##RETURN VALUE##

return the installation information

##ERRORS##

NULL

##HISTORY##

Since v1.0.

##EXAMPLES##

1. Get the installation information.

	```lang-javascript
	> var oma = new Oma( "localhost", 11790 )
	> oma.getOmaInstallInfo()
    {
    "NAME": "sdbcm",
    "SDBADMIN_USER": "sdbadmin",
    "INSTALL_DIR": "/opt/sequoiadb",
    "MD5": "0702f9916d37af0ae5917c0c34edbca3"
    }
    Takes 0.000532s.
 	```