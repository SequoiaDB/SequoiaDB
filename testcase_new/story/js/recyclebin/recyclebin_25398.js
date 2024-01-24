/******************************************************************************
 * @Description   : seqDB-25398:dropCL执行强制删除 
 * @Author        : liuli
 * @CreateTime    : 2022.02.21
 * @LastEditTime  : 2022.03.25
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );
function test ()
{
   var csName = "cs_25398";
   var clName = "cl_25398";
   var docs = [{ a: 1, b: 1 }, { a: 2, b: "test2" }, { a: 3, b: 234.3 }];

   commDropCS( db, csName );
   cleanRecycleBin( db, csName );

   // 创建CL并插入数据
   var dbcs = commCreateCS( db, csName );
   var dbcl = dbcs.createCL( clName );
   dbcl.insert( { a: 1 } );

   // 删除CL指定SkipRecycleBin为true
   dbcs.dropCL( clName, { SkipRecycleBin: true } );

   // 校验回收站项目
   var recycleName = getRecycleName( db, csName + "." + clName, "Drop" );
   assert.equal( recycleName.length, 0 );

   // 创建同名CL并插入数据
   var dbcl = dbcs.createCL( clName );
   dbcl.insert( docs );

   // 删除CL后恢复CL
   dbcs.dropCL( clName );
   var recycleName = getRecycleName( db, csName + "." + clName, "Drop" );
   db.getRecycleBin().returnItem( recycleName[0] );

   // 校验数据
   var dbcl = db.getCS( csName ).getCL( clName );
   var actResult = dbcl.find();
   commCompareResults( actResult, docs );

   commDropCS( db, csName );
   cleanRecycleBin( db, csName );
}