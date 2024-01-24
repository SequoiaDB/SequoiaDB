/****************************************************
@description:   queryandupdate, abnormal case
                testlink cases: seqDB-7208
@input:         1 insert{myid:229095,age:10}
                2 rest command lack of [name]
                3 rest command lack of [updator]
@expectation:   1 return SDB_INVALIDARG error
@modify list:
                2015-5-25 Ting YU init   2016-3-16 XiaoNi Huang init
****************************************************/
var csName = COMMCSNAME;
var clName = COMMCLNAME + "_7208";
var cl = "name=" + csName + '.' + clName;
var varCL;


main( test );

function test ()
{
   ready();
   varCL.insert( { myid: 229095, age: 10 } );
   lackName();
   lackUpdator();
   commDropCL( db, csName, clName, false, true, "drop cl in clean in finally" );
}


function ready ()
{
   commDropCL( db, csName, clName, true, true, "drop cl in begin" );
   varCL = commCreateCL( db, csName, clName, {}, true, false, "create cl in begin" );
   var index = { age: 1 };
   commCreateIndex( varCL, "ageIndex", index );
}

function lackName ()
{
   tryCatch( ["cmd=queryandupdate", 'updator={$inc:{age:1}}'], [-6], "Error occurs in " + getFuncName() );

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

function lackUpdator ()
{
   tryCatch( ["cmd=queryandupdate", cl], [SDB_INVALIDARG], "Error occurs in " + getFuncName() );

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



