/****************************************************
@description:   update, normal case
                testlink cases: seqDB-7213
@input:         1 insert {_id:229095,name:"Jack",male:true},{_id:229096,name:"Harry",male:true}
                2 update lack of [filter], filter is an optional parameter.
@expectation:   1 cl.count: 2
                2 check Harry rec: add [age] field, not add other field
                3 check Jack rec: don't have change
@modify list:
                2015-4-3 Ting YU init   2016-3-16 XiaoNi Huang init
****************************************************/
var csName = COMMCSNAME;
var clName = COMMCLNAME + "_7213";


main( test );

function test ()
{
   commDropCL( db, csName, clName, true, true, "drop cl in begin" );
   var varCL = commCreateCL( db, csName, clName, {}, true, false, "create cl in begin" );
   varCL.insert( [
      { _id: 229095, name: "Jack", male: true },
      { _id: 229096, name: "Harry", male: true },
   ] );
   updateAndCheck( varCL );
   commDropCL( db, csName, clName, false, false, "drop cl in clean" );
}

function updateAndCheck ( varCL )
{  //The filter is an optional parameter.

   var word = "update";
   tryCatch( ["cmd=" + word, "name=" + csName + '.' + clName, 'updator={$set:{"age":16}}'], [0], "lackFilter: fail to run rest command: " + word );

   /******check count is 2**********/
   var size = varCL.count();
   if( 2 != size )
   {
      throw new Error( "size: " + size );
   }

   /**********check each record****************/
   var recsCnt1 = varCL.find( { _id: 229096, name: "Harry", male: true, age: 16 } ).count();
   var recsCnt2 = varCL.find( { _id: 229095, name: "Jack", male: true, age: 16 } ).count();
   var expCnt = 1;
   if( expCnt != recsCnt1 || expCnt != recsCnt2 )
   {
      throw new Error( "expCnt: " + expCnt + "\nrecsCnt1: " + recsCnt1 + "\nrecsCnt2: " + recsCnt2 );
   }
}




