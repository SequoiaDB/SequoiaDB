/******************************************************************************
 * @Description   : seqDB-25375 : MaxVersionNum参数校验
 * @Author        : 钟子明
 * @CreateTime    : 2022.02.18
 * @LastEditTime  : 2022.04.25
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.csName = COMMCSNAME + "_25375";
testConf.clName = COMMCLNAME + "_25375";

main( test );
function test ()
{
   var csName = "cs_25375";
   var clName = "cl_25375";

   var recycle = db.getRecycleBin();
   commDropCS( db, csName );
   cleanRecycleBin( db, csName );
   var dbcs = commCreateCS( db, csName );
   var dbcl = dbcs.createCL( clName );

   try
   {
      recycle.alter( { AutoDrop: true } );
      assert.equal( JSON.parse( recycle.getDetail() ).MaxVersionNum, 2, "回收站MaxVersionNum默认值有误" );

      recycle.alter( { MaxVersionNum: 0 } );
      recycle.alter( { MaxVersionNum: -1 } );
      recycle.alter( { MaxVersionNum: 1 } );
      recycle.alter( { MaxVersionNum: 2147483647 } );

      assert.tryThrow( SDB_INVALIDARG, function()
      {
         recycle.alter( { MaxVersionNum: -2 } );
      } );
      assert.tryThrow( SDB_INVALIDARG, function()
      {
         recycle.alter( { MaxVersionNum: null } );
      } );
      assert.tryThrow( SDB_INVALIDARG, function()
      {
         recycle.alter( { MaxVersionNum: 2147483648 } );
      } );
      assert.tryThrow( SDB_INVALIDARG, function()
      {
         recycle.alter( { MaxVersionNum: true } );
      } );
      assert.tryThrow( SDB_INVALIDARG, function()
      {
         recycle.alter( { MaxVersionNum: false } );
      } );
      assert.tryThrow( SDB_INVALIDARG, function()
      {
         recycle.alter( { MaxVersionNum: "2147483648" } );
      } );
      assert.tryThrow( SDB_INVALIDARG, function()
      {
         recycle.alter( { MaxVersionNum: "true" } );
      } );

      recycle.setAttributes( { MaxVersionNum: 0 } );
      recycle.setAttributes( { MaxVersionNum: -1 } );
      recycle.setAttributes( { MaxVersionNum: 1 } );
      recycle.setAttributes( { MaxVersionNum: 2147483647 } );

      assert.tryThrow( SDB_INVALIDARG, function()
      {
         recycle.setAttributes( { MaxVersionNum: -2 } );
      } );
      assert.tryThrow( SDB_INVALIDARG, function()
      {
         recycle.setAttributes( { MaxVersionNum: null } );
      } );
      assert.tryThrow( SDB_INVALIDARG, function()
      {
         recycle.setAttributes( { MaxVersionNum: 2147483648 } );
      } );
      assert.tryThrow( SDB_INVALIDARG, function()
      {
         recycle.setAttributes( { MaxVersionNum: true } );
      } );
      assert.tryThrow( SDB_INVALIDARG, function()
      {
         recycle.setAttributes( { MaxVersionNum: false } );
      } );
      assert.tryThrow( SDB_INVALIDARG, function()
      {
         recycle.setAttributes( { MaxVersionNum: "2147483648" } );
      } );
      assert.tryThrow( SDB_INVALIDARG, function()
      {
         recycle.setAttributes( { MaxVersionNum: "true" } );
      } );

      var setVersionNum = 100;
      recycle.alter( { MaxVersionNum: setVersionNum } );

      for( var i = 0; i < setVersionNum + 5; i++ )
      {
         dbcl.insert( { a: i } );
         dbcl.truncate();
      }

      assert.equal( recycle.count(), setVersionNum, '回收站对象数量与setVersionNum不相等' );
      commDropCS( db, csName );
      cleanRecycleBin( db, csName );
   }
   finally
   {
      recycle.alter( { AutoDrop: false } );
      recycle.alter( { MaxVersionNum: 2 } );
   }
}