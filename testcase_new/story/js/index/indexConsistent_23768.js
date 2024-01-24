/******************************************************************************
 * @Description   : seqDB-23768 :: 主表上没有索引，复制索引  
 * @Author        : wu yan
 * @CreateTime    : 2021.04.02
 * @LastEditTime  : 2021.12.17
 * @LastEditors   : Wu Yan
 ******************************************************************************/
testConf.skipStandAlone = true;

main( test );
function test ( testPara )
{
   var mainCLName = COMMCLNAME + "_maincl_index23768";
   var subCLName = COMMCLNAME + "_subcl_index23768";

   commDropCL( db, COMMCSNAME, mainCLName, true, true, "Fail to drop CL in the beginning" );
   var maincl = commCreateCL( db, COMMCSNAME, mainCLName, { ShardingKey: { a: 1 }, ShardingType: "range", IsMainCL: true } );
   commCreateCL( db, COMMCSNAME, subCLName, { ShardingKey: { a: 1 }, ShardingType: "hash" } );
   maincl.attachCL( COMMCSNAME + "." + subCLName, { LowBound: { a: 0 }, UpBound: { a: 10000 } } );

   assert.tryThrow( SDB_MAINCL_NOIDX_NOSUB, function()
   {
      maincl.copyIndex();
   } );
   checkNoTask( COMMCSNAME, mainCLName, "Copy index" );

   commDropCL( db, COMMCSNAME, mainCLName, true, true, "Fail to drop CL in the ending" );
}
