/******************************************************************************
 * @Description   : seqDB-25377:AutoDrop为true，MaxVersionNum回收不同名同UniqueID项目
 * @Author        : 钟子明
 * @CreateTime    : 2022.02.21
 * @LastEditTime  : 2022.04.13
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;

main( test );
function test ()
{
   var csName = "cs_25377";
   var clName = "cl_25377";
   var clName1 = "cl_25377_1";
   var clName2 = "cl_25377_2";

   var recycle = db.getRecycleBin();
   commDropCS( db, csName );
   cleanRecycleBin( db, csName );
   var dbcs = commCreateCS( db, csName );
   var dbcl = dbcs.createCL( clName );
   try
   {
      // 修改回收站属性
      recycle.alter( { AutoDrop: true } );

      // 插入数据后执行truncate
      dbcl.insert( { a: 1 } );
      dbcl.truncate();
      var oldRecName = getOneRecycleName( db, csName + '.' + clName, 'Truncate' );

      // rename后执行truncate
      dbcs.renameCL( clName, clName1 );
      var dbcl = db.getCS( csName ).getCL( clName1 );
      dbcl.insert( { a: 2 } )
      dbcl.truncate();

      // 再次rename后执行truncate
      dbcs.renameCL( clName1, clName2 );
      var dbcl = db.getCS( csName ).getCL( clName2 );
      dbcl.insert( { a: 3 } );
      dbcl.truncate();

      // 校验第一个回收站项目不存在，后面两个回收站项目存在
      var recycleName = getRecycleName( db, csName + "." + clName );
      assert.equal( recycleName.length, 0 );
      var recycleName = getRecycleName( db, csName + "." + clName1 );
      assert.equal( recycleName.length, 1 );
      var recycleName = getRecycleName( db, csName + "." + clName2 );
      assert.equal( recycleName.length, 1 );

      // 恢复第一个回收站项目报错
      assert.tryThrow( [SDB_RECYCLE_ITEM_NOTEXIST], function() 
      {
         db.getRecycleBin().returnItem( oldRecName );
      } );

      commDropCS( db, csName );
      cleanRecycleBin( db, csName );
   }
   finally
   {
      recycle.alter( { AutoDrop: false } );
   }
}