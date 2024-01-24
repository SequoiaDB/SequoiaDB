
本文档将介绍如何使用 CSharp 驱动接口编写使用 SequoiaDB 巨杉数据库的程序。下述为 SequoiaDB 巨杉数据库 CSharp 驱动的简单示例，详细的使用规范可参照官方的 [CSharp API](api/cs/html/index.html) 文档。

##命名空间##

在使用 CSharp 驱动的相关 API 之前，需要在源代码中添加如下 using 申明：

```lang-cs
using SequoiaDB;
using SequoiaDB.Bson;
```

##数据库操作##

* 连接数据库

   ```lang-cs
   string addr = "127.0.0.1:11810";
   Sequoiadb sdb = new Sequoiadb(addr);
   try
   {
         sdb.Connect();
   }
   catch (BaseException e)
   {
         Console.WriteLine("ErrorCode:{0}, ErrorType:{1}", e.ErrorCode, e.ErrorType);
         Console.WriteLine(e.Message);
   }
   catch (System.Exception e)
   {
         Console.WriteLine(e.StackTrace);
   }
   ```
  
   如果数据库创建了用户，则连接的时需要指定用户名和密码
  
   ```lang-cs
   string addr = "127.0.0.1:11810";
   Sequoiadb sdb = new Sequoiadb(addr);
   try
   {
         sdb.Connect("testusr", "testpwd");
   }
   catch (BaseException e)
   {
         Console.WriteLine("ErrorCode:{0}, ErrorType:{1}", e.ErrorCode, e.ErrorType);
         Console.WriteLine(e.Message);
   }
   catch (System.Exception e)
   {
         Console.WriteLine(e.StackTrace);
   }
   ```

   > **Note:**
   >
   > 示例中异常信息的 try 和 catch 块，下述的所有操作都会抛出同样的异常信息，因此不再给出相关的 try 和 catch 块。
   
* 断开与数据库连接

   ```lang-cs
   sdb.Disconnect();
   ```

* 获取或创建集合空间和集合

   根据集合空间名，获取对应的 CollectionSpace，如果指定集合空间不存在则创建

   ```lang-cs
   // get if the CollectionSpace exists, otherwise create
   string csName = "TestCS";
   CollectionSpace cs = null;
   if (sdb.IsCollectionSpaceExist(csName))
   {
         cs = sdb.GetCollectionSpace(csName);
   }
   else
   {
         cs = sdb.CreateCollectionSpace(csName);
         // or sdb.CreateCollectionSpace(csName, pageSize), need to specify the pageSize
   }
   ```
  
   根据集合名，获取对应的 Collection，如果指定集合不存在则创建
  
   ```lang-cs
   // get if the Collection exists, otherwise create
   string clName = "TestCL";
   DBCollection dbc = null;
   if (cs.IsCollectionExist(clName))
   {
         dbc = cs.GetCollection(clName);
   }
   else
   {
         dbc = cs.CreateCollection(clName);
         //or cs.CreateCollection(clName, options), create collection with some options
   }
   ```

* 插入数据

   插入一条数据

   ```lang-cs
   // 创建包含插入信息的 BsonDocument 对象
   BsonDocument insertor = new BsonDocument();
   string date = DateTime.Now.ToString();
   insertor.Add("operation", "Insert");
   insertor.Add("date", date);
   // 将此 BsonDocument 对象插入集合中
   dbc.Insert(insertor);
   ```

   BsonDocument 中既可以嵌套 BsonDocument 对象，也可以直接 new 一个完整的 BsonDocument，而不需要通过 Add 方法

   ```lang-cs
   BsonDocumentinsertor = new BsonDocument
   {
        {"FirstName","John"},
        {"LastName","Smith"},
        {"Age",50},
        {"id",i},
        {"Address",
            new BsonDocument
            {
               {"StreetAddress","212ndStreet"},
               {"City","NewYork"},
               {"State","NY"},
               {"PostalCode","10021"}
            }
        },
        {"PhoneNumber",
             new BsonDocument
             {
                {"Type","Home"},
                {"Number","212555-1234"}
           }
        }
   };
   ```
 
   插入多条数据
 
   ```lang-cs
   List< BsonDocument > insertor=new List < BsonDocument > ();
   for(int i=0;i<10;i++)
   {
        BsonDocument obj=new BsonDocument();
        obj.Add("operation","BulkInsert");
        obj.Add("date",DateTime.Now.ToString());
        insertor.Add(obj);
   }
   dbc.BulkInsert(insertor,0);
   ```

* 索引

   创建索引

   ```lang-cs
   // createindexkey,indexonattribute'Id'byASC(1)/DESC(-1)
   BsonDocument key = new BsonDocument();
   key.Add("id", 1);
   string name = "index name";
   bool isUnique = true;
   bool isEnforced = true;
   dbc.CreateIndex(name, key, isUnique, isEnforced);
   ```
 
   删除索引
 
   ```lang-cs
   string name = "index name";
   dbc.DropIndex(name);
   ```

* 查询操作

   ```lang-c#
   // 查询操作需要使用游标对查询结果进行遍历，可以先获取当前 Collection 的索引，如果不为空，则可用于制定访问计划（hint）
   DBCursor icursor = dbc.GetIndex(name);
   BsonDocument index = icursor.Current();
   // 构建相应的 BsonDocument 对象用于查询
   BsonDocument conditon = new BsonDocument();
   BsonDocument matcher = new BsonDocument();    // 查询匹配规则（matcher，包含相应的查询条件）
   conditon.Add("$gte", 0);
   conditon.Add("$lte", 9);
   matcher.Add("id", conditon);
   BsonDocument selector = new BsonDocument();    // 域选择（selector）
   selector.Add("id", null);
   selector.Add("Age", null);
   BsonDocument orderBy = new BsonDocument();    // 排序规则（orderBy，增序或降序）
   orderBy.Add("id", -1);
   BsonDocument hint = null;    // 制定访问计划（hint）
   if (index != null)
         hint = index;
   else
         hint = new BsonDocument();
   // 获取对应的 Cursor，跳过记录个数（0），返回记录个数（-1：返回所有数据）
   DBCursor cursor = dbc.Query(matcher, selector, orderBy, hint, 0, -1);
   // 使用 DBCursor 游标进行遍历
   while (cursor.Next() != null)
   Console.WriteLine(cursor.Current());
   ```

* 删除操作

   ```lang-cs
   // 创建包含删除条件的 BsonDocument 对象
   BsonDocument drop = new BsonDocument();
   drop.Add("Last Name", "Smith");
   // 执行删除
   coll.Delete(drop);
   ```

* 更新操作

   ```lang-cs
   // 创建 DBQuery 对象封装所有的查询或更新规则
   DBQuery query = new DBQuery();
   // 创建包含更新条件的 BsonDocument 对象
   BsonDocument updater = new BsonDocument();
   BsonDocument matcher = new BsonDocument();
   BsonDocument modifier = new BsonDocument();
   updater.Add("Age", 25);
   modifier.Add("$set", updater);
   matcher.Add("First Name", "John");
   query.Matcher = matcher;
   query.Modifier = modifier;
   // 更新记录
   dbc.Update(query);
   ```

  更新操作，如果没有满足 matcher 的条件，则插入如下记录：

  ```lang-cs
  dbc.Upsert(query);
  ```
