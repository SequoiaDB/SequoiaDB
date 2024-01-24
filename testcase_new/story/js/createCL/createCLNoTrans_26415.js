/******************************************************************************
 * @Description   : seqDB-26415:主表下部分子表指定无事务属性
 * @Author        : liuli
 * @CreateTime    : 2022.04.24
 * @LastEditTime  : 2022.04.24
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;

main( test );
function test ( args )
{
   var csName = "cs_26415";
   var mainCLName = "maincl_26415";
   var subCLName1 = "subcl_26415_1";
   var subCLName2 = "subcl_26415_2";

   commDropCS( db, csName );
   var maincl = commCreateCL( db, csName, mainCLName, { ShardingKey: { a: 1 }, ShardingType: "range", IsMainCL: true } );
   commCreateCL( db, csName, subCLName1, { ShardingKey: { a: 1 }, ShardingType: "hash" } );
   maincl.attachCL( csName + "." + subCLName1, { LowBound: { a: 0 }, UpBound: { a: 500 } } );
   commCreateCL( db, csName, subCLName2, { ShardingKey: { a: 1 }, ShardingType: "hash", NoTrans: true } );
   maincl.attachCL( csName + "." + subCLName2, { LowBound: { a: 500 }, UpBound: { a: 1000 } } );

   var recsNum = 1000;
   var docs = [];
   var expResult = []
   for( var i = 0; i < recsNum; i++ )
   {
      docs.push( { a: i, b: i } );
      if( i >= 500 )
      {
         expResult.push( { a: i, b: i } );
      }
   }

   // 开启事务 
   db.transBegin();

   // 插入数据
   maincl.insert( docs );

   // 回滚事务
   db.transRollback();

   // 查询数据
   var actResult = maincl.find().sort( { a: 1 } );
   commCompareResults( actResult, expResult );

   commDropCS( db, csName );
}