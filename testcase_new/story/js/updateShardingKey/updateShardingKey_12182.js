/************************************
 *@Description: 测试用例 seqDB-12182 :: 版本: 1 :: 子表指定多个分区键字段更新 
 *@author:      LaoJingTang
 *@createdate:  2017.8.22
 **************************************/

var csName = CHANGEDPREFIX + "_cs_12182";
var mainCLName = CHANGEDPREFIX + "_mcl_12182";
var subCLName1 = "subcl1";
var subCLName2 = "subcl2";
main( test );

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

   var sk1 = { a: 1 };
   var sk2 = { b: 1, c: 1 }
   var mainCL = createMainCL( csName, mainCLName, sk1 );
   createCL( csName, subCLName1, sk2 );
   createCL( csName, subCLName2, sk2 );

   mainCL.attachCL( csName + "." + subCLName1, { LowBound: { "a": 0 }, UpBound: { "a": 100 } } );
   mainCL.attachCL( csName + "." + subCLName2, { LowBound: { "a": 100 }, UpBound: { "a": 1000 } } );

   //insert data 	
   var docs = [{ a: 1, b: 1, c: 1 }];
   mainCL.insert( docs );

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
      tempJson.b = updatejson[x];
      tempJson.c = updatejson[x];
      mainCL.update( { $set: tempJson }, {}, {}, { KeepShardingKey: true } );
      tempJson.a = 1;
      checkResult( mainCL, null, { "_id": { "$include": 0 } }, [tempJson] );
   }

   //drop collectionspace in clean
   commDropCS( db, csName, false, "Failed to drop CS." );
}

