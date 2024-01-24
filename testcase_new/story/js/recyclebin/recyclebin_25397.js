/******************************************************************************
 * @Description   : seqDB-25397:truncate执行强制删除 
 * @Author        : liuli
 * @CreateTime    : 2022.02.22
 * @LastEditTime  : 2022.03.25
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );
function test ()
{
   var csName = "cs_25397";
   var clName = "cl_25397";
   var docs = [{ a: 1, b: 1 }, { a: 2, b: "test2" }, { a: 3, b: 234.3 }];

   commDropCS( db, csName );
   cleanRecycleBin( db, csName );

   // 创建CL并插入数据
   var dbcs = commCreateCS( db, csName );
   var dbcl = dbcs.createCL( clName );
   dbcl.insert( { a: 1 } );

   // 执行truncate指定SkipRecycleBin为true
   dbcl.truncate( { SkipRecycleBin: true } );

   // 检查回收站项目
   var recycleName = getRecycleName( db, csName + "." + clName, "Truncate" );
   assert.equal( recycleName.length, 0 );

   // 再次插入数据
   dbcl.insert( docs );

   // 执行truncate指定SkipRecycleBin为false
   dbcl.truncate( { SkipRecycleBin: false } );

   // 校验数据被删除
   var actResult = dbcl.find();
   commCompareResults( actResult, [] );

   // 恢复回收站项目
   var recycleName = getRecycleName( db, csName + "." + clName, "Truncate" );
   db.getRecycleBin().returnItem( recycleName[0] );

   var dbcl = db.getCS( csName ).getCL( clName );
   var actResult = dbcl.find().sort( { a: 1 } );
   commCompareResults( actResult, docs );

   commDropCS( db, csName );
   cleanRecycleBin( db, csName );
}