/******************************************************************************
 * @Description   : seqDB-25401:truncate后删除CS不使用回收站 
 * @Author        : liuli
 * @CreateTime    : 2022.02.22
 * @LastEditTime  : 2022.03.25
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );
function test ()
{
   var csName = "cs_25401";
   var clName = "cl_25401";
   var docs = [{ a: 1, b: 1 }, { a: 2, b: "test2" }, { a: 3, b: 234.3 }];

   commDropCS( db, csName );
   cleanRecycleBin( db, csName );

   // 创建CL并插入数据
   var dbcs = commCreateCS( db, csName );
   var dbcl = dbcs.createCL( clName );
   dbcl.insert( { a: 1 } );

   // 执行truncate不指定SkipRecycleBin
   dbcl.truncate();

   // 检查回收站项目
   var recycleName = getRecycleName( db, csName + "." + clName, "Truncate" );
   assert.equal( recycleName.length, 1 );

   // 再次插入数据
   dbcl.insert( docs );

   // 删除CS指定SkipRecycleBin为true
   db.dropCS( csName, { SkipRecycleBin: true } );

   // 检查回收站项目
   var recycleName = getRecycleName( db, csName + "." + clName, "Truncate" );
   assert.equal( recycleName.length, 0 );

   commDropCS( db, csName );
   cleanRecycleBin( db, csName );
}