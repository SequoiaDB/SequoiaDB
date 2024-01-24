
##NAME##

setIniConfigs - Set the INI file configuration information.

##SYNOPSIS##

**Oma.setIniConfigs(\<config\>, \<configPath\>, [options])**

##CATEGORY##

Oma

##DESCRIPTION##

Set the INI file configuration information.

##PARAMETERS##

* `config` ( *Object*, *Required* )

   The configuration information.

* `configPath` ( *String*, *Required* )

   The path of INI file.

* `options` ( *Object*, *Optional* )

   Options for parsing configuration items.

   EnableType: true is enable type, false is all types are treated as strings, default false.

   StrDelimiter: true is string with double quotes, false is string with single quotes, null is no string delimiter, default true.

##RETURN VALUE##

On success, nothing returns.

On error, exception will be thrown.

##HISTORY##

Since v3.0.2.

##EXAMPLES##

1. set the configuration information of INI file.

	```lang-javascript
	> Oma.setIniConfigs( { "a": 1, "b": true, "c": "hello"}, "/opt/config.ini" )
	```

2. set the configuration information of INI file and enable type.

	```lang-javascript
	> Oma.setIniConfigs( { "a": 1, "b": true, "c": "hello"}, "/opt/config.ini", { EnableType: true } )
	```