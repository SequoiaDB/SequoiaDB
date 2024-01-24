/****************************************************
@description:   queryandupdate shardingKey
                testlink cases: seqDB-12648
@modify list:
                2017-11-29 XiaoNi Huang init
****************************************************/
var csName = COMMCSNAME;
var clName = COMMCLNAME + "_12649";
var cl = "name=" + csName + '.' + clName;

// 目前版本不支持分区键字段修改，因此屏蔽
//main( test );

function test ()
{
   commDropCL( db, csName, clName, true, true, "drop cl in begin" );
   var opt = { ReplSize: 0, ShardingKey: { a: 1 } };
   var varCL = commCreateCL( db, csName, clName, opt, true, false, "create cl in begin" );
   varCL.insert( { a: 1, b: 1 } );
   queryandupdate();
   commDropCL( db, csName, clName, false, true, "drop cl in clean in finally" );
}

function queryandupdate ()
{
   tryCatch(
      ["cmd=queryandupdate", cl, 'updator={$inc:{a:1, b:1}}', 'selector={a:"",b:""}', 'returnnew=true', 'flag=SDB_QUERY_KEEP_SHARDINGKEY_IN_UPDATE'], [-178], "Error occurs in " + getFuncName() );

   /******check rest return**********/
   var rtnExp = '{ "errno": 0 }{ "a": 2, "b": 2 }';
   if( info != rtnExp )
   {
      throw new Error( "info: " + info + "\nrtnExp: " + rtnExp );
   }

   /******check count is 1**********/
   var recNum = varCL.find( { a: 2, b: 2 } ).count();
   if( 1 != recNum )
   {
      throw new Error( "recNum: " + recNum );
   }

}
