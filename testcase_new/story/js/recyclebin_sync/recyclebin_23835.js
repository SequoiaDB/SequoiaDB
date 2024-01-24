/******************************************************************************
 * @Description   : seqDB-23835:配置MaxItemNum:0，回收项目
 * @Author        : liuli
 * @CreateTime    : 2021.04.16
 * @LastEditTime  : 2022.04.25
 * @LastEditors   : liuli
 ******************************************************************************/
main( test );

function test ()
{
   try
   {
      alterRecycleBin( function()
      {
         db.getRecycleBin().alter( { MaxItemNum: 0 } )
      } );
   }
   finally
   {
      db.getRecycleBin().alter( { MaxItemNum: 100 } )
   }

   try
   {
      alterRecycleBin( function()
      {
         db.getRecycleBin().setAttributes( { MaxItemNum: 0 } )
      } );
   }
   finally
   {
      db.getRecycleBin().alter( { MaxItemNum: 100 } )
   }
}

function alterRecycleBin ( func )
{
   var csName = "cs_23835";
   var clName = "cl_23835";

   commDropCS( db, csName );
   cleanRecycleBin( db, csName );

   // 配置回收站属性
   func();
   var maxItemNum = db.getRecycleBin().getDetail().toObj().MaxItemNum;
   assert.equal( maxItemNum, 0 );

   var dbcs = commCreateCS( db, csName );
   var dbcl = dbcs.createCL( clName );
   var docs = [{ a: 1, b: 1 }, { a: 2, b: 2 }, { a: 3, b: 3 }];
   dbcl.insert( docs );

   // 执行 truncate 后检查回收站项目
   dbcl.truncate();
   var option = { "OriginName": csName + "." + clName };
   var recyclebinItem = db.getRecycleBin().count( option );
   assert.equal( recyclebinItem, 0, "Recycle bin property error" );

   // 再次插入数据执行truncate
   dbcl.insert( docs );
   dbcl.truncate();
   var option = { "OriginName": csName + "." + clName };
   var recyclebinItem = db.getRecycleBin().count( option );
   assert.equal( recyclebinItem, 0, "Recycle bin property error" );

   commDropCS( db, csName );
}