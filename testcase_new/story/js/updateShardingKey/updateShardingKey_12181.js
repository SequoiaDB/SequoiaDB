/************************************
 *@Description: 测试用例 seqDB-12181 :: 版本: 1 :: 分区表指定多个分区键字段更新
 1、创建cs、分区表cl，指定分区键包含多个字段，如{a:1,b:1:c:-1,d:1}
 2、插入数据，其中分区键字段覆盖多个数据类型（如数组、int、timestamp、long、float、date、decimal）
 3、执行update更新分区键，设置flag为true
 4、检查更新结果
 1、更新返回成功，查看数据更新值正确
 *@author:      LaoJingTang
 *@createdate:  2017.8.22
 **************************************/

var csName = CHANGEDPREFIX + "_cs_12181";
var clName = CHANGEDPREFIX + "_cl_12181";
//main( test );

function test ()
{
   if( true == commIsStandalone( db ) )
   {
      return;
   }

   var allGroupName = getGroupName( db, true );
   if( 1 === allGroupName.length )
   {
      return;
   }

   commDropCS( db, csName, true, "Failed to drop CS." );
   commCreateCS( db, csName, false, "Failed to create CS." );
   var mycl = createCL( csName, clName, { a: 1, b: 1, c: 1 } );

   //insert data
   var docs = [{ a: 1, b: 1, c: 1 }];
   mycl.insert( docs );

   var updatejson = [
      { "a": 123 },
      { "a": 3000000000 },
      { "a": 123.456 },
      { "a": 123.456 },
      { "a": "value" },
      { "a": "1234" },
      { "a": "1234.123" },
      { "a": { "$oid": "123abcd00ef12358902300ef" } },
      { "a": true },
      { "a": { "$date": "2012-01-01" } },
      { "a": { "$timestamp": "2012-01-01-13.14.26.124233" } },
      { "a": { "$binary": "aGVsbG8gd29ybGQ=", "$type": "1" } },
      { "a": { "$regex": "^张", "$options": "i" } },
      { "a": { "subobj": "value" } },
      { "a": ["abc", 0, "def"] },
      { "a": null },
      { "a": { "$minKey": 1 } },
      { "a": { "$maxKey": 1 } }
   ];

   for( x in updatejson )
   {
      var tempJson = new Object();
      tempJson.a = updatejson[x];
      tempJson.b = updatejson[x];
      tempJson.c = updatejson[x];
      mycl.update( { $set: tempJson }, {}, {}, { KeepShardingKey: true } );
      checkResult( mycl, null, { "_id": { "$include": 0 } }, [tempJson] );
   }


   //drop collectionspace in clean
   commDropCS( db, csName, false, "Failed to drop CS." );
}
