/******************************************************************************
 * @Description   : seqDB-26326:forceHint查询计划缓存
 * @Author        : ZhangYanan
 * @CreateTime    : 2022.04.06
 * @LastEditTime  : 2022.04.08
 * @LastEditors   : Zhang Yanan
 ******************************************************************************/
main( test );

function test ( args )
{
   var clName = "cl_26326";
   var match = { age: { $gt: 10 } };

   commDropCL( db, COMMCSNAME, clName, true, true );
   var cl = commCreateCL( db, COMMCSNAME, clName )

   assert.tryThrow( SDB_RTN_INVALID_HINT, function()
   {
      cl.find( { "a": 1, "b": 1 } ).hint( { "": "a" } ).flags( 0x00000080 ).explain();
   } )

   var explainDetail1 = cl.find( match ).sort( { a: 1 } ).explain();
   assert.equal( explainDetail1.current().toObj().UseExtSort, true );

   var explainDetail2 = cl.find( match ).remove().explain( { Detail: true } );
   assert.equal( explainDetail2.current().toObj().Hint.$Modify.Remove, true );

   commDropCL( db, COMMCSNAME, clName, false, false );
}
