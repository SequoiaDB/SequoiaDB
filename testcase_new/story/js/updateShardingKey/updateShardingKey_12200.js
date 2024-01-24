/************************************
 *@Description:  测试用例 seqDB-12200 :: 版本: 1 :: 开启flag在内置SQL执行更新分区键 
 *@author:      LaoJingTang
 *@createdate:  2017.8.22
 **************************************/

var csName = CHANGEDPREFIX + "_cs_12200";
var clName = CHANGEDPREFIX + "_cl_12200";
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
   var mycl = db.getCS( csName ).createCL( clName, { ShardingKey: { a: 1 } } );

   //insert data 	
   var docs = [{ a: 1 }];
   mycl.insert( docs );

   db.execUpdate( "update " + csName + "." + clName + " set a = 10 where a < 25 /*+use_flag(SQL_UPDATE_KEEP_SHARDINGKEY)*/" );

   //check the update result
   var expRecs = [{ a: 10 }]
   checkResult( mycl, null, { "_id": { "$include": 0 } }, expRecs );

   //drop collectionspace in clean
   commDropCS( db, csName, false, "Failed to drop CS." );
}


