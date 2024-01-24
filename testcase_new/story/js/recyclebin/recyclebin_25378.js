/******************************************************************************
 * @Description   : seqDB-25378:回收站项OriginName和OriginID相同项目小于MaxVersionNum，总项目大于MaxVersionNum
 * @Author        : liuli
 * @CreateTime    : 2022.02.22
 * @LastEditTime  : 2022.06.28
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );
function test ()
{
   var csName = "cs_25378";
   var clName1 = "cl_25378_1";
   var clName2 = "cl_25378_2";
   var docs = [{ a: 1, b: 1 }, { a: 2, b: "test2" }, { a: 3, b: 234.3 }];

   commDropCS( db, csName );
   cleanRecycleBin( db, csName );

   // 创建CL并插入数据
   var dbcs = commCreateCS( db, csName );
   var dbcl = dbcs.createCL( clName1 );
   dbcl.insert( docs );

   // 删除CL后记录对应的回收站项目
   dbcs.dropCL( clName1 );
   var oldRecName = getOneRecycleName( db, csName + "." + clName1, "Drop" );

   // 创建同名CL并执行truncate
   var dbcl = dbcs.createCL( clName1 );
   dbcl.insert( { a: 1 } );
   dbcl.truncate();

   // renameCL后执行truncate
   dbcs.renameCL( clName1, clName2 );
   var dbcl = dbcs.getCL( clName2 );
   dbcl.insert( { a: 2 } );
   dbcl.truncate();

   // 校验回收站项目
   var recycleName = getOneRecycleName( db, csName + "." + clName1, "Drop" );
   assert.equal( recycleName, oldRecName );
   var recycleNams = getRecycleName( db, csName + "." + clName1, "Truncate" );
   assert.equal( recycleNams.length, 1 );
   var recycleNams = getRecycleName( db, csName + "." + clName2, "Truncate" );
   assert.equal( recycleNams.length, 1 );

   // 恢复dropCL项目
   db.getRecycleBin().returnItem( oldRecName );

   var dbcl = dbcs.getCL( clName1 );
   var cursor = dbcl.find().sort( { a: 1 } );
   commCompareResults( cursor, docs );
   checkRecycleItem( oldRecName );

   commDropCS( db, csName );
   cleanRecycleBin( db, csName );
}