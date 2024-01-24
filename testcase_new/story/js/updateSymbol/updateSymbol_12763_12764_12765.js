/************************************
*@Description: seqDB-12763:update使用pull_all_by更新普通数组对象
               seqDB-12764:update使用pull_all_by更新含嵌套数组的数组对象
               seqDB-12765:update使用pull_all_by更新含嵌套对象的数组对象
*@author:      liuxiaoxuan
*@createdate:  2017.09.19
**************************************/
testConf.clName = COMMCLNAME + "_pull_all_by_12763";
main( test );

function test ( testPara )
{
   //create index
   var indexName = 'b';
   var key = { b: 1 }
   testPara.testCL.createIndex( indexName, key );

   //insert data   
   var doc = [{ a1: [1, 2, 3, 0, -2, -1], b1: [10, 20, 30, -30, -10] },
   { a2: ['a', 'b', 'c', 'd', 'e'], b2: ['w', 'x', 'y', 'z'] },
   { a3: [100, [-100, 0, -200], [10, 20], 200], b3: [[-100, 0], [-200, -1, 1], 100, 200] },
   { a4: [[-300, [0, -100, 300]], [100, 200]], b4: [0, [100, [-300, 200, 300]], [-100, -200]] },
   { a5: [{ obj1: 1, obj2: 'testa51' }, { obj1: 2, obj2: 'testa52' }, { obj1: 3 }] },
   { a6: [{ obj1: 1, obj2: 'testa61' }, { obj1: 2, obj2: 'testa62' }, { obj1: 3, obj2: 'testa63' }] },
   { a7: [{ obj1: 1, obj2: 'testa71', obj3: 'ta7obj3aa' }, { obj1: 1, obj2: 'testa71', obj3: 'ta7obj3bb' }, { obj1: 2, obj2: 'testa72' }, { obj1: 3, obj3: 'ta7obj3cc' }] },
   { a8: [{ obj1: -1, obj2: 'testa81', obj3: 'ta8obj3dd' }, { obj2: 'testa82', obj1: 0, obj3: 'ta8obj3ee' }, { obj3: 'ta8obj3ff', obj2: 'testa83', obj1: 1 }] },
   { a9: [{ obj1: { obj2: 1 }, obj3: 'ta9obj3gg' }, { obj1: { obj2: 2 }, obj3: 'ta9obj3hh' }, { obj1: { obj2: 3 } }] },
   { a10: [{ obj1: { obj2_1: 1, obj2_2: -2 } }, { obj1: { obj2_1: 1, obj2_2: 0 } }, { obj1: { obj2_1: 0, obj2_2: -1 } }] },
   { a11: [{ obj1: { obj2: { obj3: 1 } }, obj4: 'ta11obj4ii' }, { obj1: { obj2: { obj3: 1 } } }, { obj1: { obj2: { obj3: 2 } }, obj4: 'ta11obj4jj' }] },
   { a12: [{ obj1: { obj2: { obj3_1: -2, obj3_2: 0 } } }, { obj1: { obj2: { obj3_1: -1, obj3_2: 2 } } }, { obj1: { obj2: { obj3_1: -2 } } }] },
   { b5: [{ obj1: -1, obj2: 'testb51' }, { obj1: -2, obj2: 'testb52' }, { obj1: -3 }] },
   { b6: [{ obj1: 1, obj2: 'testb61' }, { obj1: 2, obj2: 'testb62' }, { obj1: 3, obj2: 'testb63' }] },
   { b7: [{ obj2: 'testb71', obj1: -1 }, { obj1: -2, obj2: 'testb72' }, { obj1: -2 }, { obj2: 'testb73' }] },
   { b8: [{ obj1: -1, obj2: 'testb81', obj3: 'tb8obj3aa' }, { obj1: -1, obj2: 'testb81', obj3: 'tb8obj3bb' }, { obj1: -1, obj2: 'testb82' }] },
   { b9: [{ obj1: -1, obj2: 'testb91', obj3: 'tb9obj3cc' }, { obj2: 'testb92', obj1: 0, obj3: 'tb9obj3dd' }, { obj3: 'tb9obj3ee', obj2: 'testb93', obj1: 1 }, { obj1: 100 }, { obj2: 'testb99' }, { obj3: 'tbtbtbtbobjboj3' }] },
   { b10: [{ obj1: { obj2: 1 }, obj3: 'tb10obj3ff' }, { obj1: { obj2: 2 } }, { obj1: { obj2: 3 } }] },
   { b11: [{ obj3: 'tb11obj3hh', obj1: { obj2: 1 } }, { obj1: { obj2: 2 }, obj3: 'tb11obj3ii' }, { obj1: { obj2: 3 }, obj3: 'tb11obj3jj' }] },
   { b12: [{ obj1: { obj2_1: -1, obj2_2: -2 } }, { obj1: { obj2_1: 0, obj2_2: 1 } }, { obj1: { obj2_1: 1, obj2_2: 2 } }] },
   { b13: [{ obj1: { obj2: { obj3: 1 } }, obj4: 'tb13obj4aa' }, { obj1: { obj2: { obj3: 2 } }, obj4: 'tb13obj4bb' }, { obj1: { obj2: { obj3: 2 } } }] }];
   testPara.testCL.insert( doc );

   //pull_all_by , without match
   var updateRule1 = {
      $pull_all_by: {
         a1: [-100, -200], a2: ['abc', 'def'],
         'a3.0': [1, 2],
         'a4.0.1': [-10, -20],
         a5: [{ obj1: -1 }, { obj1: -2 }],
         a6: [{ obj1: -2 }, { obj2: 'aobj2' }],
         a7: [{ obj1: 1, obj2: 'testa72' }, { obj1: 2, obj2: 'testa71' }],
         b1: [-2, -1], b2: ['b21', 'b22'],
         'b3.1': [-20, -30],
         'b4.1.1': [10, 20],
         b5: [{ obj1: 0 }, { obj1: 1 }],
         b6: [{ obj1: 101 }, { obj2: 'bobj2' }],
         b7: [{ obj1: -1, obj2: 'testb72' }, { obj1: -2, obj2: 'testb71' }]
      }
   };
   testPara.testCL.update( updateRule1 );
   //check result
   var expResult1 = [{ a1: [1, 2, 3, 0, -2, -1], b1: [10, 20, 30, -30, -10] },
   { a2: ['a', 'b', 'c', 'd', 'e'], b2: ['w', 'x', 'y', 'z'] },
   { a3: [100, [-100, 0, -200], [10, 20], 200], b3: [[-100, 0], [-200, -1, 1], 100, 200] },
   { a4: [[-300, [0, -100, 300]], [100, 200]], b4: [0, [100, [-300, 200, 300]], [-100, -200]] },
   { a5: [{ obj1: 1, obj2: 'testa51' }, { obj1: 2, obj2: 'testa52' }, { obj1: 3 }] },
   { a6: [{ obj1: 1, obj2: 'testa61' }, { obj1: 2, obj2: 'testa62' }, { obj1: 3, obj2: 'testa63' }] },
   { a7: [{ obj1: 1, obj2: 'testa71', obj3: 'ta7obj3aa' }, { obj1: 1, obj2: 'testa71', obj3: 'ta7obj3bb' }, { obj1: 2, obj2: 'testa72' }, { obj1: 3, obj3: 'ta7obj3cc' }] },
   { a8: [{ obj1: -1, obj2: 'testa81', obj3: 'ta8obj3dd' }, { obj2: 'testa82', obj1: 0, obj3: 'ta8obj3ee' }, { obj3: 'ta8obj3ff', obj2: 'testa83', obj1: 1 }] },
   { a9: [{ obj1: { obj2: 1 }, obj3: 'ta9obj3gg' }, { obj1: { obj2: 2 }, obj3: 'ta9obj3hh' }, { obj1: { obj2: 3 } }] },
   { a10: [{ obj1: { obj2_1: 1, obj2_2: -2 } }, { obj1: { obj2_1: 1, obj2_2: 0 } }, { obj1: { obj2_1: 0, obj2_2: -1 } }] },
   { a11: [{ obj1: { obj2: { obj3: 1 } }, obj4: 'ta11obj4ii' }, { obj1: { obj2: { obj3: 1 } } }, { obj1: { obj2: { obj3: 2 } }, obj4: 'ta11obj4jj' }] },
   { a12: [{ obj1: { obj2: { obj3_1: -2, obj3_2: 0 } } }, { obj1: { obj2: { obj3_1: -1, obj3_2: 2 } } }, { obj1: { obj2: { obj3_1: -2 } } }] },
   { b5: [{ obj1: -1, obj2: 'testb51' }, { obj1: -2, obj2: 'testb52' }, { obj1: -3 }] },
   { b6: [{ obj1: 1, obj2: 'testb61' }, { obj1: 2, obj2: 'testb62' }, { obj1: 3, obj2: 'testb63' }] },
   { b7: [{ obj2: 'testb71', obj1: -1 }, { obj1: -2, obj2: 'testb72' }, { obj1: -2 }, { obj2: 'testb73' }] },
   { b8: [{ obj1: -1, obj2: 'testb81', obj3: 'tb8obj3aa' }, { obj1: -1, obj2: 'testb81', obj3: 'tb8obj3bb' }, { obj1: -1, obj2: 'testb82' }] },
   { b9: [{ obj1: -1, obj2: 'testb91', obj3: 'tb9obj3cc' }, { obj2: 'testb92', obj1: 0, obj3: 'tb9obj3dd' }, { obj3: 'tb9obj3ee', obj2: 'testb93', obj1: 1 }, { obj1: 100 }, { obj2: 'testb99' }, { obj3: 'tbtbtbtbobjboj3' }] },
   { b10: [{ obj1: { obj2: 1 }, obj3: 'tb10obj3ff' }, { obj1: { obj2: 2 } }, { obj1: { obj2: 3 } }] },
   { b11: [{ obj3: 'tb11obj3hh', obj1: { obj2: 1 } }, { obj1: { obj2: 2 }, obj3: 'tb11obj3ii' }, { obj1: { obj2: 3 }, obj3: 'tb11obj3jj' }] },
   { b12: [{ obj1: { obj2_1: -1, obj2_2: -2 } }, { obj1: { obj2_1: 0, obj2_2: 1 } }, { obj1: { obj2_1: 1, obj2_2: 2 } }] },
   { b13: [{ obj1: { obj2: { obj3: 1 } }, obj4: 'tb13obj4aa' }, { obj1: { obj2: { obj3: 2 } }, obj4: 'tb13obj4bb' }, { obj1: { obj2: { obj3: 2 } } }] }];
   checkResult( testPara.testCL, null, null, expResult1, { _id: 1 } );

   //pull_all_by , part of / all match
   var updateRule2 = {
      $pull_all_by: {
         a1: [-1, 2], a2: ['a', 'd', 'e'],
         'a3.1': [-100, -200],
         'a4.0.1': [0, -100],
         a5: [{ obj1: 1 }, { obj1: 2 }],
         a6: [{ obj1: 1 }, { obj2: 'testa63' }],
         a7: [{ obj1: 1, obj2: 'testa71' }, { obj3: 'ta7obj3cc' }],
         a8: [{ obj1: 0, obj2: 'testa81', obj3: 'ta8obj3ff' }],
         a9: [{ obj1: { obj2: 1 } }, { obj1: { obj2: 2 } }],
         a10: [{ obj1: { obj2_1: 0 } }, { obj1: { obj2_1: 1 } }],
         a11: [{ obj1: { obj2: { obj3: 1 } } }, { obj4: 'obj4' }],
         a12: [{ obj1: { obj2: { obj3_1: -2 } } }, { obj1: { obj2: { obj3_2: 2 } } }],
         b1: [-10, 20], b2: ['x', 'y'],
         'b3.1': [-1, 1],
         'b4.1.1': [200, 300],
         b5: [{ obj1: -1 }, { obj1: -2 }],
         b6: [{ obj1: 1, obj2: 'testb61' }, { obj1: 2, obj2: 'testb62' }],
         b7: [{ obj1: -2 }, { obj2: 'testb72' }],
         b8: [{ obj1: -1, obj2: 'testb81' }, { obj3: 'obj3' }],
         b9: [{ obj1: -1 }, { obj2: 'testb92' }, { obj3: 'tb9obj3ee' }],
         b10: [{ obj1: { obj2: 1 } }, { obj1: { obj2: 2 } }],
         b11: [{ obj1: { obj2: 1 } }, { obj3: 'tb11obj3ii' }],
         b12: [{ obj1: { obj2_1: -1 } }, { obj1: { obj2_2: 1 } }],
         b13: [{ obj1: { obj2: { obj3: 1 } }, obj4: 'tb13obj4aa' }, { obj4: 'obj4' }]
      }
   };
   testPara.testCL.update( updateRule2 );
   //check result
   var expResult2 = [{ a1: [1, 3, 0, -2], b1: [10, 30, -30] },
   { a2: ['b', 'c'], b2: ['w', 'z'] },
   { a3: [100, [0], [10, 20], 200], b3: [[-100, 0], [-200], 100, 200] },
   { a4: [[-300, [300]], [100, 200]], b4: [0, [100, [-300]], [-100, -200]] },
   { a5: [{ obj1: 3 }] },
   { a6: [{ obj1: 2, obj2: 'testa62' }] },
   { a7: [{ obj1: 2, obj2: 'testa72' }] },
   { a8: [{ obj1: -1, obj2: 'testa81', obj3: 'ta8obj3dd' }, { obj2: 'testa82', obj1: 0, obj3: 'ta8obj3ee' }, { obj3: 'ta8obj3ff', obj2: 'testa83', obj1: 1 }] },
   { a9: [{ obj1: { obj2: 3 } }] },
   { a10: [{ obj1: { obj2_1: 1, obj2_2: -2 } }, { obj1: { obj2_1: 1, obj2_2: 0 } }, { obj1: { obj2_1: 0, obj2_2: -1 } }] },
   { a11: [{ obj1: { obj2: { obj3: 2 } }, obj4: 'ta11obj4jj' }] },
   { a12: [{ obj1: { obj2: { obj3_1: -2, obj3_2: 0 } } }, { obj1: { obj2: { obj3_1: -1, obj3_2: 2 } } }] },
   { b5: [{ obj1: -3 }] },
   { b6: [{ obj1: 3, obj2: 'testb63' }] },
   { b7: [{ obj2: 'testb71', obj1: -1 }, { obj2: 'testb73' }] },
   { b8: [{ obj1: -1, obj2: 'testb82' }] },
   { b9: [{ obj1: 100 }, { obj2: 'testb99' }, { obj3: 'tbtbtbtbobjboj3' }] },
   { b10: [{ obj1: { obj2: 3 } }] },
   { b11: [{ obj1: { obj2: 3 }, obj3: 'tb11obj3jj' }] },
   { b12: [{ obj1: { obj2_1: -1, obj2_2: -2 } }, { obj1: { obj2_1: 0, obj2_2: 1 } }, { obj1: { obj2_1: 1, obj2_2: 2 } }] },
   { b13: [{ obj1: { obj2: { obj3: 2 } }, obj4: 'tb13obj4bb' }, { obj1: { obj2: { obj3: 2 } } }] }];
   checkResult( testPara.testCL, null, null, expResult2, { _id: 1 } );
}

