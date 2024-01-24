/************************************
*@Description:  seqDB-8002:匹配不到记录，upsert使用pull更新符更新数组对象
*@author:      zhaoyu
*@createdate:  2016.5.19
**************************************/
testConf.clName = COMMCLNAME + "_pull8002";
main( test );

function test ( testPara )
{
   //insert object
   var doc = { a: 1 };
   testPara.testCL.insert( doc );

   //upsert any object when match nothing,use matches and
   var upsertCondition1 = {
      $pull: {
         object1: 40, object2: -8, object3: 0,
         object4: -10, object5: 1,
         object6: 2, object7: "string", object8: 0,
         "object9.1": 30, "object10.1": [30, 50, [90, 40], 80], "object11.0": 1,
         "object12.1.2": 6, "object13.1.2": 40,
         object14: 15, object15: 16
      }
   };
   var findCondition1 = {
      $and: [{ object1: [50, [30, 50, [90, 40], 80], 25, 40, 15] },
      { object2: [5, 7, 9, 2, -8, 4] },
      { object3: [3, 7, 0] },
      { object4: [5, [9, 8, 4, 3], 10] },
      { object5: [5, [9, 8, 4, 3], 10] },
      { object6: { $et: { $decimal: "2" } } },
      { object7: "string" },
      { object8: { $et: { $numberLong: "2" } } },
      { object9: { $all: [50, [30, 50, [90, 40], 80], 25, 40, 15] } },
      { object10: { $all: [50, [30, 50, [90, 40], 80], 25, 40, 15] } },
      { object11: [5, [9, 8, 4, 3], 10] },
      { object12: [5, [9, 8, 4, 3], 10] },
      { object13: [50, [30, 50, [90, 40], 80], 25, 40, 15] },
      { object17: [50, [30, 50, [90, 40], 80], 25, 40, 15] },
      { c: { $gt: 100 } },
      { d: 1000 }]
   };
   testPara.testCL.upsert( upsertCondition1, findCondition1 );

   //check result
   var expRecs1 = [{
      object1: [50, [30, 50, [90, 40], 80], 25, 15],
      object2: [5, 7, 9, 2, 4],
      object3: [3, 7],
      object4: [5, [9, 8, 4, 3], 10],
      object5: [5, [9, 8, 4, 3], 10],
      object6: { $decimal: "2" },
      object7: "string",
      object8: 2,
      object9: [50, [50, [90, 40], 80], 25, 40, 15],
      object10: [50, [30, 50, [90, 40], 80], 25, 40, 15],
      object11: [5, [9, 8, 4, 3], 10],
      object12: [5, [9, 8, 4, 3], 10],
      object13: [50, [30, 50, [90], 80], 25, 40, 15],
      object17: [50, [30, 50, [90, 40], 80], 25, 40, 15],
      d: 1000
   },
   { a: 1 }];
   checkResult( testPara.testCL, null, null, expRecs1, { a: 1 } );

   //upsert any object when match nothing,use matches or
   /*var upsertCondition2 = {$pull:{object1:40,object2:-8,object3:0,
                                 object4:-10,object5:1,
                                 object6:2,object7:"string",object8:0,
                                 "object9.1":30,"object10.1":[30,50,[90,40],80],"object11.0":1,
                                 "object12.1.2":6,"object13.1.2":40,
                                 object14:15,object15:16}};
   var findCondition2 = {$or:[{object1:[50,[30,50,[90,40],80],25,40,15]},
                              {object2:[5,7,9,2,-8,4]},
                              {object3:[3,7,0]},
                              {object4:[5,[9,8,4,3],10,11]},
                              {object5:[5,[9,8,4,3],10,11]},
                              {object6:{$et:3}},
                              {object7:"string1"},
                              {object8:{$et:{$numberLong:"3"}}},
                              {object9:{$all:[50,[30,50,[90,40],80],25,40,15]}},
                              {object10:{$all:[50,[30,50,[90,40],80],25,40,15,70]}},
                              {object11:[5,[9,8,4,3,14],10]},
                              {object12:[4,5,[9,8,4,3],10]},
                              {object13:[50,[30,50,[90,40],80],25,40,15]},
                              {object17:[50,[30,70,50,[90,40],80],25,40,15]},
                              {c:{$gt:100}},
                              {d:10000}]};
   testPara.testCL.upsert( upsertCondition2, findCondition2 );
   
   //check result
   var expRecs2 = [{object1:[50,[30,50,[90,40],80],25,15],
                    object2:[5,7,9,2,4],
                    object3:[3,7],
                    object4:[5,[9,8,4,3],10],
                    object5:[5,[9,8,4,3],10],
                    object6:{$decimal:"2"},
                    object7:"string",
                    object8:2,
                    object9:[50,[50,[90,40],80],25,40,15],
                    object10:[50,[30,50,[90,40],80],25,40,15],
                    object11:[5,[9,8,4,3],10],
                    object12:[5,[9,8,4,3],10],
                    object13:[50,[30,50,[90],80],25,40,15],
                    object17:[50,[30,50,[90,40],80],25,40,15],
                    d:1000},
                   {a:1}];
   checkResult( testPara.testCL, null, null, expRecs2, {a:1} );
   
   //delete all data
  testPara.testCL.remove();
   
   //upsert any object when match nothing,use matches not
   var upsertCondition3 = {$pull:{object1:40,object2:-8,object3:0,
                              object4:-10,object5:1,
                              object6:2,object7:"string",object8:0,
                              "object9.1":30,"object10.1":[30,50,[90,40],80],"object11.0":1,
                              "object12.1.2":6,"object13.1.2":40,
                              object14:15,object15:16}};
   var findCondition3 = {$not:[{object1:[50,[30,50,[90,40],80],25,40,15]},
                               {object2:[5,7,9,2,-8,4]},
                               {object3:[3,7,0]},
                               {object4:[5,[9,8,4,3],10,11]},
                               {object5:[5,[9,8,4,3],10,11]},
                               {object6:{$et:3}},
                               {object7:"string1"},
                               {object8:{$et:{$numberLong:"3"}}},
                               {object9:{$all:[50,[30,50,[90,40],80],25,40,15]}},
                               {object10:{$all:[50,[30,50,[90,40],80],25,40,15,70]}},
                               {object11:[5,[9,8,4,3,14],10]},
                               {object12:[4,5,[9,8,4,3],10]},
                               {object13:[50,[30,50,[90,40],80],25,40,15]},
                               {object17:[50,[30,70,50,[90,40],80],25,40,15]},
                               {c:{$gt:100}},
                               {d:10000}]};
   testPara.testCL.upsert( upsertCondition3, findCondition3 );
   
   check result
   var expRecs3 = [];
   checkResult( testPara.testCL, null, null, expRecs3, {a:1} );*/
}

