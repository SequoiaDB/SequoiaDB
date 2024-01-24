/******************************************************************************
 * @Description   : seqDB-23782:CS下有多个CL，drop部分CL，dropCS后恢复CS
 * @Author        : liuli
 * @CreateTime    : 2021.04.06
 * @LastEditTime  : 2022.08.15
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;

main( test );
function test ()
{
   var csName = "cs_23782";
   var clName = "cl_23782_";
   var docs = [{ a: 1, b: 1 }, { a: 2, b: "test2" }, { a: 3, b: 234.3 }];

   commDropCS( db, csName );
   cleanRecycleBin( db, csName );

   var dbcs = commCreateCS( db, csName );
   for( var i = 0; i < 20; i++ )
   {
      dbcs.createCL( clName + i );
      dbcs.getCL( clName + i ).insert( docs );
   }

   // 删除部分CL
   for( var i = 0; i < 10; i++ )
   {
      dbcs.dropCL( clName + i );
   }

   // 删除CS后恢复
   db.dropCS( csName );
   var recycleName = getOneRecycleName( db, csName, "Drop" );
   db.getRecycleBin().returnItem( recycleName );

   // 检查恢复后CS下的CL数据正确性
   var dbcs = db.getCS( csName );
   for( var i = 0; i < 10; i++ )
   {
      assert.tryThrow( SDB_DMS_NOTEXIST, function()
      {
         dbcs.getCL( clName + i );
      } );
   }
   for( var i = 10; i < 20; i++ )
   {
      var cursor = dbcs.getCL( clName + i ).find().sort( { "a": 1 } );
      commCompareResults( cursor, docs );
   }

   // 恢复删除的CL项目
   for( var i = 0; i < 10; i++ )
   {
      var recycleName = getOneRecycleName( db, csName + "." + clName + i, "Drop" );
      db.getRecycleBin().returnItem( recycleName );
   }

   // 检查恢复后CL数据正确性
   for( var i = 0; i < 10; i++ )
   {
      var cursor = dbcs.getCL( clName + i ).find().sort( { "a": 1 } );
      commCompareResults( cursor, docs );
   }

   commDropCS( db, csName );
   cleanRecycleBin( db, csName );
}