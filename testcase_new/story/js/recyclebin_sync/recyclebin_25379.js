/******************************************************************************
 * @Description   : seqDB-25379:设置AutoDrop为true，MaxVersionNum回收回收truncate和dropCL项目，同名不同UniqueID项目
 * @Author        : 钟子明
 * @CreateTime    : 2022.02.21
 * @LastEditTime  : 2022.02.23
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;

main( test );
function test ()
{
   var csName = "cs_25379";
   var clName1 = "cl_25379_1";
   var clName2 = "cl_25379_2";

   var recycle = db.getRecycleBin();
   commDropCS( db, csName );
   cleanRecycleBin( db, csName );
   var dbcs = commCreateCS( db, csName );
   var dbcl = dbcs.createCL( clName1 );

   try
   {
      // 修改回收站配置
      recycle.alter( { AutoDrop: true } );

      // 插入数据后执行truncate
      dbcl.insert( { a: 1 } );
      dbcl.truncate();
      var oldRecName = getOneRecycleName( db, csName + '.' + clName1, 'Truncate' );

      // 再次执行truncate
      dbcl.insert( { a: 2 } );
      dbcl.truncate();

      // rename后删除CL
      dbcs.renameCL( clName1, clName2 );
      dbcs.dropCL( clName2 );

      // 检查回收站项目
      var recycleNames = getRecycleName( db, csName + '.' + clName1, 'Truncate' );
      assert.equal( recycleNames.indexOf( oldRecName ) < 0, true, "no " + oldRecName + " item is expected in the recycle bin" );
      assert.equal( recycleNames.length, 1 );
      var recycleNames = getRecycleName( db, csName + '.' + clName2, 'Drop' );
      assert.equal( recycleNames.length, 1 );

      commDropCS( db, csName );
      cleanRecycleBin( db, csName );
   }
   finally
   {
      recycle.alter( { AutoDrop: false } );
   }
}