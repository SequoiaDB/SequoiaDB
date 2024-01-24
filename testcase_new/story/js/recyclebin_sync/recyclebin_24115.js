/******************************************************************************
 * @Description   : seqDB-24115:设置AutoDrop为false，回收站项目满
 * @Author        : liuli
 * @CreateTime    : 2021.04.20
 * @LastEditTime  : 2022.02.17
 * @LastEditors   : liuli
 ******************************************************************************/
main( test );

function test ()
{
   try
   {
      autoDrop();
   }
   finally
   {
      db.getRecycleBin().alter( { AutoDrop: false } );
      db.getRecycleBin().alter( { MaxVersionNum: 2 } );
   }
}

function autoDrop ()
{
   var csName = "cs_24115";
   var clName = "cl_24115";

   db.getRecycleBin().alter( { AutoDrop: false } );
   db.getRecycleBin().alter( { MaxVersionNum: -1 } );
   var autoDrop = db.getRecycleBin().getDetail().toObj().AutoDrop;
   assert.equal( autoDrop, false );

   commDropCS( db, csName );
   cleanRecycleBin( db, csName );
   var dbcl = commCreateCL( db, csName, clName );
   var num = db.getRecycleBin().count();
   var maxItemNum = db.getRecycleBin().getDetail().toObj().MaxItemNum;

   for( var i = num; i < maxItemNum; i++ )
   {
      dbcl.insert( { a: i } );
      dbcl.truncate();
   }

   assert.tryThrow( SDB_RECYCLE_FULL, function()
   {
      db.dropCS( csName );
   } );

   db.getRecycleBin().dropAll();
   commDropCS( db, csName );
   cleanRecycleBin( db, csName );
}