/******************************************************************************
 * @Description   : seqDB-25400:删除CL后删除CS不使用回收站 
 * @Author        : liuli
 * @CreateTime    : 2022.02.21
 * @LastEditTime  : 2022.03.25
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );
function test ()
{
   var csName = "cs_25403";
   var clName = "cl_25403";
   var docs = [{ a: 1, b: 1 }, { a: 2, b: "test2" }, { a: 3, b: 234.3 }];

   commDropCS( db, csName );
   cleanRecycleBin( db, csName );

   // 创建CL并插入数据
   var dbcs = commCreateCS( db, csName );
   var dbcl = dbcs.createCL( clName );
   dbcl.insert( docs );

   // 删除CL
   dbcs.dropCL( clName );

   // 检查回收站项目
   var recycleName = getRecycleName( db, csName + "." + clName, "Drop" );
   assert.equal( recycleName.length, 1 );

   // 删除CS不使用回收站
   db.dropCS( csName, { SkipRecycleBin: true } );

   // 检查回收站项目
   var recycleName = getRecycleName( db, csName + "." + clName, "Drop" );
   assert.equal( recycleName.length, 0 );

   cleanRecycleBin( db, csName );
}