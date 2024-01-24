/****************************************************
@description:   upsert shardingKey
                testlink cases: seqDB-12650
@modify list:
                2017-11-29 XiaoNi Huang init
****************************************************/
var csName = COMMCSNAME;
var clName = COMMCLNAME + "_12650";
var cl = "name=" + csName + '.' + clName;

// 目前版本不支持分区键字段修改，因此屏蔽
//main( test );

function test ()
{
   commDropCL( db, csName, clName, true, true, "drop cl in begin" );
   var opt = { ReplSize: 0, ShardingKey: { a: 1 } };
   var varCL = commCreateCL( db, csName, clName, opt, true, false, "create cl in begin" );
   varCL.insert( { a: 1, b: 1 } );
   upsert();
   commDropCL( db, csName, clName, false, true, "drop cl in clean in finally" );
}
function upsert ()
{
   tryCatch( ["cmd=upsert", cl, 'updator={$inc:{a:1, b:1}}', 'filter={a:1}', 'flag=SDB_QUERY_KEEP_SHARDINGKEY_IN_UPDATE'], [-178], "Error occurs in " + getFuncName() );

   var cursor = varCL.find();
   var expRecs = [{ a: 2, b: 2 }];
   commCompareResults( cursor, expRecs );

   tryCatch( ["cmd=upsert", cl, 'updator={$inc:{a:1, b:1}}', 'filter={a:1}', 'flag=SDB_QUERY_KEEP_SHARDINGKEY_IN_UPDATE'], [-178], "Error occurs in " + getFuncName() );

   cursor = varCL.find();
   expRecs = [{ a: 1, b: 1 }, { a: 2, b: 2 }];
   commCompareResults( cursor, expRecs );
}
