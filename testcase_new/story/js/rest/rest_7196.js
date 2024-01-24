/****************************************************
@description:   alter, normal case
                testlink cases: seqDB-7196
@input:         create a normal cl, alter ShardingKey to {age:1}
@expectation:   check ShardingKey is {age:1} by db.snapshot(8)
@modify list:
                2015-4-3 Ting YU init   2016-3-16 XiaoNi Huang init
****************************************************/
var csName = COMMCSNAME;
var clName = COMMCLNAME + "_7196";

main( test );

function test ()
{
   if( commIsStandalone( db ) )
   {
      return;
   }
   commDropCL( db, csName, clName, true, true, "drop cl in begin" );
   var varCL = commCreateCL( db, csName, clName, {}, true, false, "create cl in begin" );
   alterAndCheck();
   commDropCL( db, csName, clName, false, false, "drop cl in clean" );
}


function alterAndCheck ()
{
   var word = "alter collection";
   tryCatch( ["cmd=" + word, "name=" + csName + '.' + clName, 'options={ShardingKey:{age:1}}'], [0], "alterAndCheck: fail to run rest command: " + word );

   var tep = db.snapshot( 8, { Name: csName + '.' + clName } ).current().toObj()["ShardingKey"];

   if( JSON.stringify( tep ) != '{"age":1}' )
   {
      throw new Error( 'Error: options parameter Shardingkey, expect: {"age":1}, actual: ' + JSON.stringify( tep ) );
   }
}


