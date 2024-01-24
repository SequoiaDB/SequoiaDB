/******************************************************************************
 * @Description   : seqDB-26592:diagsecureon配置参数校验
 * @Author        : liuli
 * @CreateTime    : 2022.05.27
 * @LastEditTime  : 2022.05.27
 * @LastEditors   : liuli
 ******************************************************************************/
main( test );
function test ()
{
   try
   {
      // 校验默认配置
      var actConfig = { diagsecureon: "TRUE" };
      checkSnapshot( db, actConfig );

      var config = { diagsecureon: false };
      db.updateConf( config );
      var actConfig = { diagsecureon: "FALSE" };
      checkSnapshot( db, actConfig );

      var config = { diagsecureon: true };
      db.updateConf( config );
      var actConfig = { diagsecureon: "TRUE" };
      checkSnapshot( db, actConfig );

      var config = { diagsecureon: 1 };
      db.updateConf( config );
      checkSnapshot( db, actConfig );

      var actConfig = { diagsecureon: "FALSE" };
      var config = { diagsecureon: 0 };
      db.updateConf( config );
      checkSnapshot( db, actConfig );

      var actConfig = { diagsecureon: "TRUE" };
      var config = { diagsecureon: -1 };
      db.updateConf( config );
      checkSnapshot( db, actConfig );

      var config = { diagsecureon: "true" };
      db.updateConf( config );
      checkSnapshot( db, actConfig );

      var config = { diagsecureon: "aaa" };
      db.updateConf( config );
      checkSnapshot( db, actConfig );
   }
   finally
   {
      db.deleteConf( { diagsecureon: "" } );
   }
}

function checkSnapshot ( sdb, option )
{
   var cursor = sdb.snapshot( SDB_SNAP_CONFIGS, {}, { diagsecureon: "" } );
   while( cursor.next() )
   {
      var obj = cursor.current().toObj();
      for( var key in option )
      {
         assert.equal( obj[key], option[key] );
      }
   }
   cursor.close();
}