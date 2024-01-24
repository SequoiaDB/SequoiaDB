/******************************************************************************
 * @Description   : seqDB-24306:主表上所有索引复制到指定子表
 * @Author        : liuli
 * @CreateTime    : 2021.08.02
 * @LastEditTime  : 2021.09.08
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.csName = COMMCSNAME + "_24306";
testConf.clName = COMMCLNAME + "_maincl_24306";
testConf.clOpt = { ShardingKey: { a: 1 }, ShardingType: "range", IsMainCL: true };

main( test );
function test ( args )
{
   var indexName = "Index_24306_";
   var csName = testConf.csName;
   var mainCLName = testConf.clName;
   var subCLName1 = "subcl_24306_1";
   var subCLName2 = "subcl_24306_2";
   var maincl = args.testCL;

   // 创建多个索引
   var indexNames = [];
   var indexDef = new Object();
   for( var i = 0; i < 10; i++ )
   {
      indexDef["a" + i] = 1;
      maincl.createIndex( indexName + i, indexDef );
      delete indexDef["a" + i];
      indexNames.push( indexName + i );
   }

   commCreateCL( db, csName, subCLName1, { ShardingKey: { a: 1 }, ShardingType: "hash" } );
   maincl.attachCL( csName + "." + subCLName1, { LowBound: { a: 0 }, UpBound: { a: 1000 } } );
   commCreateCL( db, csName, subCLName2, { ShardingKey: { a: 1 }, ShardingType: "hash", ReplSize: 0 } );
   maincl.attachCL( csName + "." + subCLName2, { LowBound: { a: 1000 }, UpBound: { a: 2000 } } );

   var docs = [];
   for( var i = 0; i < 2000; i++ )
   {
      docs.push( { "a": i, "a0": i, "a1": i, "a2": i, "a3": i, "a4": i, "a5": i, "a6": i, "a7": i, "a8": i, "a9": i } );
   }
   maincl.insert( docs );
   maincl.copyIndex( csName + "." + subCLName1 );

   // 校验主表和子表subCLName1任务
   checkCopyTask( csName, mainCLName, indexNames, [csName + "." + subCLName1], 0 );
   checkIndexTask( "Create index", csName, mainCLName, indexNames, 0 );
   checkIndexTask( "Create index", csName, subCLName1, indexNames, 0 );
   // 校验子表subCLName2不存在任务
   checkNoTask( csName, subCLName2, "Create index" );

   for( var i = 0; i < indexNames.length; i++ )
   {
      checkIndexExist( db, csName, mainCLName, indexNames[i], true );
      commCheckIndexConsistent( db, csName, subCLName1, indexNames[i], true );
      commCheckIndexConsistent( db, csName, subCLName2, indexNames[i], false );
   }
}