/******************************************************************************
 * @Description   : seqDB-29719:修改statmcvlimit参数查看数据分布
 * @Author        : HuangHaimei
 * @CreateTime    : 2022.12.19
 * @LastEditTime  : 2023.01.03
 * @LastEditors   : HuangHaimei
 ******************************************************************************/
testConf.skipStandAlone = true;

main( test );
function test ()
{
   var indexName = "index_29719";
   var csName = "cs_29719";
   var mainCLName = "mainCL_29719";
   var subCLName = "subcl_29719";
   var subTableNum = 200;
   var totalRecords = 600000;
   var perSubRecords = totalRecords / subTableNum;
   var groups = commGetGroupsNum( db );

   commDropCS( db, csName );
   // 创建主表
   var maincl = commCreateCL( db, csName, mainCLName, { ShardingKey: { a: 1 }, ShardingType: "range", IsMainCL: true } );

   // 创建200个子表,并挂载至主表
   for( var i = 0; i < subTableNum; i++ )
   {
      commCreateCL( db, csName, subCLName + i, { ShardingKey: { a: 1 }, ShardingType: "hash", AutoSplit: true } );
      maincl.attachCL( csName + "." + subCLName + i, { LowBound: { a: perSubRecords * i }, UpBound: { a: perSubRecords * ( i + 1 ) } } );
   }

   // 插入数据
   var record = totalRecords / 5;
   var records_1 = record * 2;
   var records_2 = record;
   var records_3 = record;
   var records_4 = record;
   var num = 0;

   var docs_4 = [];
   for( var i = 0; i < records_4; i++ , num++ )
   {
      docs_4.push( { a: num, b: 4, c: totalRecords - num, no: 1 } );
   }
   maincl.insert( docs_4 );

   var docs_3 = [];
   for( var i = 0; i < records_3; i++ , num++ )
   {
      docs_3.push( { a: num, b: 3, c: totalRecords - num, no: 1 } );
   }
   maincl.insert( docs_3 );

   var docs_2 = [];
   for( var i = 0; i < records_2; i++ , num++ )
   {
      docs_2.push( { a: num, b: 2, c: totalRecords - num, no: 1 } );
   }
   maincl.insert( docs_2 );

   var docs_1 = [];
   for( var i = 0; i < records_1; i++ , num++ )
   {
      docs_1.push( { a: num, b: 1, c: totalRecords - num, no: 1 } );
   }
   maincl.insert( docs_1 );

   // 创建索引
   maincl.createIndex( indexName, { b: 1 } );

   // 对主表进行analyze，样本数指定为600
   db.analyze( {
      "Collection": csName + "." + mainCLName,
      "Index": indexName,
      "SampleNum": 600
   } );

   var SampleRecordsInfo = 600 * groups * subTableNum;
   var values = [{ "b": 1 }, { "b": 2 }, { "b": 3 }, { "b": 4 }];

   var actResult1 = maincl.getIndexStat( indexName, true ).toObj();
   delete ( actResult1.StatTimestamp );
   delete ( actResult1.TotalIndexLevels );
   delete ( actResult1.TotalIndexPages );
   delete ( actResult1.DistinctValNum );
   delete ( actResult1.MCV );

   try
   {
      var expResult1 = {
         "Collection": csName + "." + mainCLName,
         "Index": indexName,
         "Unique": false,
         "KeyPattern": { "b": 1 },
         "MinValue": values[0],
         "MaxValue": values[values.length - 1],
         "NullFrac": 0,
         "UndefFrac": 0,
         "SampleRecords": SampleRecordsInfo,
         "TotalRecords": totalRecords
      };
      assert.equal( actResult1, expResult1 );

      // 修改statmcvlimit为40w
      db.updateConf( { statmcvlimit: 400000 } );
      var fracs = [4000, 2000, 2000, 2000];
      var actResult2 = maincl.getIndexStat( indexName, true ).toObj();
      delete ( actResult2.StatTimestamp );
      delete ( actResult2.TotalIndexLevels );
      delete ( actResult2.TotalIndexPages );
      var expResult2 = {
         "Collection": csName + "." + mainCLName,
         "Index": indexName,
         "Unique": false,
         "KeyPattern": { "b": 1 },
         "DistinctValNum": [4],
         "MinValue": values[0],
         "MaxValue": values[values.length - 1],
         "NullFrac": 0,
         "UndefFrac": 0,
         "MCV": {
            "Values": values,
            "Frac": fracs
         },
         "SampleRecords": SampleRecordsInfo,
         "TotalRecords": totalRecords
      };
      assert.equal( actResult2, expResult2 );

      // 修改statmcvlimit为10w
      db.updateConf( { statmcvlimit: 100000 } );
      var actResult3 = maincl.getIndexStat( indexName, true ).toObj();
      delete ( actResult3.StatTimestamp );
      delete ( actResult3.TotalIndexLevels );
      delete ( actResult3.TotalIndexPages );
      delete ( actResult3.DistinctValNum );
      delete ( actResult3.MCV );

      var expResult3 = {
         "Collection": csName + "." + mainCLName,
         "Index": indexName,
         "Unique": false,
         "KeyPattern": { "b": 1 },
         "MinValue": values[0],
         "MaxValue": values[values.length - 1],
         "NullFrac": 0,
         "UndefFrac": 0,
         "SampleRecords": SampleRecordsInfo,
         "TotalRecords": totalRecords
      };
      assert.equal( actResult3, expResult3 );
   } finally
   {
      var config = { statmcvlimit: 200000 };
      db.deleteConf( config );
   }

   commDropCS( db, csName );
}