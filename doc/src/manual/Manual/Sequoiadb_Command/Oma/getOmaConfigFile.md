
##名称##

getOmaConfigFile - 获取 sdbcm 的配置文件。

##语法##

**oma.getOmaConfigFile()**

##类别##

Oma

##描述##

获取 sdbcm 的配置文件，该文件默认情况为安装目录（默认为/opt/sequoiadb/）的conf/sdbcm.conf文件。可通过修改该配置文件来设置 sdbcm 服务的行为。

##参数##

NULL

##返回值##

sdbcm 配置文件名。默认为/opt/sequoiadb/conf/sdbcm.conf。

##错误##

NULL

##版本##

v1.0及以上版本。

##示例##

1. 获取 sdbcm 配置文件。

	```lang-javascript
	> Oma.getOmaConfigFile()
	/opt/sequoiadb/bin/../conf/sdbcm.conf
 	```