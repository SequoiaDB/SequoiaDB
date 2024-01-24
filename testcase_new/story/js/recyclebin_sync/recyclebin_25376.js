/******************************************************************************
 * @Description   : seqDB-25376:AutoDrop为true，MaxVersionNum默认值，回收项目
 * @Author        : 钟子明
 * @CreateTime    : 2022.02.18
 * @LastEditTime  : 2022.02.23
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;

main( test );
function test ()
{
   var csName = "cs_25376";
   var clName = "cl_25376";

   var recycle = db.getRecycleBin();
   commDropCS( db, csName );
   cleanRecycleBin( db, csName );
   var dbcs = commCreateCS( db, csName );
   var dbcl = dbcs.createCL( clName );

   try
   {
      recycle.alter( { AutoDrop: true } );

      // 执行truncate并记录回收站项目
      dbcl.insert( { a: 1 } );
      dbcl.truncate();
      var oldRecName = getOneRecycleName( db, csName + '.' + clName, 'Truncate' );

      // 连续执行2次truncate
      dbcl.insert( { a: 2 } );
      dbcl.truncate();
      dbcl.insert( { a: 3 } );
      dbcl.truncate();

      // 校验第一个回收站项目被删除
      var recycleNames = getRecycleName( db, csName + "." + clName, "Truncate" );
      assert.equal( recycleNames.length, 2 );
      checkRecycleItem( oldRecName );

      // 删除CL并记录回收站项目
      dbcs.dropCL( clName );
      var oldRecName = getOneRecycleName( db, csName + '.' + clName, 'Drop' );

      // 创建同名CL，并删除CL
      var dbcl = dbcs.createCL( clName );
      dbcl.insert( { a: 1 } );
      dbcs.dropCL( clName );

      // 校验回收站存在两个dropCL项目，不存在truncate项目
      var recycleNames = getRecycleName( db, csName + "." + clName, "Truncate" );
      assert.equal( recycleNames.length, 0 );
      var recycleNames = getRecycleName( db, csName + "." + clName, "Drop" );
      assert.equal( recycleNames.length, 2 );

      // 再次创建同名CL并删除CL
      var dbcl = dbcs.createCL( clName );
      dbcl.insert( { a: 1 } );
      dbcs.dropCL( clName );

      // 检查第一个dropCL项目被删除
      checkRecycleItem( oldRecName );

      // 删除CS
      db.dropCS( csName );
      var oldRecName = getOneRecycleName( db, csName, 'Drop' );

      // 创建同名CS后dropCS
      db.createCS( csName );
      db.dropCS( csName );
      db.createCS( csName );
      db.dropCS( csName );

      // 检查回收站项目
      var recycleNames = getRecycleName( db, csName + "." + clName, "Drop" );
      assert.equal( recycleNames.length, 0 );
      var recycleNames = getRecycleName( db, csName, "Drop" );
      assert.equal( recycleNames.length, 2 );
      checkRecycleItem( oldRecName );
      cleanRecycleBin( db, csName );
   }
   finally
   {
      recycle.alter( { AutoDrop: false } );
   }
}