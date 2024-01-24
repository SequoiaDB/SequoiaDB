
##名称##

close - 关闭 Oma 连接对象。

##语法##

**oma.close()**

##类别##

Oma

##描述##

关闭 Oma 连接对象。

##参数##

无

##返回值##

成功：无。  

失败：无。

##错误##

无。

##版本##

v2.0及以上版本。

##示例##

1. 连接到本地的集群管理服务进程 sdbcm，关闭 oma。

	```lang-javascript
	> var oma = new Oma( "localhost", 11790 )
	> oma.close()
	```