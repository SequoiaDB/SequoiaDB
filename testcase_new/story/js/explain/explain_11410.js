/******************************************************************************
*@Description : seqDB-11410:find().remove().sort()与hint()组合下的索引选择 
*@Author      : Chen xiaodan
*@Date        : 2021-6-11
******************************************************************************/
testConf.clName = CHANGEDPREFIX + "_11410";

main( test );

function test ( args )
{
   var cl = args.testCL;
   var idexName1 = "index_a";
   var idexName2 = "index_b";
   var idexName3 = "index_ab";

   cl.createIndex( idexName1, { a: 1 } );
   cl.createIndex( idexName2, { b: 1 } );
   cl.createIndex( idexName3, { a: 1, b: 1 } );

   cl.insert( [{ a: 1, b: 0 }, { a: 2, b: 1 }, { a: 3, b: 2 }, { a: 4, b: 3 }] );

   //单键索引 
   var explain = cl.find( { a: 1 } ).sort( { a: 1 } ).hint( { "": "" } ).remove().explain();
   assert.equal( JSON.parse( explain[0] ).IndexName, idexName1 );
   assert.equal( JSON.parse( explain[0] ).ScanType, "ixscan" );

   var explain = cl.find( { a: { $gt: 2 } }, { b: 1 } ).sort( { b: 1 } ).hint( { "": "" } ).remove().explain();
   assert.equal( JSON.parse( explain[0] ).IndexName, idexName2 );
   assert.equal( JSON.parse( explain[0] ).ScanType, "ixscan" );

   //组合索引
   var explain = cl.find( { a: 1, b: { $gt: 2 } } ).sort( { a: 1 } ).hint( { "": "" } ).remove().explain();
   assert.equal( JSON.parse( explain[0] ).IndexName, idexName3 );
   assert.equal( JSON.parse( explain[0] ).ScanType, "ixscan" );

   //排序索引与hint索引一致
   var explain = cl.find( { a: 1 } ).sort( { a: 1, b: 1 } ).hint( { "": idexName3 } ).remove().explain();
   assert.equal( JSON.parse( explain[0] ).IndexName, idexName3 );
   assert.equal( JSON.parse( explain[0] ).ScanType, "ixscan" );

   //排序索引与hint索引不一致
   assert.tryThrow( SDB_RTN_QUERYMODIFY_SORT_NO_IDX, function()
   {
      cl.find( { a: 1 } ).sort( { a: 1, b: 1 } ).hint( { "": idexName1 } ).remove().explain();
   } );

   //表扫描
   var explain = cl.find( { a: { $gt: 2 } }, { b: 1 } ).hint( { "": null } ).remove().explain();
   assert.equal( JSON.parse( explain[0] ).IndexName, "" );
   assert.equal( JSON.parse( explain[0] ).ScanType, "tbscan" );

}
