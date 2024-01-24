/******************************************************************************
 * @Description   : seqDB-29717:statmcvlimit参数校验
 * @Author        : HuangHaimei
 * @CreateTime    : 2022.12.19
 * @LastEditTime  : 2022.12.28
 * @LastEditors   : HuangHaimei
 ******************************************************************************/
main( test );
function test ()
{
   try
   {
      // 设置statmcvlimit为负数
      var config = { statmcvlimit: -1 };
      assert.tryThrow( SDB_INVALIDARG, function()
      {
         db.updateConf( config );
      } );

      // 设置statmcvlimit超过最大限制
      var config = { statmcvlimit: 2000001 };
      db.updateConf( config );
      var cursor = db.snapshot( SDB_SNAP_CONFIGS, {}, { NodeName: "", statmcvlimit: "" } );
      checkConfig( cursor, 2000000 );

      // 设置statmcvlimit为null
      var config = { statmcvlimit: null };
      assert.tryThrow( SDB_INVALIDARG, function()
      {
         db.updateConf( config );
      } );

      // 设置statmcvlimit为字符串
      var config = { statmcvlimit: "巨杉数据库" };
      assert.tryThrow( SDB_INVALIDARG, function()
      {
         db.updateConf( config );
      } );

      // 设置statmcvlimit为bool类型
      var config = { statmcvlimit: true };
      assert.tryThrow( SDB_INVALIDARG, function()
      {
         db.updateConf( config );
      } );

      var config = { statmcvlimit: false };
      assert.tryThrow( SDB_INVALIDARG, function()
      {
         db.updateConf( config );
      } );

      // 设置statmcvlimit为0
      var config = { statmcvlimit: 0 };
      db.updateConf( config );
      var cursor = db.snapshot( SDB_SNAP_CONFIGS, {}, { NodeName: "", statmcvlimit: "" } );
      checkConfig( cursor, 0 );

      // 设置statmcvlimit为2000000
      var config = { statmcvlimit: 2000000 };
      db.updateConf( config );
      var cursor = db.snapshot( SDB_SNAP_CONFIGS, {}, { NodeName: "", statmcvlimit: "" } );
      checkConfig( cursor, 2000000 );
   }
   finally
   {
      var config = { statmcvlimit: 200000 };
      db.deleteConf( config );
   }
}

function checkConfig ( cursor, number )
{
   while( cursor.next() )
   {
      var statmcvlimitInfo = cursor.current().toObj()["statmcvlimit"];
      var nodeName = cursor.current().toObj()["NodeName"];
      assert.equal( number, statmcvlimitInfo, "nodeName:" + nodeName );
   }
   cursor.close();
}