/******************************************************************************
 * @Description   : seqDB-25403:回收站项目满，dropCS强制删除 
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

   // 创建CS/CL并插入数据
   var dbcs = commCreateCS( db, csName );
   var dbcl = dbcs.createCL( clName );
   dbcl.insert( docs );

   // 删除CS
   db.dropCS( csName );

   // 创建同名CS/CL，插入数据后删除CS
   var dbcs = commCreateCS( db, csName );
   var dbcl = dbcs.createCL( clName );
   dbcl.insert( docs );
   db.dropCS( csName );

   // 检查回收站项目
   var recycleName = getRecycleName( db, csName, "Drop" );
   assert.equal( recycleName.length, 2 );

   // 创建同名CS/CL并插入数据
   var dbcs = commCreateCS( db, csName );
   var dbcl = dbcs.createCL( clName );
   dbcl.insert( { a: 1 } );

   // 删除CS指定SkipRecycleBin为true
   db.dropCS( csName, { SkipRecycleBin: true } );

   // 恢复回收站项目并校验数据
   db.getRecycleBin().returnItem( recycleName[1] );
   var dbcl = db.getCS( csName ).getCL( clName );

   // 校验数据
   var dbcl = db.getCS( csName ).getCL( clName );
   var actResult = dbcl.find();
   commCompareResults( actResult, docs );

   commDropCS( db, csName );
   cleanRecycleBin( db, csName );
}