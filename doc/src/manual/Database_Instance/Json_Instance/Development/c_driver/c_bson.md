 
BSON 是 JSON 的二进制表现形式，通过记录每个对象、元素及嵌套元素、数组的类及长度，提供高速有效地进行某个元素的查找。因此，在 C 和 C++ 中使用 BSON 官方提供的 BSON 接口进行数据存储。详情可参考 [BSON](api/bson/html/index.html)。

与普通的 JSON 不同，BSON 提供更多的数据类型，以满足 C/C++ 语言多种多样的需求。SequoiaDB 巨杉数据库提供了包括 8 字节浮点数（double）、字符串、嵌套对象、嵌套数组、对象 ID（数据库中每个集合中每条记录都有一个唯一 ID）、布尔值、日期、NULL、正则表达式、4 字节整数（int），时间戳，以及 8 字节整数等数据类型。这些类型的定义可以在  `bson.h` 中的 bson_type 找到。详情可查看 [C BSON API](api/bson/html/index.html)。

> **Note:**
>
> 使用 C BSON API 函数构建 BSON 出错时，API 将返回错误码表示构建失败。用户应适当检测函数返回值。

在用户程序使用 BSON 对象时，主要分为建立对象和读取对象两个操作。

##建立对象##

总的来说，一个 BSON 对象的创建主要分为三大步操作：

1. 创建对象（bson_create；bson_init）

2. 使用对象

3. 清除对象（bson_dispose，与 bson_create 配对使用；bson_destroy，与 bson_init 配对使用）

* 创建一个简单的 BSON 对象 {age:20}

  ```lang-c
  bson obj;
  bson_init( &obj );
  bson_append_int( &obj, "age", 20 );
  if ( BSON_OK != bson_finish( &obj ) )
  {
      printf( "Error." ) ;
  }
  else
  {
      bson_print( &obj );
  }
  // never use "bson_dispose" here,
  // for "bson_destroy" is used with
  // "bson_init"
  bson_destroy( &obj );
  ```

* 创建一个复杂的 BSON 对象

  ```lang-c
  /* 创建一个包含{name:"tom",colors:["red","blue","green"], address: {city:"Toronto", province: "Ontario"}}的对象 */
  bson *newobj = bson_create ();
  bson_append_string ( newobj, "name", "tom" );
  bson_append_start_array( newobj, "colors" );
  bson_append_string( newobj, "0", "red" );
  bson_append_string( newobj, "1", "blue" );
  bson_append_string( newobj, "2", "green" );
  bson_append_finish_array( newobj );
  bson_append_start_object ( newobj, "address" );
  bson_append_string ( newobj, "city", "Toronto" );
  bson_append_string ( newobj, "province", "Ontario" );
  bson_append_finish_object ( newobj );
  if( BSON_OK != bson_finish ( newobj ) ) 
  {
      printf( "Error." );
  }
  else
  {
      bson_print( newobj );
  }
  // never use "bson_destroy" here,
  // for "bson_dispose" is used with
  // "bson_create"
  bson_dispose( newobj );
  ```

##读取对象##

* 可以使用 bson_print 方法来打印 BSON 内容，也可以使用 bson_iterator 来遍历 BSON 的所有字段内容；要遍历 BSON，首先要初始化 bson_iterator，然后使用 bson_iterator_next 遍历 BSON 每一个元素

  ```lang-c
  bson newobj;
  bson_iterator i;
  bson_type type;
  const char *key = NULL;
  INT32 value     = 0;

  // build a bson
  bson_init( &newobj );
  bson_append_int( &newobj, "a", 1 );
  bson_finish( &newobj );

  // init bson iterator
  bson_iterator_init( &i, &newobj );

  // get type and value
  while( BSON_EOO != ( type = bson_iterator_next ( &i ) ) )
  {
      key = bson_iterator_key ( &i );
      if ( BSON_INT == type )
      {
          value = bson_iterator_int( &i );
          printf( "Type: %d, Key: %s, value: %d\n", type, key, value );
      }
  }

  // release resource
  bson_destroy( &newobj );
  ```

* 对于每个 bson_iterator，使用 bson_iterator_type 函数可以得到其类型，使用 bson_iterator_string 等函数可以得到其相对应类型的数值

  ```lang-c
  bson newobj;
  bson_iterator i;
  bson_type type;

  // build a bson
  bson_init( &newobj );
  bson_append_string( &newobj, "a", "hello" );
  bson_finish( &newobj );

  // init bson iterator
  bson_iterator_init( &i, &newobj );

  // get the type of the value
  type = bson_iterator_type( &i );

  // display the value
  if ( BSON_STRING == type )
  {
      printf( "Value: %s\n", bson_iterator_string( &i ) );
  }

  // release resource
  bson_destroy( &newobj );
  ```

* 遍历每个连续的 BSON 对象元素，可以使用 bson_find 函数直接跳转得到元素的名称；如果该元素不存在于 bson 之内，则 bson_find 函数返回 BSON_EOO

  例如想得到 name 元素名可以这样使用：

  ```lang-c
  bson newobj;
  bson_iterator i;
  bson_type type;

  // build a bson
  bson_init( &newobj );
  bson_append_string( &newobj, "Name", "Sam" );
  bson_finish( &newobj );


  type = bson_find ( &i, &newobj, "Name" );
  if ( BSON_EOO != type )
  {
      printf( "Name: %s\n", bson_iterator_string( &i ) );
  }

  // release resource
  bson_destroy( &newobj );
  ```

* 读取数组元素或嵌套对象，因为“address”是一个嵌套对象，需要特殊遍历；首先得到 address 值，再初始化一个新的 BSON 迭代器

  ```lang-c
  bson newobj;
  bson_iterator i;
  bson_iterator sub;
  bson_type type;
  const CHAR *key   = NULL;
  const CHAR *value = NULL;

  // build a bson
  bson_init( &newobj );
  bson_append_start_object( &newobj, "address" );
  bson_append_string( &newobj, "Home", "guangzhou" );
  bson_append_string( &newobj, "WorkPlace", "shenzhen" );
  bson_append_finish_object( &newobj );
  bson_finish( &newobj );

  // init bson iterator and display contents in the sub object
  type = bson_find( &i, &newobj, "address" );
  if ( BSON_EOO != type )
  {
      bson_iterator_subiterator( &i, &sub );
      while ( bson_iterator_more( &sub ) )
      {
          type = bson_iterator_next( &sub );
          key = bson_iterator_key( &sub );
          value = bson_iterator_string( &sub ) ;
          if ( BSON_STRING == type )
          {
              printf( "Type: %d, Key: %s, value: %s\n", type, key, value );
          }
      }
  }

  // release resource
  bson_destroy( &newobj );
  ```

  方法 bson_iterator_subiterator 初始化迭代器 sub，并且指向子对象的开始位置，从这里开始可以遍历 sub 中的所有元素，直到子对象的结束位置。
