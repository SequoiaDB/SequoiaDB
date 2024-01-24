/************************************
*@Description: seqDB-8005:匹配不到记录，upsert使用pull_all更新符更新数组对象
*@author:      zhaoyu
*@createdate:  2016.5.19
**************************************/
testConf.clName = COMMCLNAME + "_pull_all8005";
main( test );

function test ( testPara )
{
   //insert object
   var doc = { a: 1 };
   testPara.testCL.insert( doc );

   //upsert any object when match nothing,use matches and
   var upsertCondition1 = {
      $pull_all: {
         object1: [40, 50, 25], object2: [-8, 100, 9, 200], object3: [13, 17],
         object6: [2, 3],
         "object9.1": [30, 80], "object10.1": [30, 50, [90, 40, 5], 90], "object11.1": [1, 2],
         "object12.1.2": [6, 10],
         object14: [15, 16]
      }
   };
   var findCondition1 = {
      $and: [{ object1: [50, [30, 50, [90, 40], 80], 25, 40, 15] },
      { object2: [5, 7, 9, 2, -8, 4] },
      { object3: [3, 7, 0] },
      { object6: { $et: { $decimal: "2" } } },
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
      object1: [[30, 50, [90, 40], 80], 15],
      object2: [5, 7, 2, 4],
      object3: [3, 7, 0],
      object6: { $decimal: "2" },
      object9: [50, [50, [90, 40]], 25, 40, 15],
      object10: [50, [[90, 40], 80], 25, 40, 15],
      object11: [5, [9, 8, 4, 3], 10],
      object12: [5, [9, 8, 4, 3], 10],
      object13: [50, [30, 50, [90, 40], 80], 25, 40, 15],
      object17: [50, [30, 50, [90, 40], 80], 25, 40, 15],
      d: 1000
   },
   { a: 1 }];
   checkResult( testPara.testCL, null, null, expRecs1, { a: 1 } );

   //upsert any object when match nothing,use matches or
   /*var upsertCondition2 = {$pull_all:{object1:[40,50,25],object2:[-8,100,9,200],object3:[13,17],
                                    object6:[2,3],
                                    "object9.1":[30,80],"object10.1":[30,50,[90,40,5],90],"object11.1":[1,2],
                                    "object12.1.2":[6,10],
                                    object14:[15,16]}};
   var findCondition2 = {$or:[{object1:[50,[30,50,[90,40],80],25,40,15]},
                              {object2:[5,7,9,2,-8,4]},
                              {object3:[3,7,1]},
                              {object6:{$et:{$decimal:"5"}}},
                              {object9:{$all:[50,[30,50,[90,40],80],25,40,15]}},
                              {object10:{$all:[50,[30,50,[90,40],80],25,40,15]}},
                              {object11:[5,[9,8,4,3],11]},
                              {object12:[5,[9,8,4,3],11]},
                              {object13:[50,[30,50,[90,40],80],25,41,15]},
                              {object17:[50,[30,50,[90,40],80],25,41,15]},
                              {c:{$gt:100}},
                              {d:10000}]};
   testPara.testCL.upsert( upsertCondition2, findCondition2 );
   
   //check result
   var expRecs2 = [{object1:[[30,50,[90,40],80],15],
                    object2:[5,7,2,4],
                    object3:[3,7,0],
                    object6:{$decimal:"2"},
                    object9:[50,[50,[90,40]],25,40,15],
                    object10:[50,[[90,40],80],25,40,15],
                    object11:[5,[9,8,4,3],10],
                    object12:[5,[9,8,4,3],10],
                    object13:[50,[30,50,[90,40],80],25,40,15],
                    object17:[50,[30,50,[90,40],80],25,40,15],
                    d:1000},
                   {a:1}];
   checkResult( testPara.testCL, null, null, expRecs2, {a:1} );
   
   //delete all data
  testPara.testCL.remove();
   
   //upsert any object when match nothing,use matches not
   var upsertCondition3 = {$pull_all:{object1:[40,50,25],object2:[-8,100,9,200],object3:[13,17],
                                    object6:[2,3],
                                    "object9.1":[30,80],"object10.1":[30,50,[90,40,5],90],"object11.1":[1,2],
                                    "object12.1.2":[6,10],
                                    object14:[15,16]}};
   var findCondition3 = {$not:[{object1:[50,[30,50,[90,40],80],25,40,15]},
                              {object2:[5,7,9,2,-8,4]},
                              {object3:[3,7,1]},
                              {object6:{$et:{$decimal:"5"}}},
                              {object9:{$all:[50,[30,50,[90,40],80],25,40,15]}},
                              {object10:{$all:[50,[30,50,[90,40],80],25,40,15]}},
                              {object11:[5,[9,8,4,3],11]},
                              {object12:[5,[9,8,4,3],11]},
                              {object13:[50,[30,50,[90,40],80],25,41,15]},
                              {object17:[50,[30,50,[90,40],80],25,41,15]},
                              {c:{$gt:100}},
                              {d:10000}]};
   testPara.testCL.upsert( upsertCondition3, findCondition3 );
   
   //check result
   var expRecs3 = [];
   checkResult( testPara.testCL, null, null, expRecs3, {a:1} );*/
}

