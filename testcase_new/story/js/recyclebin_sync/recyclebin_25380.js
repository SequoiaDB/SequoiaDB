/******************************************************************************
 * @Description   : seqDB-25380:MaxVersionNum配置大于MaxItemNum 
 * @Author        : liuli
 * @CreateTime    : 2022.02.23
 * @LastEditTime  : 2022.04.25
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;

main( test );
function test ()
{
   var csName = "cs_25380";
   var clName = "cl_25380";

   var recycle = db.getRecycleBin();
   commDropCS( db, csName );
   cleanRecycleBin( db, csName );
   var dbcs = commCreateCS( db, csName );
   var dbcl = dbcs.createCL( clName );

   try
   {
      // 修改回收站配置
      recycle.alter( { AutoDrop: true } );
      recycle.alter( { MaxItemNum: 5 } );
      recycle.alter( { MaxVersionNum: 10 } );

      // 循环执行插入数据后truncate
      truncateNum = 30;
      for( var i = 0; i < truncateNum; i++ )
      {
         dbcl.insert( { a: i } );
         dbcl.truncate();
      }

      // 检查回收站truncate项目数量
      var recycleNames = getRecycleName( db, csName + '.' + clName, 'Truncate' );
      assert.equal( recycleNames.length, 5 );

      // 恢复排序第一个的回收站项目
      recycle.returnItem( recycleNames[0] );

      var cursor = dbcl.find();
      commCompareResults( cursor, [{ a: truncateNum - 5 }] );
      checkRecycleItem( recycleNames[0] );

      commDropCS( db, csName );
      cleanRecycleBin( db, csName );
   }
   finally
   {
      recycle.alter( { AutoDrop: false } );
      recycle.alter( { MaxItemNum: 100 } );
      recycle.alter( { MaxVersionNum: 2 } );
   }
}