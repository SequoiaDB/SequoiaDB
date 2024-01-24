
##名称##

setIniConfigs - 把配置信息写入 INI 文件。

##语法##

**Oma.setIniConfigs(\<config\>, \<configPath\>, [options])**

##类别##

Oma

##描述##

把配置信息写入 INI 文件。

##参数##

* `config` ( *Object*, *必填* )

   配置信息。

* `configPath` ( *String*, *必填* )

   INI 配置文件的路径。

* `options` ( *Object*, *选填* )

   解析配置文件的参数项.

   EnableType: true 是开启数据类型, false 是所有类型都视为字符串, 默认 false.

   StrDelimiter: true 是字符串用双引号, false 是字符串用单引号, null 是不输出字符串分隔符, 默认 true.

##返回值##

成功：无。

失败：抛出异常。

##版本##

v3.0.2及以上版本。

##示例##

1. 写入 INI 配置文件。

	```lang-javascript
	> Oma.setIniConfigs( { "a": 1, "b": true, "c": "hello"}, "/opt/config.ini" )
	```

2. 写入 INI 配置文件，并且开启数据类型。

	```lang-javascript
	> Oma.setIniConfigs( { "a": 1, "b": true, "c": "hello"}, "/opt/config.ini", { EnableType: true } )
	```