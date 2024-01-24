
##名称##

reloadConfigs - sdbcm 重新加载其配置文件的内容，并使其生效。

##语法##

**oma.reloadConfigs()**

##类别##

Oma

##描述##

重新加载其配置文件的内容，并使其生效。

##参数##

无

##返回值##

无。

##错误##

无。

##版本##

v2.0及以上版本。

##示例##

1. 重新加载目标机器 sdbserver1 上 sdbcm 的配置文件。

	```lang-javascript
	> var oma = new Oma( "sdbserver1", 11790 )
	> oma.reloadConfigs()
	```