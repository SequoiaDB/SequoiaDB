/******************************************************************************
 * @Description   : seqDB-28720:执行analyz,向主表插入大量数据，使主表部分/全部过期，不执行analyze
 * @Author        : HuangHaimei
 * @CreateTime    : 2022.12.21
 * @LastEditTime  : 2023.01.12
 * @LastEditors   : HuangHaimei
 ******************************************************************************/
testConf.skipStandAlone = true;

main( test );
function test ()
{
   var csName = "cs_28720";
   var mainCLName = "mainCL_28720";
   var subCLName1 = "subCL_28720_1";
   var subCLName2 = "subCL_28720_2";

   commDropCS( db, csName );

   var mainCL = commCreateCL( db, csName, mainCLName, { ShardingKey: { a: 1 }, ShardingType: "range", IsMainCL: true } );
   commCreateCL( db, csName, subCLName1, { ShardingKey: { a: 1 }, ShardingType: "hash" } );
   commCreateCL( db, csName, subCLName2, { ShardingKey: { a: 1 }, ShardingType: "hash" } );
   mainCL.attachCL( csName + "." + subCLName1, { LowBound: { a: 0 }, UpBound: { a: 500000 } } );
   mainCL.attachCL( csName + "." + subCLName2, { LowBound: { a: 500000 }, UpBound: { a: 900000 } } );

   db.analyze( { "Collection": csName + "." + mainCLName } );
   // 表1插入大量数据，使主表部分过期
   var docs1 = [];
   for( var i = 0; i < 300000; i++ )
   {
      docs1.push( { a: i } );
   }
   mainCL.insert( docs1 );

   var isDefault = false;
   var isExpired = true;
   var avgNumFields = 10;
   var sampleRecords = 0;
   var totalRecords = 0;
   var totalDataPages = 0;
   var totalDataSize = 0;
   // 返回过期值
   checkCollectionStat( mainCL, isDefault, isExpired, avgNumFields, sampleRecords, totalRecords, totalDataPages, totalDataSize );
   // 表2插入大量数据，使主表全部过期
   var docs2 = [];
   for( var i = 500000; i < 800000; i++ )
   {
      docs2.push( { a: i } );
   }
   mainCL.insert( docs2 );
   // 返回过期值
   checkCollectionStat( mainCL, isDefault, isExpired, avgNumFields, sampleRecords, totalRecords, totalDataPages, totalDataSize );

   commDropCS( db, csName );
}