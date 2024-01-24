/****************************************************
@description:   queryandremove,normal case
                testlink cases: seqDB-7209
                queryandremove(): cover all parameters
@input:         1 insertmyid:229095,age:10},{myid:229098,age:13},{myid:229096,age:11,male:true},{age:12},{myid:229099,age:14}
                2 cmd=queryandremove&name=cs.cl&sort={age:1}&selector={myid:"",age:""}&skip=1&returnnum=2&filter={myid:{$exists:1}}
@expectation:   delete 229096 229098 records
@modify list:
                2015-5-25 Ting YU init   2016-3-16 XiaoNi Huang init
****************************************************/
var csName = COMMCSNAME;
var clName = COMMCLNAME + "_7209";
var cl = "name=" + csName + '.' + clName;
var varCL;


main( test );

function test ()
{
   ready();
   varCL.insert( [
      { myid: 229095, age: 10 },
      { myid: 229098, age: 13 },
      { myid: 229096, age: 11, male: true },
      { age: 12 },
      { myid: 229099, age: 14 }
   ] );
   queryandremove();
   commDropCL( db, csName, clName, false, true, "drop cl in clean in finally" );
}


function ready ()
{
   commDropCL( db, csName, clName, true, true, "drop cl in begin" );
   varCL = commCreateCL( db, csName, clName, {}, true, false, "create cl in begin" );
   var index = { age: 1 };
   commCreateIndex( varCL, "ageIndex", index );
}

function queryandremove ()
{
   tryCatch( ["cmd=queryandremove", cl, 'sort={age:1}', 'selector={myid:"",age:""}', 'skip=1', 'returnnum=2', 'filter={myid:{$exists:1}}'], [0], "Error occurs in " + getFuncName() );

   /******check rest return**********/
   var rtnExp = '{ "errno": 0 }{ "myid": 229096, "age": 11 }{ "myid": 229098, "age": 13 }';
   if( info != rtnExp )
   {
      throw new Error( "info: " + info + "\nrtnExp: " + rtnExp );
   }

   /******check count is 3**********/
   var recNum = varCL.find().count();
   if( 3 != recNum )
   {
      throw new Error( "recNum: " + recNum );
   }

   /******check record in cl**********/
   var recNum1 = varCL.find( { "myid": 229095, "age": 10 } ).count();
   var recNum2 = varCL.find( { "age": 12 } ).count();
   var recNum3 = varCL.find( { "myid": 229099, "age": 14 } ).count();
   if( 1 != recNum1 )
   {
      throw new Error( "recNum1: " + recNum1 );
   }
   if( 1 != recNum2 )
   {
      throw new Error( "recNum2: " + recNum2 );
   }
   if( 1 != recNum3 )
   {
      throw new Error( "recNum3: " + recNum3 );
   }
}


