/****************************************************
*@description: seqDB-11201:query.hint.count查询
*@author:
*              2017-3-7 huangxiaoni init
*              2020-10-14 liuli
****************************************************/
testConf.clName = COMMCLNAME + "_11201";

main( test );

function test ( args )
{
   var cl = args.testCL;
   var idxName = "idx";

   cl.createIndex( idxName, { a: -1 } );
   insertRecs( cl );
   queryRecs( cl, idxName );  //query and check result
}

function insertRecs ( cl )
{
   for( i = 0; i < 50; i++ )
   {
      cl.insert( { a: i, b: i } );
   }
}

function queryRecs ( cl, idxName )
{
   var cnt1 = cl.find( { b: { $gte: 10 } } ).hint( { "": "" } ).sort( { a: 1 } ).skip( 10 ).limit( 20 ).count();
   var cnt2 = cl.find( { b: { $gte: 10 } } ).hint( { "": idxName } ).sort( { a: 1 } ).skip( 10 ).limit( 20 ).count();

   //check result
   var expCnt = 40;
   var actCnt1 = Number( cnt1 );
   var actCnt2 = Number( cnt2 );
   assert.equal( expCnt, actCnt1 );
   assert.equal( actCnt1, actCnt2 );
}