/******************************************************************************
 * @Description   : seqDB-31343:transconsistencystrategy参数校验
 * @Author        : Bi Qin
 * @CreateTime    : 2023.04.28
 * @LastEditTime  : 2023.06.16
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );
function test ()
{
   // 校验默认transconsistencystrategy为3
   var expConfig = { transconsistencystrategy: 3 };
   checkSnapshot( db, expConfig );

   // 无效值检验
   var invalidValue = "";
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      db.updateConf( { transconsistencystrategy: invalidValue } );
   } );
   checkSnapshot( db, expConfig );

   invalidValue = "123";
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      db.updateConf( { transconsistencystrategy: invalidValue } );
   } );
   checkSnapshot( db, expConfig );

   invalidValue = true;
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      db.updateConf( { transconsistencystrategy: invalidValue } );
   } );
   checkSnapshot( db, expConfig );

   // 有效值检验
   var config = { transconsistencystrategy: 1 };
   db.updateConf( config );
   checkSnapshot( db, config );

   var config = { transconsistencystrategy: 2 };
   db.updateConf( config );
   checkSnapshot( db, config );

   var config = { transconsistencystrategy: 3 };
   db.updateConf( config );
   checkSnapshot( db, config );

   var config = { transconsistencystrategy: -100 };
   db.updateConf( config );
   var expConfig = { transconsistencystrategy: 1 };
   checkSnapshot( db, expConfig );

   var config = { transconsistencystrategy: 100 };
   db.updateConf( config );
   var expConfig = { transconsistencystrategy: 3 };
   checkSnapshot( db, expConfig );

   var config = { transconsistencystrategy: 1.1 };
   db.updateConf( config );
   var expConfig = { transconsistencystrategy: 1 };
   checkSnapshot( db, expConfig );

   db.deleteConf( { transconsistencystrategy: 1 } );
}

function checkSnapshot ( sdb, option )
{
   var cursor = sdb.snapshot( SDB_SNAP_CONFIGS );
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