/***************************************************************************
@Description :排序字段与索引字段不匹配时, 如果索引字段是等于操作, 仍可以保留排列有序.
              When the Sort field doesn't match the Index field, if the Index field is a
              equal operation, Still can keep the data list in a well-ordered.
@Author : 20120-08-10   Zixian Yan
****************************************************************************/
testConf.clName = COMMCLNAME + "_22423";
main( test );

function test ( testPara )
{
   var dbCL = testPara.testCL;

   var tableScan = "tableScan";
   var indexScan = "indexScan";

   commCreateIndex( dbCL, "abc", { a: 1, b: 1, c: 1 } );

   //Task 1: sortConditon field included in findCondition field
   /***Table Scan***/
   checkResult( dbCL, { a: 1, c: 1 }, { a: 1, c: 1 }, tableScan, false );
   checkResult( dbCL, { a: 1, b: 1, d: 1 }, { a: 1, b: 1, d: 1 }, tableScan, false );

   /***Index Scan***/
   checkResult( dbCL, { a: 1, c: 1 }, { a: 1, c: 1 }, indexScan, false );
   checkResult( dbCL, { a: 1, b: 1, d: 1 }, { a: 1, b: 1, d: 1 }, indexScan, false );


   //Task 2: findCondition field included in sortConditon field
   /***Table Scan***/
   checkResult( dbCL, { a: 1 }, { a: 1, c: 1 }, tableScan, true );
   checkResult( dbCL, { b: 1 }, { b: 1, c: 1 }, tableScan, true );

   /***Index Scan***/
   checkResult( dbCL, { a: 1 }, { a: 1, c: 1 }, indexScan, true );
   checkResult( dbCL, { b: 1 }, { b: 1, c: 1 }, indexScan, true );


   //Task 3: Field of findCondition and sortConditon are part of same, And both are index field;
   /***Table Scan***/
   checkResult( dbCL, { a: 1, b: 1 }, { a: 1, c: 1 }, tableScan, true );
   /***Index Scan***/
   checkResult( dbCL, { a: 1, b: 1 }, { a: 1, c: 1 }, indexScan, false );

   //Task 4: Field of findCondition and sortConditon are part of same;
   //Also findCondition contains non-indexed field, sortConditon is not contain non-indexed field.
   /***Table Scan***/
   checkResult( dbCL, { a: 1, d: 1 }, { a: 1, b: 1 }, tableScan, true );
   checkResult( dbCL, { a: 1, d: 1 }, { a: 1, c: 1 }, tableScan, true );
   /***Index Scan***/
   checkResult( dbCL, { a: 1, d: 1 }, { a: 1, b: 1 }, indexScan, false );
   checkResult( dbCL, { a: 1, d: 1 }, { a: 1, c: 1 }, indexScan, true );

   //Task 5: Field of findCondition and sortConditon are part of same;
   //Also sortConditon contains non-indexed field, findCondition is not contain non-indexed field.
   /***Table Scan***/
   checkResult( dbCL, { a: 1, c: 1 }, { a: 1, d: 1 }, tableScan, true );
   /***Index Scan***/
   checkResult( dbCL, { a: 1, c: 1 }, { a: 1, d: 1 }, indexScan, true );
}

function checkResult ( dbCL, findCond, sortCond, scanType, expectResult )
{
   if( scanType == "tableScan" )
   {
      scanType = { "": null };
   }
   else
   {
      scanType = { "": "abc" };
   }

   var actuallyResult = dbCL.find( findCond ).sort( sortCond ).hint( scanType ).explain().current().toObj()["UseExtSort"];

   assert.equal( actuallyResult, expectResult );
}
