
##名称##

getOmaInstallFile - 获取安装信息文件。

##语法##

**oma.getOmaInstallFile()**

##类别##

Oma

##描述##

一旦 SequoiaDB 被安装到机器上，安装信息便被写入到一个文件中，默认为/etc/default/sequoiadb。当SequoiaDB 被卸载时，该文件也会被删除。可通过当前接口获取这个文件名。

##参数##

NULL

##返回值##

安装信息文件名。默认为/etc/default/sequoiadb。

##错误##

NULL

##版本##

v1.0及以上版本。

##示例##

1. 获取安装信息文件。

	```lang-javascript
	> Oma.getOmaInstallFile()
	/etc/default/sequoiadb
 	```