/******************************************************************************
 * @Description   : seqDB-23832:配置ExpireTime:0，回收项目
 * @Author        : liuli
 * @CreateTime    : 2021.04.16
 * @LastEditTime  : 2021.08.13
 * @LastEditors   : liuli
 ******************************************************************************/
main( test );

function test ()
{
   try
   {
      alterRecycleBin( function()
      {
         db.getRecycleBin().alter( { ExpireTime: 0 } )
      } );
   }
   finally
   {
      db.getRecycleBin().alter( { ExpireTime: 4320 } )
   }

   try
   {
      alterRecycleBin( function()
      {
         db.getRecycleBin().setAttributes( { ExpireTime: 0 } )
      } );
   }
   finally
   {
      db.getRecycleBin().alter( { ExpireTime: 4320 } )
   }
}

function alterRecycleBin ( func )
{
   var csName = "cs_23832";
   var clName = "cl_23832";

   commDropCS( db, csName );
   cleanRecycleBin( db, csName );

   // 配置回收站属性
   func();
   var expireTime = db.getRecycleBin().getDetail().toObj().ExpireTime;
   assert.equal( expireTime, 0 );

   var dbcs = commCreateCS( db, csName );
   var dbcl = dbcs.createCL( clName );
   var docs = [{ a: 1, b: 1 }, { a: 2, b: 2 }, { a: 3, b: 3 }];
   dbcl.insert( docs );

   // 删除 cl 后检查回收站项目
   dbcs.dropCL( clName );
   var option = { "OriginName": csName + "." + clName };
   var recyclebinItem = db.getRecycleBin().count( option );
   assert.equal( recyclebinItem, 0, "Recycle bin property error" );

   commDropCS( db, csName );
}