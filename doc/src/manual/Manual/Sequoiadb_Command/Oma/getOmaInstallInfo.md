
##名称##

getOmaInstallInfo - 从安装信息文件中获取安装信息。

##语法##

**oma.getOmaInstallInfo()**

##类别##

Oma

##描述##

从安装信息文件中获取安装信息，安装文件为[getOmaInstallFile()](manual/Manual/Sequoiadb_Command/Oma/getOmaInstallFile.md)指定的文件。

##参数##

无

##返回值##

成功：返回安装信息。  

失败：无。

##错误##

无。

##版本##

v2.0及以上版本。

##示例##

1. 获取本机的 SequoiaDB 的安装信息。

	```lang-javascript
	> var oma = new Oma( "localhost", 11790 )
	> oma.getOmaInstallInfo()
    {
    "NAME": "sdbcm",
    "SDBADMIN_USER": "sdbadmin",
    "INSTALL_DIR": "/opt/sequoiadb",
    "MD5": "0702f9916d37af0ae5917c0c34edbca3"
    }
	```