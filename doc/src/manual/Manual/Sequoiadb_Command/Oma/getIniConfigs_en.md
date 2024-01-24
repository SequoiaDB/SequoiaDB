
##NAME##

getIniConfigs - Get the INI file configuration information.

##SYNOPSIS##

**Oma.getIniConfigs(\<configPath\>, [options])**

##CATEGORY##

Oma

##DESCRIPTION##

Get the INI file configuration information.

##PARAMETERS##

* `configPath` ( *String*, *Required* )

   The path of INI file.

* `options` ( *Object*, *Optional* )

   Options for parsing configuration items.

   EnableType: true is enable type, false is all types are treated as strings, default false.

   StrDelimiter: true is string only supports double quotes, false is String supports double quotes and single quotes, default true.

##RETURN VALUE##

On success, return an object of BSONObj, which contains the configuration information of INI file.

On error, exception will be thrown.

##ERRORS##

the exceptions of `getIniConfigs()` are as below:

| Error Code | Error Type | Description | Solution |
| ------ | --- | ------------ | ----------- |
| -4 | SDB_FNE | File not exist. | Check the configuration file exists or not. |

##HISTORY##

Since v3.0.2.

##EXAMPLES##

1. print the configuration information of INI file.

	```lang-javascript
	> Oma.getIniConfigs( "/opt/config.ini" )
	{
		"datestyle": "iso, ymd",
		"listen_addresses": "*",
		"log_timezone": "PRC",
		"port": "1234",
		"shared_buffers": "128MB",
		"timezone": "PRC"
	}
	```

2. print the configuration information of INI file and enable type.

	```lang-javascript
	> Oma.getIniConfigs( "/opt/config.ini", { "EnableType": true } )
	{
		"datestyle": "iso, ymd",
		"listen_addresses": "*",
		"log_timezone": "PRC",
		"port": 1234,
		"shared_buffers": "128MB",
		"timezone": "PRC"
	}
	```