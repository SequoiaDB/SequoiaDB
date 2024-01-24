/******************************************************************************
 * @Description   : seqDB-25406:删除CL后关闭回收站删除CS 
 * @Author        : liuli
 * @CreateTime    : 2022.02.22
 * @LastEditTime  : 2022.04.25
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );

function test ()
{
   // Enable为false
   try
   {
      testDropCS( function()
      {
         db.getRecycleBin().alter( { Enable: false } );
      } );
   }
   finally
   {
      db.getRecycleBin().alter( { Enable: true } );
   }

   // MaxItemNum为0
   try
   {
      testDropCS( function()
      {
         db.getRecycleBin().alter( { MaxItemNum: 0 } );
      } );
   }
   finally
   {
      db.getRecycleBin().alter( { MaxItemNum: 100 } );
   }

   // MaxVersionNum为0
   try
   {
      testDropCS( function()
      {
         db.getRecycleBin().alter( { MaxVersionNum: 0 } );
      } );
   }
   finally
   {
      db.getRecycleBin().alter( { MaxVersionNum: 2 } );
   }
}

function testDropCS ( func )
{
   var csName = "cs_25406";
   var clName = "cl_25406";
   var docs = [{ a: 1, b: 1 }, { a: 2, b: "test2" }, { a: 3, b: 234.3 }];

   commDropCS( db, csName );
   cleanRecycleBin( db, csName );

   // 创建CL并插入数据
   var dbcs = commCreateCS( db, csName );
   var dbcl = dbcs.createCL( clName );
   dbcl.insert( docs );

   // 删除CL
   dbcs.dropCL( clName );

   // 检查回收站项目
   var recycleName = getRecycleName( db, csName + "." + clName, "Drop" );
   assert.equal( recycleName.length, 1 );

   func();

   // 删除CS项目
   db.dropCS( csName );

   // 检查回收站项目
   var recycleName = getRecycleName( db, csName + "." + clName, "Drop" );
   assert.equal( recycleName.length, 0 );

   var recycleName = getRecycleName( db, csName, "Drop" );
   assert.equal( recycleName.length, 0 );

   commDropCS( db, csName );
   cleanRecycleBin( db, csName );
}