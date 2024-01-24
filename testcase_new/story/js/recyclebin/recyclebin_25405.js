/******************************************************************************
 * @Description   : seqDB-25405:回收站项目满，truncate强制删除 
 * @Author        : liuli
 * @CreateTime    : 2022.02.22
 * @LastEditTime  : 2022.03.25
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );
function test ()
{
   var csName = "cs_25405";
   var clName = "cl_25405";
   var docs = [{ a: 1, b: 1 }, { a: 2, b: "test2" }, { a: 3, b: 234.3 }];

   commDropCS( db, csName );
   cleanRecycleBin( db, csName );

   // 创建CL并插入数据
   var dbcs = commCreateCS( db, csName );
   var dbcl = dbcs.createCL( clName );
   dbcl.insert( docs );

   // 执行truncate
   dbcl.truncate();

   // 插入数据后再次执行truncate
   dbcl.insert( { a: 1 } );
   dbcl.truncate();

   // 检查回收站项目
   var recycleNames = getRecycleName( db, csName + "." + clName, "Truncate" );
   assert.equal( recycleNames.length, 2 );

   // 插头数据后执行truncate指定SkipRecycleBin为true
   dbcl.insert( { a: 1 } );
   dbcl.truncate( { SkipRecycleBin: true } );

   // 恢复回收站的第一个truncate项目
   db.getRecycleBin().returnItem( recycleNames[0] );

   var actResult = dbcl.find().sort( { a: 1 } );
   commCompareResults( actResult, docs );

   commDropCS( db, csName );
   cleanRecycleBin( db, csName );
}