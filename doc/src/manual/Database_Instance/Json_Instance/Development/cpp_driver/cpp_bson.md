
使用 [C++ BSON](api/bsoncpp/html/index.html) 主要会接触到以下四个类：

-   bson::BSONObj：用于创建 BSONObj 对象

-   bson::BSONElement：BSONObj 对象由 BSONElement 对象组成，即 BSONElement 对象为 BSONObj 对象的字段或者元素，BSONElement 是键值对

-   bson::BSONObjBuilder：用于实例化 BSONObj 对象

-   bson::BSONObjlterator：用于遍历 BSONObj 对象中的元素

命名空间 bson 中定义了这些类的类型为：

  -   typedef bson::BSONElement be;

  -   typedef bson::BSONObj bo;

  -   typedef bson::BSONObjBuilder bob;

此外，用户还可以使用 [BASE64C API](api/bsoncpp/html/base64c_8h.html) 和 [FROMJSON API](api/bsoncpp/html/fromjson_8hpp.html) 来帮助构建 C++ BSON。

##建立对象##

下述示例将简单介绍如何创建 C++ BSON 实例，详细内容可参考 [C++ BSON API](api/bsoncpp/html/index.html)。

* 使用 BSONObject 和 BSONObjBuilder 建立对象

  ```lang-cpp
  #include "client.hpp"

  using namespace bson ;
  BSONObj obj ;
  BSONObjBuilder b ;

  b.append("name","sam") ;
  b.append("age","24") ;
  obj = b.obj() ;
  // 或者使用如下语句：
  // obj = BSONObjBuilder().genOID().append("name","sam").append("age",24).obj() ;
  ```

 也可以通过数据流的方法建立 BSONObj 对象

  ```lang-cpp
  BSONObj obj ;
  BSONObjBuilder b ;
  b << "name" << "sam" << "age" << "24" ;
  obj = b.obj() ;
  ```

* 使用宏 BSON 建立对象

  C++ BSON 中定义还定义了一个 BSON 的宏，用于快速建立 BSONObj 对象

  ```lang-cpp
  BSONObj obj ;
  // int32
  obj = BSON( "a" << 1 ) ;
  // float
  obj = BSON( "b" << 3.14159265359 ) ;
  // string
  obj = BSON( "sample" << "employee" ) ;
  // OID
  obj = BSON( GENOID ) ;
  // boolean
  obj = BSON( "flag" << true << "ret" << false ) ;
  // object
  obj = BSON( "d" << BSON("e" << "hi!") ) ;
  // array
  obj = BSON( "phone" << BSON_ARRAY( "13800138123" << "13800138124" ) ) ;
  // others, less then, greater then, etc
  obj = BSON( "g" << LT << 99 ) ;
  ```


* 使用 fromjson 接口建立对象

  通过 `fromjson.hpp` 中的 fromjson() 将 json 字符串转换成 BSONObj 对象

  ```lang-cpp
  string s("{name:\"sam\"}") ;
  fromjson ( s, obj ) ;
  或者
  const char *r ="{ \
                       firstName:\"Sam\", \
                       lastName:\"Smith\",age:25,id:\"count\", \
                       address:{streetAddress: \"25 3ndStreet\", \
                       city:\"NewYork\",state:\"NY\",postalCode:\"10021\"}, \
                       phoneNumber:[{\"type\": \"home\",number:\"212555-1234\"}] \
                    }" ;
  fromjson ( r, obj ) ;
  ```
