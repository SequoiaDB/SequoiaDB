##名称##

getLastErrObj - 以 bson 对象的方式，返回前一次操作的详细错误信息。

##语法##

**getLastErrObj()**

##类别##

Global

##描述##

获取前一次操作的详细错误信息。

##参数##

无。

##返回值##

若前一次操作发生错误，该函数以 BSON 对象的形式返回错误信息。否则，无返回值（即void）。BSON 对象包含的字段具体如下：

* errno: (Int32) 错误码。
* description: (String) 错误码对应的描述。
* detail: (String) 详细的错误描述信息。
* ErrNodes: (BSON object) 描述哪些数据节点发生了错误，及错误的详细信息（该字段为扩展字段，仅当错误发生在数据节点时才返回该字段）。

##版本##

v2.6及以上版本。

##示例##

通过 getLastErrObj() 获取前一次操作的详细错误信息。当错误发生在数据节点时，返回的错误信息会带有 ErrNodes 字段描述。

```lang-javascript
> db.sample.employee.createIndex("A",{"a":1})
(shell):1 uncaught exception: -247
Redefine index
> var err = getLastErrObj()
> var obj = err.toObj()
> println( obj.toString() )
{
  "errno": -247,
  "description": "Redefine index",
  "detail": "",
  "ErrNodes": [
    {
      "NodeName": "localhost:11830",
      "GroupName": "group2",
      "Flag": -247,
      "ErrInfo": {
        "errno": -247,
        "description": "Redefine index",
        "detail": ""
      }
    }
  ]
}
```