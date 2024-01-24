/****************************************************
@description:   create collectionspace, normal case
                testlink cases: seqDB-7190 & 7191 & 7194
@input:         run CURL command: 
                1 curl http://127.0.0.1:11814/ -d "create collection&name=local_test_cs.local_test_cl"
                2 createCL, lack [options], the options is an optional parameter.
                3 curl http://127.0.0.1:11814/ -d "drop collection&name=local_test_cs.local_test_cl"
@expectation:   return errno:0
@modify list:
                2015-4-3 Ting YU init   2016-3-16 XiaoNi Huang init
****************************************************/
var csName = COMMCSNAME;
var clName = COMMCLNAME + "_7190_7191_7194_1";
var clName2 = COMMCLNAME + "_7190_7191_7194_2";
var cs = "name=" + csName;
var cl = "name=" + csName + '.' + clName;
var cl2 = "name=" + csName + '.' + clName2;

main( test );

function test ()
{
   if( commIsStandalone( db ) )
   {
      return;
   }
   commDropCL( db, csName, clName, true, true, "drop cl in begin" );
   commDropCL( db, csName, clName2, true, true, "drop cl in begin" );
   createclAndCheck();
   createclLackOptions();
   dropclAndCheck();
   commDropCL( db, csName, clName2, false, false, "drop cl in clean" );
}

function createclAndCheck ()
{
   var word = "create collection";
   tryCatch( ["cmd=" + word, cl, "options={ShardingKey:{age:1}}"], [0], "Fail to run rest [cmd=" + word + "]" );
   db.getCS( csName ).getCL( clName );

   //check cl attribute
   var tep = db.snapshot( 8, { Name: csName + '.' + clName } ).current().toObj()["ShardingKey"];
   if( JSON.stringify( tep ) != '{"age":1}' )
   {
      throw new Error( 'Error: options parameter Shardingkey, expect: {"age":1}, actual: ' + JSON.stringify( tep ) );
   }
}

function createclLackOptions ()
{  //The options is an optional parameter.

   var word = "create collection";
   tryCatch( ["cmd=" + word, cl2], [0], "Fail to run rest [cmd=" + word + "]" );
   db.getCS( csName ).getCL( clName2 );
}

function dropclAndCheck ()
{
   var word = "drop collection";
   tryCatch( ["cmd=" + word, cl], [0], "Fail to run rest [cmd=" + word + "]" );

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




