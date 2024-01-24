/******************************************************************************
 * @Description   : seqDB-25388:配置MaxVersionNum为0，回收项目 
 * @Author        : liuli
 * @CreateTime    : 2022.02.24
 * @LastEditTime  : 2022.02.24
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;

main( test );

function test ()
{
   try
   {
      alterAttr( function()
      {
         db.getRecycleBin().alter( { MaxVersionNum: 0 } );
      } )
   }
   finally
   {
      db.getRecycleBin().alter( { MaxVersionNum: 2 } );
   }

   try
   {
      alterAttr( function()
      {
         db.getRecycleBin().setAttributes( { MaxVersionNum: 0 } );
      } )
   }
   finally
   {
      db.getRecycleBin().setAttributes( { MaxVersionNum: 2 } );
   }
}

function alterAttr ( func )
{
   var csName = "cs_25380";
   var clName = "cl_25380";

   commDropCS( db, csName );
   cleanRecycleBin( db, csName );
   var dbcs = commCreateCS( db, csName );
   var dbcl = dbcs.createCL( clName );

   func();

   // 多次执行truncate
   var truncateNum = 100;
   for( var i = 0; i < truncateNum; i++ )
   {
      dbcl.insert( { a: 1 } );
      dbcl.truncate();
   }

   // 检查回收站truncate项目数量
   var recycleNames = getRecycleName( db, csName + '.' + clName );
   assert.equal( recycleNames.length, 0 );

   commDropCS( db, csName );
}