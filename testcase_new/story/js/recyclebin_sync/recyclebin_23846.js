/******************************************************************************
 * @Description   : seqDB-23846:clearAll清空回收站
 * @Author        : liuli
 * @CreateTime    : 2021.04.19
 * @LastEditTime  : 2022.04.25
 * @LastEditors   : liuli
 ******************************************************************************/
main( test );

function test ()
{
   try
   {
      dropAll();
   }
   finally
   {
      db.getRecycleBin().alter( { MaxItemNum: 100 } );
      db.getRecycleBin().setAttributes( { MaxVersionNum: 2 } );
   }
}

function dropAll ()
{
   var csName = "cs_23846_";
   var clName = "cl_23846_";

   db.getRecycleBin().setAttributes( { MaxItemNum: -1 } );
   db.getRecycleBin().alter( { MaxVersionNum: -1 } );
   var maxItemNum = db.getRecycleBin().getDetail().toObj().MaxItemNum;
   assert.equal( maxItemNum, -1 );
   cleanRecycleBin( db, csName );

   // 产生大量回收站项目
   for( var i = 0; i < 5; i++ )
   {
      commDropCS( db, csName + i );
      var dbcs = commCreateCS( db, csName + i );
      var dbcl = commCreateCL( db, csName + i, clName + i );
      for( var j = 0; j < 100; j++ )
      {
         dbcl.insert( { a: j } );
         dbcl.truncate();
      }
      dbcs.dropCL( clName + i );
      db.dropCS( csName + i );
   }

   // 清空回收站后校验
   db.getRecycleBin().dropAll();
   var itemNum = db.getRecycleBin().count();
   assert.equal( itemNum, 0 );

   cleanRecycleBin( db, csName );
}