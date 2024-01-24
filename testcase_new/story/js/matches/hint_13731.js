/*******************************************************************************
*@Description : hint测试：强制走表扫描、hint走不存在的索引
                          强制走原本要走的索引、强制走另外一个索引
*@Modify List : 2014-06-12   xiaojun Hu   Init
                2016-03-17   Ting YU      Modify
                2020-08-20   Zixian Yan   Modify
*******************************************************************************/
testConf.clName = COMMCLNAME + "_13731";
main( test );

function test ( testPara )
{
   var cl = testPara.testCL;
   var idxName1 = "cl_a_idx";
   var idxName2 = "cl_b_idx";

   cl.createIndex( idxName1, { a: -1 } );
   cl.createIndex( idxName2, { b: 1 } );

   var recs = [];
   for( var i = 0; i < 100; i++ ) recs.push( { a: i, b: i + 0.95 } );
   cl.insert( recs );

   //  "---begin to query by hint({'':null})" 
   var rc = cl.find( { a: { $gte: 0 } } ).sort( { a: 1 } ).hint( { "": null } );
   checkExplain( rc, "" );
   checkRec( rc, recs );

   // "---begin to query, hinted by non-existed index"
   var rc = cl.find().sort( { b: 1 } ).hint( { "": "non_existed_index" } );
   checkExplain( rc, idxName2 );
   checkRec( rc, recs );

   //  "---begin to query, hinted by index"
   var rc = cl.find().sort( { a: 1 } ).hint( { "": idxName1 } );
   checkExplain( rc, idxName1 );
   checkRec( rc, recs );

   // "---begin to query, hinted by another index"
   var rc = cl.find().sort( { b: 1 } ).hint( { "": idxName1 } );
   checkExplain( rc, idxName1 );
   checkRec( rc, recs );

}

function checkExplain ( rc, expIdxName )
{
   var plan = rc.explain().current().toObj();
   var expScanType = "ixscan";
   if( expIdxName == "" ) { expScanType = "tbscan"; }
   assert.equal( plan.ScanType, expScanType );
   assert.equal( plan.IndexName, expIdxName );
}
