/************************************************************************
*@Description:  seqDB-5570:主子表上使用访问计划，查询条件为空/仅为索引字段/不仅有索引字段_ST.explainAdd.02
*@Author:  2016/7/11  huangxiaoni
************************************************************************/
testConf.skipStandAlone = true;
testConf.skipOneGroup = true;

testConf.clName = COMMCLNAME + "_mcl_5570";
testConf.clOpt = { IsMainCL: true, ShardingKey: { a: 1 }, ShardingType: "range", Compressed: true };
main( test );

function test ( testPara )
{
   var subCLName = COMMCLNAME + "_scl_5570";
   var idxName = CHANGEDPREFIX + "_idx";

   commDropCL( db, COMMCSNAME, subCLName, true, true, "Failed to drop CL in the begin." );
   commCreateCL( db, COMMCSNAME, subCLName, { ShardingKey: { a: 1 }, ShardingType: "hash", Compressed: true } );

   attachCLAndInsertRecs( testPara.testCL, subCLName );
   testPara.testCL.createIndex( idxName, { b: 1 } );
   var rc = explain( testPara.testCL );
   checkResult( rc );
}

function attachCLAndInsertRecs ( dbcl, subCLName )
{
   var options = { LowBound: { "a": { $minKey: 1 } }, UpBound: { "a": { $maxKey: 1 } } };
   dbcl.attachCL( COMMCSNAME + "." + subCLName, options );

   dbcl.insert( { a: 1, b: 1, c: 1 } );
   dbcl.insert( { a: 2, b: 2, c: 2 } );
}

function explain ( cl )
{

   var rc = [];
   var rc0 = cl.find().explain( { Run: true } ).current().toObj();
   var rc1 = cl.find( { a: 2 } ).explain( { Run: true } ).current().toObj();
   var rc2 = cl.find( { "$and": [{ "a": { "$gte": 1 } }, { b: 2 }] } ).explain( { Run: true } ).current().toObj();
   rc.push( rc0 );
   rc.push( rc1 );
   rc.push( rc2 );

   return rc;
}

function checkResult ( rc )
{

   //compare the returned records for rc[0]
   var Query = JSON.stringify( rc[0]["SubCollections"][0]["Query"]["$and"] );
   var IXBound = rc[0]["SubCollections"][0]["IXBound"];
   var NeedMatch = rc[0]["SubCollections"][0]["NeedMatch"];
   var expQuery = '[]';
   var expIXBound = null;
   var expNeedMatch = false;
   if( Query !== expQuery || IXBound !== expIXBound
      || NeedMatch !== expNeedMatch )
   {
      throw new Error( "checkResult", null, "[ rc0 ]" + "[Query:" + expQuery + ",IXBound:" + expIXBound + ",NeedMatch:" + expNeedMatch + "]" + "[Query:" + Query + ",IXBound:" + IXBound + ",NeedMatch:" + NeedMatch + "]" );
   }

   //compare the returned records for rc[1]
   var Query = JSON.stringify( rc[1]["SubCollections"][0]["Query"]["$and"] );
   var IXBound = JSON.stringify( rc[1]["SubCollections"][0]["IXBound"] );
   var NeedMatch = rc[1]["SubCollections"][0]["NeedMatch"];

   var expQuery = '[{"a":{"$et":2}}]';
   var expIXBound = '{"a":[[2,2]]}';
   var expNeedMatch = false;
   if( Query !== expQuery || IXBound !== expIXBound
      || NeedMatch !== expNeedMatch )
   {
      throw new Error( "checkResult", null, "[ rc1 ]" + "[Query:" + expQuery + ",IXBound:" + expIXBound + ",NeedMatch:" + expNeedMatch + "]" + "[Query:" + Query + ",IXBound:" + IXBound + ",NeedMatch:" + NeedMatch + "]" );
   }

   //compare the returned records for rc[2]
   var Query = JSON.stringify( rc[2]["SubCollections"][0]["Query"]["$and"] );
   var IXBound = JSON.stringify( rc[2]["SubCollections"][0]["IXBound"] );
   var NeedMatch = rc[2]["SubCollections"][0]["NeedMatch"];

   //var expQuery     = '[{"$and":[{"b":{"$et":2}},{"a":{"$gte":1}}]}]';
   var expQuery = '[{"b":{"$et":2}},{"a":{"$gte":1}}]';
   var expIXBound = '{"b":[[2,2]]}';
   var expNeedMatch = true;
   if( Query !== expQuery || IXBound !== expIXBound
      || NeedMatch !== expNeedMatch )
   {
      throw new Error( "checkResult", null, "[ rc2 ]" + "[Query:" + expQuery + ",IXBound:" + expIXBound + ",NeedMatch:" + expNeedMatch + "]" + "[Query:" + Query + ",IXBound:" + IXBound + ",NeedMatch:" + NeedMatch + "]" );
   }

}
