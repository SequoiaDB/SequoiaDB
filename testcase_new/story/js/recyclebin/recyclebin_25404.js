/******************************************************************************
 * @Description   : seqDB-25404:回收站项目满，dropCL强制删除
 * @Author        : liuli
 * @CreateTime    : 2022.02.22
 * @LastEditTime  : 2022.03.25
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );
function test ()
{
   var csName = "cs_25404";
   var clName = "cl_25404";
   var docs = [{ a: 1, b: 1 }, { a: 2, b: "test2" }, { a: 3, b: 234.3 }];

   commDropCS( db, csName );
   cleanRecycleBin( db, csName );

   // 创建CL并插入数据
   var dbcs = commCreateCS( db, csName );
   var dbcl = dbcs.createCL( clName );
   dbcl.insert( docs );

   // 删除CL
   dbcs.dropCL( clName );

   // 创建同名CL并插入数据
   var dbcl = dbcs.createCL( clName );
   dbcl.insert( { a: 1 } );

   // 删除CL
   dbcs.dropCL( clName );

   // 检查回收站项目
   var recycleNames = getRecycleName( db, csName + "." + clName, "Drop" );
   assert.equal( recycleNames.length, 2 );

   // 再次创建同名CL并插入数据
   var dbcl = dbcs.createCL( clName );
   dbcl.insert( { a: 1 } );

   // 删除CL指定SkipRecycleBin为true
   dbcs.dropCL( clName, { SkipRecycleBin: true } );

   // 检查回收站项目
   var recycleNames = getRecycleName( db, csName + "." + clName, "Drop" );
   assert.equal( recycleNames.length, 2 );

   // 恢复第一个回收站项目
   db.getRecycleBin().returnItem( recycleNames[0] );

   var dbcl = db.getCS( csName ).getCL( clName );
   var actResult = dbcl.find().sort( { a: 1 } );
   commCompareResults( actResult, docs );

   commDropCS( db, csName );
   cleanRecycleBin( db, csName );
}