/*******************************************************************************
*@Description:   seqDB-13717: hash分区表使用切分键/非切分键sort+limit+skip执行查询 
*@Author:        2019-5-14  wangkexin
********************************************************************************/
testConf.skipStandAlone = true;
testConf.skipOneGroup = true;

testConf.useSrcGroup = true;
testConf.useDstGroup = true;
testConf.clOpt = { ShardingKey: { a: 1 }, ShardingType: 'hash', ReplSize: 0 };
testConf.clName = COMMCLNAME + "_cl13717";
main( test );

function test ( testPara )
{
   var rownums = 10000;
   var srcGroupName = testPara.srcGroupName;
   var dstGroupName = testPara.dstGroupNames[0];
   testPara.testCL.split( srcGroupName, dstGroupName, 50 );
   var insetRecs = loadDataAndCreateIndex( testPara.testCL, rownums );

   //query 1 使用sort执行查询
   var sel_1 = testPara.testCL.find().sort( { a: 1 } );
   checkRec( sel_1, insetRecs );

   //query 2 使用limit执行查询
   var sel_2 = testPara.testCL.find().limit( 1500 );
   checkResultNum( sel_2, 1500 );

   //query 3 使用skip执行查询，覆盖表扫描和索引扫描  
   //a.使某个分区组一次返回的记录数小于skip的数目，这里指定skip为1000，2000
   var sel_3_1_table = testPara.testCL.find().skip( 1000 ).hint( { "": null } );
   checkResultNum( sel_3_1_table, rownums - 1000 );

   var sel_3_1_index = testPara.testCL.find().skip( 2000 ).hint( { "": "aIndex" } );
   checkResultNum( sel_3_1_index, rownums - 2000 );

   //b.使某个分区组一次返回的记录数大于skip的数目，这里指定skip为1
   var sel_3_2_table = testPara.testCL.find().skip( 1 ).hint( { "": null } );
   checkResultNum( sel_3_2_table, rownums - 1 );

   var sel_3_2_index = testPara.testCL.find().skip( 1 ).hint( { "": "aIndex" } );
   checkResultNum( sel_3_2_index, rownums - 1 );

   //query 4 使用sort+limit+skip执行查询,覆盖表扫描和索引扫描
   //a.使某个分区组一次返回的记录数小于skip的数目，这里指定skip为1000，3000
   var sel_4_1_table = testPara.testCL.find().sort( { a: 1 } ).limit( 1500 ).skip( 1000 ).hint( { "": null } );
   var expRec = getExpRec( insetRecs, 1000, 2499 );
   checkRec( sel_4_1_table, expRec );

   var sel_4_1_index = testPara.testCL.find().sort( { a: 1 } ).limit( 1500 ).skip( 3000 ).hint( { "": "aIndex" } );
   var expRec = getExpRec( insetRecs, 3000, 4499 );
   checkRec( sel_4_1_index, expRec );

   //b.使某个分区组一次返回的记录数大于skip的数目，这里指定skip为1
   var sel_4_2_table = testPara.testCL.find().sort( { a: 1 } ).limit( 1500 ).skip( 1 ).hint( { "": null } );
   var expRec = getExpRec( insetRecs, 1, 1500 );
   checkRec( sel_4_2_table, expRec );

   var sel_4_2_index = testPara.testCL.find().sort( { a: 1 } ).limit( 1500 ).skip( 1 ).hint( { "": "aIndex" } );
   var expRec = getExpRec( insetRecs, 1, 1500 );
   checkRec( sel_4_2_index, expRec );
}

function loadDataAndCreateIndex ( cl, rownums )
{
   var record = [];
   for( var i = 0; i < rownums; i++ )
   {
      record.push( { _id: i, a: i, b: i, c: i } );
   }
   cl.insert( record );
   cl.createIndex( "aIndex", { a: 1 }, true );
   return record;
}

function checkResultNum ( sel, expResultNum )
{
   var actResultNum = sel.size();
   assert.equal( actResultNum, expResultNum );
}

function getExpRec ( record, start, end )
{
   var expRec = [];
   for( var i = start; i <= end; i++ )
   {
      expRec.push( record[i] );
   }
   return expRec;
}