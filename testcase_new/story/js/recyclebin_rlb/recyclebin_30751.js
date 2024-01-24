/******************************************************************************
 * @Description   : seqDB-30751:存在空复制组，使用回收站执行操作
 * @Author        : Bi Qin
 * @CreateTime    : 2023.03.25
 * @LastEditTime  : 2023.03.29
 * @LastEditors   : Bi Qin
 ******************************************************************************/
testConf.skipStandAlone = true;

main( test );
function test ()
{
   var csName = "cs_30751";
   var rgName = "group_30751";
   try
   {
      commDropCS( db, csName );
      db.getRecycleBin().dropAll();
      commCreateCS( db, csName );
      db.dropCS( csName );

      db.createRG( rgName );
      db.getRecycleBin().dropAll();
      var count = db.getRecycleBin().count();
      assert.equal( count, 0 );

      db.getRecycleBin().alter( { ExpireTime: 999 } );
      assert.equal( db.getRecycleBin().getDetail().toObj().ExpireTime, 999 );
   }
   finally
   {
      removeDataRG( db, rgName )
      db.getRecycleBin().alter( { ExpireTime: 4320 } );
   }
}

function removeDataRG ( db, rgName )
{
   try
   {
      db.removeRG( rgName );
   }
   catch( e )
   {
      if( e != SDB_CLS_GRP_NOT_EXIST )
      {
         throw new Error( e );
      }
   }
}
