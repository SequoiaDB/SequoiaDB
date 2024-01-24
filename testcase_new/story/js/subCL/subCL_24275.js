/******************************************************************************
 * @Description   : seqDB-24275:主子表属于不同CS，删除CS
 * @Author        : liuli
 * @CreateTime    : 2021.06.22
 * @LastEditTime  : 2021.06.24
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;

main( test );

function test ()
{
   var csName1 = "cs_24275_1";
   var csName2 = "cs_24275_2";
   var mainCLName1 = "mainCL_24275_1";
   var mainCLName2 = "mainCL_24275_2";
   var subCLName1 = "subCL_24275_1";
   var subCLName2 = "subCL_24275_2";
   var subCLName3 = "subCL_24275_3";
   var subCLName4 = "subCL_24275_4";

   commDropCS( db, csName1 );
   commDropCS( db, csName2 );

   // 主子表在同一个CS
   var dbcs = db.createCS( csName1 );
   var mainCL = dbcs.createCL( mainCLName1, { ShardingKey: { a: 1 }, ShardingType: "range", IsMainCL: true } );
   dbcs.createCL( subCLName1, { ShardingKey: { a: 1 }, ShardingType: "hash" } );
   dbcs.createCL( subCLName2, { ShardingKey: { a: 1 }, ShardingType: "hash" } );
   mainCL.attachCL( csName1 + "." + subCLName1, { LowBound: { a: 0 }, UpBound: { a: 1000 } } );
   mainCL.attachCL( csName1 + "." + subCLName2, { LowBound: { a: 1000 }, UpBound: { a: 2000 } } );

   // 主子表在不同CS
   var dbcs2 = db.createCS( csName2 );
   var mainCL = dbcs2.createCL( mainCLName2, { ShardingKey: { a: 1 }, ShardingType: "range", IsMainCL: true } );
   dbcs2.createCL( subCLName3, { ShardingKey: { a: 1 }, ShardingType: "hash" } );
   dbcs.createCL( subCLName4, { ShardingKey: { a: 1 }, ShardingType: "hash" } );
   mainCL.attachCL( csName2 + "." + subCLName3, { LowBound: { a: 0 }, UpBound: { a: 1000 } } );
   mainCL.attachCL( csName1 + "." + subCLName4, { LowBound: { a: 1000 }, UpBound: { a: 2000 } } );

   db.dropCS( csName1 );
   db.dropCS( csName2 )
}