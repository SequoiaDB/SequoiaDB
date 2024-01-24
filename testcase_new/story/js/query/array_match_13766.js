/*******************************************************************************
*@Description : seqDB-13766:查询匹配字段值为数组类型（走索引和不走索引）
*@Modify List : 2014-02-03   Pusheng Ding   Init
                2016-03-17   Ting YU        Modify
*******************************************************************************/
testConf.clName = COMMCLNAME + "_query_cl_13766";
main( test );

function test ( testPara )
{
   //insert records included array and other
   var recs = [{ a: [1, 2, 3], b: 1 }, { a: [], b: 2 }, { a: [1, 3, 2], b: 3 },
   { a: 1, b: 4 }, { a: { a1: 1, a2: 2 }, b: 5 }];
   testPara.testCL.insert( recs );

   //query without index
   queryAndCheckResult( testPara.testCL, recs );

   //query with index
   var idxName = 'a';
   testPara.testCL.createIndex( idxName, { a: 1 } );
   queryAndCheckResult( testPara.testCL, recs );
}
function queryAndCheckResult ( cl, recs )
{
   var rc = cl.find( { a: [1, 2, 3] } );
   var expRecs = [recs[0]];
   checkRec( rc, expRecs );

   var rc = cl.find( { a: [] } );
   var expRecs = [recs[1]];
   checkRec( rc, expRecs );

   var rc = cl.find( { a: 1 } ).sort( { b: 1 } );
   var expRecs = [recs[0], recs[2], recs[3]];
   checkRec( rc, expRecs );

   var rc = cl.find( { a: { a1: 1, a2: 2 } } );
   var expRecs = [recs[4]];
   checkRec( rc, expRecs );
}
