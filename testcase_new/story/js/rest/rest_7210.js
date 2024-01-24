/****************************************************
@description:	queryandremove,normal case
         testlink cases: seqDB-7210
         queryandremove(): use only the required parameters
@input:		1 insert{myid:229095,age:10}
				2 cmd=queryandremove&name=cs.cl
@expectation:	cl has no record
@modify list:
            	2015-5-25 Ting YU init   2016-3-16 XiaoNi Huang init
****************************************************/
var csName = COMMCSNAME;
var clName = COMMCLNAME + "_7210";
var cl = "name=" + csName + '.' + clName;
var varCL;

main( test );

function test ()
{
   ready();
   varCL.insert( { _id: 229095, age: 10 } );
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
   tryCatch( ["cmd=queryandremove", cl], [0], "Error occurs in " + getFuncName() );

   /******check rest return**********/
   var rtnExp = '{ "errno": 0 }{ "_id": 229095, "age": 10 }';
   if( info != rtnExp )
   {
      throw new Error( "info: " + info + "\nrtnExp: " + rtnExp );
   }

   /******check count is 0**********/
   var recNum = varCL.find().count();
   if( 0 != recNum )
   {
      throw new Error( "recNum: " + recNum );
   }
}



