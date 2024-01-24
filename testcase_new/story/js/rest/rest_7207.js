/****************************************************
@description:   queryandupdate,normal case
                testlink cases: seqDB-7207
                queryandupdate(): use only the required parameters
@input:         1 insert{myid:229095,age:9}
                2 cmd=queryandupate name=cs.cl updator={$inc:{age:1}}	
@expectation:   cl has 1 record: {myid:229095,age:10}
@modify list:
                2015-5-25 Ting YU init   2016-3-16 XiaoNi Huang init
****************************************************/
var csName = COMMCSNAME;
var clName = COMMCLNAME + "_7207";
var cl = "name=" + csName + '.' + clName;
var varCL;

main( test );

function test ()
{
   ready();
   varCL.insert( { myid: 229095, age: 9 } );
   queryandupdate();
   commDropCL( db, csName, clName, false, true, "drop cl in clean in finally" );
}



function ready ()
{
   commDropCL( db, csName, clName, true, true, "drop cl in begin" );
   varCL = commCreateCL( db, csName, clName, {}, true, false, "create cl in begin" );
   var index = { age: 1 };
   commCreateIndex( varCL, "ageIndex", index );
}

function queryandupdate ()
{
   tryCatch( ["cmd=queryandupdate", cl, 'updator={$inc:{age:1}}'], [0], "Error occurs in " + getFuncName() );

   /******check count is 1**********/
   var recNum = varCL.find().count();
   if( 1 != recNum )
   {
      throw new Error( "recNum: " + recNum );
   }

   /******check record in cl**********/
   var recNum1 = varCL.find( { "myid": 229095, "age": 10 } ).count();
   if( 1 != recNum1 )
   {
      throw new Error( "recNum1: " + recNum1 );
   }

}
