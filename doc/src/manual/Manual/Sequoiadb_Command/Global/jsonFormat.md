##名称##

jsonFormat- 设置BSON toString()形式,当打印BSON报内存不足时可尝试使用 “jsonFormat( false )”调整打印方式。

##语法##

**jsonFormat(\<pretty\>)**

##类别##

Global

##描述##

设置是否格式化打印BSON，即每行单独显示一个字段。
##参数##

* `pretty` ( *Bool*， *必填* )

  是否格式化打印json。

##返回值##

无。

##版本##

v2.6及以上版本。

##示例##

1. 通过jsonFormat()改变BSON打印格式。

	```lang-javascript
  	> db.sample.employee.find()
    {
      "_id": {
      "$oid": "59fac185e610b8510e000001"
      },
      "a": 1,
      "b": 2
    }
    Return 1 row(s).
    Takes 0.024873s.
    > jsonFormat( false )
    Takes 0.000225s.
    > db.sample.employee.find()
    { "_id": { "$oid": "59fac185e610b8510e000001" }, "a": 1, "b": 2 }
    Return 1 row(s).
    Takes 0.002948s.
  	```
