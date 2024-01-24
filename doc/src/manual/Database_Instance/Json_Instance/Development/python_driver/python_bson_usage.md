目前，SequoiaDB 巨杉数据库支持多种 BSON 数据类型。详情可参考[数据类型][data_type]一节。

##Python构造BSON数据类型##

对于一些特殊类型如对象 ID 类型，在构造 bson 数据时需要使用 Python 驱动中对应的类去构造，如对象 ID 类型对应着 ObjectId，特殊类详情可参考 [Python API][api]。以下为构造示例。

* 整数/浮点数

   ```lang-python
   # Python BSON 构造整数/浮点数对象类型 {"a": 123,"b":3.14}
   doc = {"a": 123,"b":3.14}
   ```
* 字符串

   ```lang-python
   # Python BSON 构造字符串对象类型 {"key":"hello word"}
   doc = {"key":"hello word"} 
   ```

* 列表

   ```lang-python
   # Python BSON 构造列表对象类型 {"key":[1,2,3]}
   sublist = [1,2,3]
   doc = {"key":sublist }
   ```

* None

   ```lang-python
   # Python BSON 构造 None 类型 {"key":null}，None 在数据库中会以 null 值存储
   doc = {"key":None }
   ```

* 对象

   ```lang-python
   # Python BSON 构造嵌套对象类型 {"b":{"a":1}}
   subObj = {"a":1}
   doc = {"b": subObj}
   # or
   # doc = {"b":{"a":1}}
   ```

* 对象 ID

   ```lang-python
   # Python BSON 构造对象 ID 类型，建议 ObjectId() 不传入参数，这样会自动生成一个 id 值
   oid = ObjectId()
   doc = {"_id" : oid}

   # 传入一个 12 字节的 str 类型参数
   oid = ObjectId("5d035e2bb4d450b04fcd0dff")
   doc = {"_id" : oid}

   # 传入由 24 个字符组成的 Unicode 编码参数
   oid = ObjectId(u'5d035e2bb4d450b04fcd0dff') 
   doc = {"_id" : oid}
   ```

* 二进制数据

   ```lang-python
   # Python BSON 构造二进制数类型 {"rest": {"$binary": "QUJD","$type": "0"}}
   binary = Binary("ABC")
   doc = {"rest" : binary}
   # 在构造二进制类型的数据时候也可以设置 "$type" 的值，未设置时默认值为 0
   subtype = 0
   binary = Binary("ABC",subtype)
   doc = {"rest" : binary}
   ```

* 高精度数

   ```lang-python
   # Python BSON 构造不带精度要求的 Decimal 类型 {"rest":{"$decimal":"12345.067891234567890123456789"}}
   decimalObj = Decimal("12345.067891234567890123456789")
   doc = { "rest":decimalObj }
   # Python BSON 构造一个最多有 100 位有效数字，其中小数部分最多有 30 位的 Decimal 类型 {"rest":{"$decimal":"12345.067891234567890123456789", "$precision":[100, 30]}}
   decimalObj =  Decimal("12345.067891234567890123456789", 100, 30)
   doc = { "rest":decimalObj  }
   ```

* 正则表达式

   ```lang-python
   # Python BSON 构造正则表达式数据类型 {"regex":{"$regex": "\^w","$options": ""}
   regexObj = Regex("^w")
   doc = {"regex":regexObj }
   # 在构造正则表达式类型的数据时，可以设置 "$options"，未设置时默认为 0
   flags = 0
   regexObj = Regex("^w",flags)
   doc = {"regex":regexObj }
   ```

* 日期

   ```lang-python
   # Python BSON 构造日期类型 {date:{"$date": "2019-07-12"}}
   import datetime
   date = datetime.datetime(2019,07,12)
   doc = {"date":date}
   ```

* 时间戳

   ```lang-python
   # Python BSON 构造时间戳类型 {"rest": {"$timestamp": "2003-12-01-08.00.00.000000"}}
   time = datetime.datetime(2003,12,01,8,00,00)
   timestamp = Timestamp(time,0)
   doc = {"rest":timestamp}
   ```

##注意事项##

在构建一些特殊类型的数据时（如对象 ID），如果是直接构建（如下面 condition 所示），数据在进行 dict 转 bson 时会被认为是普通的嵌套类型（而不是对象 ID 类型）

```lang-python
condition = {"_id" : {"$oid":"5d035e2bb4d450b04fcd0dff“}}
```

* 错误示例

   构建一个对象 ID 类型作为删除匹配条件，但是由于采用了直接构建的方式，导致所构建数据为普通嵌套类型，从而匹配不到任何数据记录

  	```lang-python
	cl = db.get_collection("sample.employee")
	condition = {"_id":{"$oid":"5d035e2bb4d450b04fcd0dff"}}
	cl.delete ( condition=condition )
	```

* 正确示例

   需要使用 ObjectId 类构建对应的对象 ID，然后再执行删除操作

	 ```lang-python
	from bson import * 
	oid = ObjectId("5d035e2bb4d450b04fcd0dff")
	condition = {"_id":oid}
	cl.delete ( condition=condition )
	  ```

其他特殊类型，如二进制数据、高精度数、日期、时间戳等，使用方式与对象 ID 类型类似，都需使用 Python 驱动的对应的构造方法来构造。


[^_^]:
     本文使用的所有引用及链接
[data_type]:manual/Distributed_Engine/Architecture/Data_Model/data_type.md
[api]:api/python/html/index.html