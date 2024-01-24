/******************************************************************************
 * @Description   : seqDB-25064:getRecycleBin.snapshot查询空回收站 
 *                : seqDB-25402:SdbRecycle,count获取回收站项目数量 
 * @Author        : liuli
 * @CreateTime    : 2022.03.01
 * @LastEditTime  : 2022.03.23
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;

main( test );

function test ()
{
   var csName = "cs_25064";
   var clName = "cl_25064_";
   var fullName = csName + "." + clName + "+";

   commDropCS( db, csName );
   cleanRecycleBin( db, csName );
   var dbcs = commCreateCS( db, csName );

   var recycle = db.getRecycleBin();
   recycle.dropAll();
   try
   {
      recycle.alter( { MaxVersionNum: -1 } );

      // 多次创建CL执行truncate
      var clNum = 5;
      var truncateNum = 5;
      for( var i = 0; i < clNum; i++ )
      {
         var dbcl = dbcs.createCL( clName + i );
         for( var j = 0; j < truncateNum; j++ )
         {
            dbcl.insert( { a: j } );
            dbcl.truncate();
         }
         dbcs.dropCL( clName + i );
      }

      db.dropCS( csName );

      // 匹配存在的值
      var itemCount = recycle.count( { OriginName: { $regex: fullName } } );
      assert.equal( itemCount, 30 );

      // 匹配不存在的字段
      var itemCount = recycle.count( { "test": fullName } );
      assert.equal( itemCount, 0 );

      // 匹配存在的字段不存在的值
      var itemCount = recycle.count( { OriginName: "nocl_25064" } );
      assert.equal( itemCount, 0 );

      // seqDB-25064:getRecycleBin.snapshot查询空回收站 
      recycle.dropAll();

      // 等待LSN同步后进行校验
      commCheckBusinessStatus( db );

      // 空的回收站返回0条数据
      var cursor = recycle.snapshot();
      commCompareResults( cursor, [] );

      var cursor = db.snapshot( SDB_SNAP_RECYCLEBIN );
      commCompareResults( cursor, [] );

      commDropCS( db, csName );
      cleanRecycleBin( db, csName );
   }
   finally
   {
      recycle.alter( { MaxVersionNum: 2 } );
   }
}