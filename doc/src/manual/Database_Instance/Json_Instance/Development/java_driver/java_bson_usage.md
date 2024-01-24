目前，SequoiaDB 巨杉数据库支持多种 BSON 数据类型，详情可参考[数据类型][data_type]一节。

##Java构造BSON数据类型##

* 整数/浮点数

  ```lang-java
  // Java BSON 构造整数/浮点数类型 {a:123, b:3.14}
  BSONObject obj = new BasicBSONObject();
  obj.put("a", 123);
  obj.put("b", 3.14);
  // or
  // BSONObject obj2 = new BasicBSONObject().append("a", 123).append("b", 3.14);
  // or
  // BSONObject obj3 = (BasicBSONObject) JSON.parse("{\"a\":123, \"b\":3.14}");
  ```

* 高精度数

  ```lang-java
  // Java BSON 构造不带精度要求的 Decimal 类型 {a:{"$decimal":"12345.067891234567890123456789"}}
  String str = "12345.067891234567890123456789";
  BSONObject obj = new BasicBSONObject();
  BSONDecimal decimal = new BSONDecimal(str);
  obj.put("a", decimal);
  // Java BSON 构造一个最多有 100 位有效数字，其中小数部分最多有 30 位的 Decimal 类型 {b:{"$decimal":"12345.067891234567890123456789", "$precision":[100, 30]}}
  BSONObject obj2 = new BasicBSONObject();
  BSONDecimal decimal2 = new BSONDecimal(str, 100, 30);
  obj2.put("b", decimal2);
  ```

* 字符串

  ```lang-java
  // Java BSON 构造字符串类型 {a:"hi"}
  BSONObject obj = new BasicBSONObject();
  obj.put("a", "hi");
  ```

* 空类型

  ```lang-java
  // Java BSON 构造空类型 {a:null}
  BSONObject obj = new BasicBSONObject();
  obj.put("a", null);
  ```
  
* 对象

  ```lang-java
  //Java BSON 构造嵌套对象类型 {b:{a:1}}
  BSONObject subObj = new BasicBSONObject();
  subObj.put("a", 1);
  BSONObject obj = new BasicBSONObject();
  obj.put("b", subObj);
  ```

* 数组

  ```lang-java
  // Java BSON 使用 org.bson.types.BasicBSONList 来构造数组类型 {a:[0,1,2]}
  BSONObject obj = new BasicBSONObject();
  BSONObject arr = new BasicBSONList();
  arr.put("0", 0);
  arr.put("1", 1);
  arr.put("2", 2);
  obj.put("a", arr);
  ```

* 布尔

  ```lang-java
  // Java BSON 构造布尔类型 {a:true, b:false}
  BSONObject obj = new BasicBSONObject();
  obj.put("a", true);
  obj.put("b", false);
  ```

* 对象 ID

   Java BSON 12 字节的 ObjectId 与[数据类型][data_type]一节介绍的对象 ID 略有不同，目前，Java ObjectId 的 12 字节内容由三部分组成：4 字节精确到秒的时间戳，4 字节系统（物理机）标示，4 字节由随机数起始的序列号。默认情况下，数据库为每条记录生成一个字段名为 _id 的唯一对象 ID。
  
  ```lang-java
  // Java BSON 构造对象 ID
  BSONObject obj = new BasicBSONObject();
  ObjectId id1 = new ObjectId();
  ObjectId id2 = new ObjectId("53bb5667c5d061d6f579d0bb");
  obj.put("_id", id1);
  ```
* 正则表达式

  使用正则表达式构造匹配条件，将序列号以“2001”开头的记录的 count 字段内容改为“1000”

  ```lang-java
  // Java BSON 构造正则表达式数据类型
  BSONObject matcher = new BasicBSONObject();
  Pattern obj = Pattern.compile("^2001",Pattern.CASE_INSENSITIVE);
  matcher.put("serial_num", obj);
  BSONObject modifier = new BasicBSONObject("$set", new BasicBSONObject("count",1000));
  cl.update(matcher, modifier, null);
  ```

  >**Note:**
  >
  >以上使用 Patten 构造的 bson matcher，当使用 matcher.toString()，内容为：
  >
  >```lang-json
  >{ "serial_num" : { "$options" : "i" , "$regex" : "^2001"}}
  >```
  >
  >通过以下 bson 构造方式也可以得到相同的内容：
  >
  >```lang-java
  >BSONObject matcher2 = new BasicBSONObject();
  >BSONObject obj2 = new BasicBSONObject();
  >obj2.put("$regex","^2001");
  >obj2.put("$options","i");
  >matcher2.put("serial_num", obj2);
  >```
  >
  >但是，通过后者构造出的 matcher2 的数据类型是一个普通的对象嵌套类型，而不是正则表达式类型。

* 日期

  ```lang-java
  // Java BSON 构造日期类型
  BSONObject obj = new BasicBSONObject();

  LocalDate localDate = LocalDate.of( 2022, 1, 1 );
  obj.put( "localDate", BSONDate.valueOf( localDate ) );
  ```

  >**Note:**
  >
  > * java.util.Date 的国际化处理不够完善，推荐使用 LocalDate 替代 java.util.Date。
  >
  > * 当日期带时间信息时，推荐使用的类型是 BSONTimestamp 类型，而不是 BSONDate 类型。

* 二进制

  ```lang-java
  // Java BSON 构造二进制类型
  BSONObject obj = new BasicBSONObject();
  String str = "hello world";
  byte[] arr = str.getBytes();
  Binary bindata = new Binary(arr);
  obj.put("bindata", bindata);
  ```


* 时间戳

  ```lang-java
  // Java BSON 构造时间戳类型
  String mydate = "2014-07-01 12:30:30.124232";
  String dateStr = mydate.substring(0, mydate.lastIndexOf('.'));
  String incStr = mydate.substring(mydate.lastIndexOf('.') + 1);
          
  SimpleDateFormat format = new SimpleDateFormat("yyyy-MM-dd HH:mm:ss");
  Date date = format.parse(dateStr);
  int seconds = (int)(date.getTime()/1000);
  int inc = Integer.parseInt(incStr);
  BSONTimestamp ts = new BSONTimestamp(seconds, inc);
          
  BSONObject obj = new BasicBSONObject();
  obj.put("timestamp", ts);
  ```


[^_^]:
    本文使用的所有链接及引用
[data_type]:manual/Distributed_Engine/Architecture/Data_Model/data_type.md
