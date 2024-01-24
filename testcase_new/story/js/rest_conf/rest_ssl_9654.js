/****************************************************
@description:	ssl, normal case
         testlink cases: seqDB-9654
@input:		connect sdb by ssl, then create cl
@expectation:	return errno:0
@modify list:
            	2017-11-21 XiaoNi Huang init
****************************************************/
var csName = COMMCSNAME;
var clName = COMMCLNAME + "_9654";
var cs = "name=" + csName;
var cl = "name=" + csName + '.' + clName;

main( test );

function test ()
{
   if( commIsStandalone( db ) )
   {
      return;
   }
   // 暂时增加打印信息，方便定位
   println( "coord ssl info: " );
   println( db.snapshot( 13, { "role": "coord" }, { "usessl": 1, "NodeName": 1 } ) );
   commDropCL( db, csName, clName, true, true, "drop cl in begin" );
   createclAndCheck();
   dropclAndCheck();
}

function createclAndCheck ()
{
   var word = "create collection";
   tryCatchByHttps( ["cmd=" + word, cl, "options={ShardingKey:{age:1}}"], [0], "Fail to run rest [cmd=" + word + "]" );
   db.getCS( csName ).getCL( clName );

   //check cl attribute
   var tep = db.snapshot( 8, { Name: csName + '.' + clName } ).current().toObj()["ShardingKey"];
   if( JSON.stringify( tep ) != '{"age":1}' )
   {
      throw new Error( 'options parameter Shardingkey, expect: {"age":1}, actual: ' + JSON.stringify( tep ) );
   }
}

function dropclAndCheck ()
{
   var word = "drop collection";
   tryCatchByHttps( ["cmd=" + word, cl], [0], "Fail to run rest [cmd=" + word + "]" );

   try
   {
      db.getCS( csName ).getCL( clName );
      throw new Error( "need throw error" );
   }
   catch( e )
   {
      if( e.message != SDB_DMS_NOTEXIST )
      {
         throw e;
      }
   }
}
