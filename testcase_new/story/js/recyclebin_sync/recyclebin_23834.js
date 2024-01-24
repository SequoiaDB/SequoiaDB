/******************************************************************************
 * @Description   : seqDB-23834:ExpireTime 参数校验
 * @Author        : Yang Qincheng
 * @CreateTime    : 2021.04.22
 * @LastEditTime  : 2022.08.15
 * @LastEditors   : liuli
 ******************************************************************************/

main( test );
function test ()
{
   var recycleBin = db.getRecycleBin();
   assert.equal( recycleBin.getDetail().toObj().ExpireTime, 4320 );
   try
   {
      // 有效参数校验
      // recycleBin.alter
      var expireTime = -1;
      recycleBin.alter( { "ExpireTime": expireTime } );
      assert.equal( recycleBin.getDetail().toObj().ExpireTime, expireTime );

      var expireTime = 0;
      recycleBin.alter( { "ExpireTime": expireTime } );
      assert.equal( recycleBin.getDetail().toObj().ExpireTime, expireTime );

      var expireTime = 100.001;
      recycleBin.alter( { "ExpireTime": expireTime } );
      assert.equal( recycleBin.getDetail().toObj().ExpireTime, Math.floor( expireTime ) );

      // recycleBin.setAttributes      
      var expireTime = 100;
      recycleBin.setAttributes( { "ExpireTime": expireTime } );
      assert.equal( recycleBin.getDetail().toObj().ExpireTime, expireTime );

      var expireTime = 2147483647;
      recycleBin.setAttributes( { "ExpireTime": expireTime } );
      assert.equal( recycleBin.getDetail().toObj().ExpireTime, expireTime );

      var expireTime = 100.001;
      recycleBin.setAttributes( { "ExpireTime": expireTime } );
      assert.equal( recycleBin.getDetail().toObj().ExpireTime, Math.floor( expireTime ) );

      // 无效参数校验
      assert.tryThrow( SDB_INVALIDARG, function() { recycleBin.setAttributes( { "ExpireTime": -2 } ) } );
      assert.tryThrow( SDB_INVALIDARG, function() { recycleBin.setAttributes( { "ExpireTime": 2147483648 } ) } );
      assert.tryThrow( SDB_INVALIDARG, function() { recycleBin.setAttributes( { "ExpireTime": -2147483647 } ) } );
      assert.tryThrow( SDB_INVALIDARG, function() { recycleBin.setAttributes( { "ExpireTime": true } ) } );
      assert.tryThrow( SDB_INVALIDARG, function() { recycleBin.setAttributes( { "ExpireTime": false } ) } );
      assert.tryThrow( SDB_INVALIDARG, function() { recycleBin.setAttributes( { "ExpireTime": "" } ) } );
   }
   finally
   {
      // 恢复默认值
      recycleBin.alter( { "ExpireTime": 4320 } );
   }
}