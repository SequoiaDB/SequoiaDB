/************************************
 *@Description: 测试用例 seqDB-12172 :: 版本: 1 :: 普通表开启flag执行find.update更新 
 *@author:      LaoJingTang
 *@createdate:  2017.8.22
 **************************************/

var csName = CHANGEDPREFIX + "_cs_12172";
var clName = CHANGEDPREFIX + "_cl_12172";
main( test );

function test ()
{

   commDropCS( db, csName, true, "Failed to drop CS." );
   commCreateCS( db, csName, false, "Failed to create CS." );
   var mycl = db.getCS( csName ).createCL( clName );
   //insert data 	
   var docs = [{ a: 1 }];
   mycl.insert( docs );

   //updateData
   mycl.find().update( { $set: { a: "test" } }, true, { KeepShardingKey: true } ).toString();
   //check the update result
   var expRecs = [{ "a": "test" }];
   checkResult( mycl, null, { "_id": { "$include": 0 } }, expRecs );

   //drop collectionspace in clean
   commDropCS( db, csName, false, "Failed to drop CS." );
}


