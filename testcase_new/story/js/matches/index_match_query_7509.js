/*******************************************************************************
*@Description : create index and query. query match $gt/$gte/$lt/$lte/$ne
*                                                   $and/$not/$or
*@Modify list :
*               2014-5-20  xiaojun Hu  Init
                2020-08-13 Zixian Yan
*******************************************************************************/
testConf.clName = COMMCLNAME + "_7509";
main( test );

function test ( testPara )
{
   var insertNum = 10;
   var cl = testPara.testCL;
   // insert data
   idxAutoGenData( cl, insertNum );

   // create Index
   var idxName = "noIndex";
   var indexDef = { "no": 1, "no1": 1, "no2": -1, "no3": 1 };
   commCreateIndex( cl, idxName, indexDef );
   commCheckIndexConsistency( cl, idxName, true );

   // query data
   var queryCond = {
      $and: [{ "no": { $gt: 2 } }, { "no1": { $gte: 4 } },
      { "no2": { $lt: 18 } },
      { "array": { "$in": ["5arr5", 5] } },
      { "no3": { $lte: 28 } }]
   };
   idxQueryCheck( cl, queryCond, 1, idxName );

   var queryCond = {
      $and: [{ "no": { $gt: 3 } }, { "no1": { $gte: 4 } },
      { "no2": { $lt: 20 } },
      { "array": { "$nin": [25, 20] } },
      { "no3": { $lte: 28 } }]
   };
   idxQueryCheck( cl, queryCond, 1, idxName );

   var queryCond = {
      $not: [{ "no": { $gt: 7 } }, { "no1": { $gte: 10 } },
      { "no": { "$ne": 9 } },
      { "no2": { $lt: 30 } },
      { "no3": { $lte: 40 } }]
   };
   idxQueryCheck( cl, queryCond, 9, idxName );

   var queryCond = {
      $or: [{
         $and: [{ "no": { $gt: 6 } },
         { "no": { $lt: 10 } }]
      },
      { "array": { "$all": ["7arr7", 35, "14ARR7", "arrayIndex"] } },
      {
         $and: [{ "no1": { $gte: 0 } },
         { "no1": { $lte: 6 } }]
      }]
   };
   idxQueryCheck( cl, queryCond, 7, idxName );

}
