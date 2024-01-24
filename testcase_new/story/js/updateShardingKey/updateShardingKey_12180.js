/************************************
 *@Description: 测试用例 seqDB-12180 :: 版本: 1 :: 更新分区键数据类型为sequoiadb支持所有类型
 1、创建cs、分区表cl，指定shardingKey
 2、插入数据，其中分区键字段数据类型覆盖sequoiadb支持所有类型，覆盖中文
 3、执行update更新分区键，设置flag为true，其中随机覆盖如下情况：
 a、更新为相同数据类型
 b、更新为不同数据类型，如obj类型更新为int类型
 4、检查更新结果
 1、更新返回成功，查看数据更新值正确
 *@author:      LaoJingTang
 *@createdate:  2017.8.22
 **************************************/

var csName = CHANGEDPREFIX + "_cs_12180";
var clName = CHANGEDPREFIX + "_cl_12180";
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
   var mycl = createCL( csName, clName, { a: 1 } );

   //insert data
   var docs = [{ a: 1 }];
   mycl.insert( docs );

   var updatejson = [{ "a": 123 },
   { "a": 3000000000 },
   { "a": 123.456 },
   { "a": 123.456 },
   { "a": "value" },
   { "a": "1234" },
   { "a": "1234.123" },
   { "a": { "$oid": "123abcd00ef12358902300ef" } },
   { "a": true },
   { "a": { "$date": "2012-01-01" } },
   { "a": { "$timestamp": "2012-01-01-13.14.26.124233" } }, {
      "a": {
         "$binary": "aGVsbG8gd29ybGQ=",
         "$type": "1"
      }
   }, { "a": { "$regex": "^张", "$options": "i" } }, { "a": { "subobj": "value" } },
   { "a": ["abc", 0, "def"] },
   { "a": null },
   { "a": { "$minKey": 1 } },
   { "a": { "$maxKey": 1 } }];

   for( x in updatejson )
   {
      mycl.update( { $set: updatejson[x] }, {}, {}, { KeepShardingKey: true } );
      checkResult( mycl, null, { "_id": { "$include": 0 } }, [updatejson[x]] );
   }


   //drop collectionspace in clean
   commDropCS( db, csName, false, "Failed to drop CS." );
}
