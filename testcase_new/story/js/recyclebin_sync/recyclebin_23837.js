/******************************************************************************
 * @Description   : seqDB-23837:MaxItemNum 参数校验
 * @Author        : Yang Qincheng
 * @CreateTime    : 2021.04.22
 * @LastEditTime  : 2022.08.15
 * @LastEditors   : liuli
 ******************************************************************************/

main( test );
function test ()
{
   var recycleBin = db.getRecycleBin();
   assert.equal( recycleBin.getDetail().toObj().MaxItemNum, 100 );
   try
   {
      // 有效参数校验
      // recycleBin.alter
      var maxItemNum = -1;
      recycleBin.alter( { "MaxItemNum": maxItemNum } );
      assert.equal( recycleBin.getDetail().toObj().MaxItemNum, maxItemNum );

      var maxItemNum = 0;
      recycleBin.alter( { "MaxItemNum": maxItemNum } );
      assert.equal( recycleBin.getDetail().toObj().MaxItemNum, maxItemNum );

      var maxItemNum = 100.001;
      recycleBin.alter( { "MaxItemNum": maxItemNum } );
      assert.equal( recycleBin.getDetail().toObj().MaxItemNum, Math.floor( maxItemNum ) );

      // recycleBin.setAttributes      
      var maxItemNum = 10;
      recycleBin.setAttributes( { "MaxItemNum": maxItemNum } );
      assert.equal( recycleBin.getDetail().toObj().MaxItemNum, maxItemNum );

      var maxItemNum = 2147483647;
      recycleBin.setAttributes( { "MaxItemNum": maxItemNum } );
      assert.equal( recycleBin.getDetail().toObj().MaxItemNum, maxItemNum );

      var maxItemNum = 100.001;
      recycleBin.setAttributes( { "MaxItemNum": maxItemNum } );
      assert.equal( recycleBin.getDetail().toObj().MaxItemNum, Math.floor( maxItemNum ) );

      // 无效参数校验
      assert.tryThrow( SDB_INVALIDARG, function() { recycleBin.alter( { "MaxItemNum": -2 } ) } );
      assert.tryThrow( SDB_INVALIDARG, function() { recycleBin.alter( { "MaxItemNum": 2147483648 } ) } );
      assert.tryThrow( SDB_INVALIDARG, function() { recycleBin.alter( { "MaxItemNum": -2147483647 } ) } );
      assert.tryThrow( SDB_INVALIDARG, function() { recycleBin.alter( { "MaxItemNum": true } ) } );
      assert.tryThrow( SDB_INVALIDARG, function() { recycleBin.alter( { "MaxItemNum": false } ) } );
      assert.tryThrow( SDB_INVALIDARG, function() { recycleBin.alter( { "MaxItemNum": "" } ) } );
   }
   finally
   {
      // 恢复默认值
      recycleBin.alter( { "MaxItemNum": 100 } );
   }
}
