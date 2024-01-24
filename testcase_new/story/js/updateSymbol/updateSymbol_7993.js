/************************************
*@Description:  seqDB-7993:匹配不到记录，upsert使用unset更新符
*@author:      zhaoyu
*@createdate:  2016.5.17
**************************************/
testConf.clName = COMMCLNAME + "_unset7993";
main( test );

function test ( testPara )
{
   //insert object
   var doc = { a: 1 };
   testPara.testCL.insert( doc );

   //upsert any object when match nothing,use matches and
   var upsertCondition3 = {
      $unset: {
         object1: "",
         object13: "",
         object14: "",
         "object2.0": "",
         "e.name.midName": ""
      }
   };
   var findCondition3 = {
      $and: [{ object1: { $et: { $decimal: "2" } } },
      { object13: { $all: [10, 20, 30] } },
      { object2: { $all: [15, 25, 35] } },
      { c: { $gt: 100 } },
      { d: 1000 },
      { "e.name.firstName": "han", "e.name.lastName": "meimei", "e.name.midName": "test" }]
   };
   testPara.testCL.upsert( upsertCondition3, findCondition3 );

   //check result
   var expRecs3 = [{ d: 1000, e: { name: { firstName: "han", lastName: "meimei" } }, object2: [null, 25, 35] },
   { a: 1 }];
   checkResult( testPara.testCL, null, null, expRecs3, { a: 1 } );

   //upsert any object when match nothing,use matches or
   /*var upsertCondition4 = {$unset:{object1:"",
                                   object13:"",
                                   object14:"",
                                   "object2.1":"",
                                   "e.name.lastName":""}};
   var findCondition4 = {$or:[{object1:{$et:{$decimal:"2"}}},
                               {object13:{$all:[10,20,30]}},
                               {object2:{$all:[15,25,35]}},
                               {c:{$gt:100}},
                               {d:1001},
                               {"e.name.firstName":"han","e.name.lastName":"meimei","e.name.midName":"test"}]};
   testPara.testCL.upsert( upsertCondition4, findCondition4 );
   
   //check result
   var expRecs4 = [{d:1000,e:{name:{firstName:"han",lastName:"meimei"}},object2:[null,25,35]},
                   {a:1}];
   checkResult( testPara.testCL, null, null, expRecs4, {a:1} );
   
   //delete all data
  testPara.testCL.remove();
   
   //upsert any object when match nothing,use matches not
   var upsertCondition5 = {$unset:{object1:"",
                                   object13:"",
                                   object14:"",
                                   "object2.1":"",
                                   "e.name.lastName":""}};
   var findCondition5 = {$not:[{object1:{$et:{$decimal:"2"}}},
                               {object13:{$all:[10,20,30]}},
                               {object2:{$all:[15,25,35]}},
                               {c:{$gt:100}},
                               {d:1001},
                               {"e.name.firstName":"han","e.name.lastName":"meimei","e.name.midName":"test"}]};
   testPara.testCL.upsert( upsertCondition5, findCondition5 );
   
   //check result
   var expRecs5 = [{d:1000,e:{name:{firstName:"han",lastName:"meimei"}},object2:[null,25,35]},
                   {a:1}];
   checkResult( testPara.testCL, null, null, expRecs5, {a:1} );*/
}

