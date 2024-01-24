
##名称##

setLastErrObj(\<obj\>) - 以 bson 对象的方式，设置前一次操作的详细错误信息。

##语法##

**setLastErrObj(\<obj\>)**

##类别##

Global

##描述##

设置前一次操作的详细错误信息。

##参数##

* `obj` ( *Object*， *必填* )

	bson 错误对象。

	bson 错误对象有3个固定的字段：

	* errno: (Int32) 错误码。
	* description: (String) 错误码对应的描述。
	* detail: (String) 详细的错误描述信息。

##返回值##

无。

##版本##

v2.6及以上版本。

##示例##

设置前一次操作的详细错误信息

```lang-javascript
> db = new Sdb()
(nofile):0 uncaught exception: -15
> var err = getLastErrObj()
> var obj = err.toObj()
> println( obj.toString() )
{
	"errno": -15,
	"description": "Network error",
	"detail": ""
}
> obj["detail"] = Date() + ": " + obj["description"]
> setLastErrObj(obj)
```
