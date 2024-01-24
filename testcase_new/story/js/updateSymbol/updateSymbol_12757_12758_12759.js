/************************************
*@Description: seqDB-12757:update使用pull_by更新普通数组对象
               seqDB-12758:update使用pull_by更新含嵌套数组的数组对象
               seqDB-12759:update使用pull_by更新含嵌套对象的数组对象
*@author:      liuxiaoxuan
*@createdate:  2017.09.18
**************************************/

testConf.clName = COMMCLNAME + "_pull_by_12755";
main( test );

function test ( testPara )
{
   //create index
   var indexName = 'b';
   var key = { b: 1 }
   testPara.testCL.createIndex( indexName, key );

   //insert data   
   var doc = [{ a1: [1, 2, 3], b1: [10, 20, 30] },
   { a2: ['a', 'b', 'c', 'd', 'e'], b2: ['x', 'y', 'z'] },
   { a3: [100, [-100, -200], 200], b3: [[-100, 0], 100, 200] },
   { a4: [[-300, [0, -100, 300]], [100, 200]], b4: [0, [100, [-300, 200, 300]], [-100, -200]] },
   { a5: [{ obj1: 1, obj2: 'testa51' }, { obj1: 1 }, { obj1: 2 }] },
   { a6: [{ obj1: 1, obj2: 'testa61' }, { obj1: 2, obj2: 'testa62' }, { obj1: 3, obj2: 'testa63' }] },
   { a7: [{ obj2: 'testa71', obj1: -1 }, { obj1: -1, obj2: 'testa72' }, { obj1: 1 }] },
   { a8: [{ obj1: 1, obj2: 'testa81', obj3: 'ta8obj3aa' }, { obj1: 1, obj2: 'testa81', obj3: 'ta8obj3bb' }, { obj1: 1, obj2: 'testa82' }] },
   { a9: [{ obj1: -1, obj2: 'testa91', obj3: 'ta9obj3cc' }, { obj2: 'testa92', obj1: 0, obj3: 'ta9obj3dd' }, { obj3: 'ta9obj3ee', obj2: 'testa93', obj1: 1 }] },
   { a10: [{ obj1: { obj2: 1 }, obj3: 'ta10obj3ff' }, { obj1: { obj2: 1 } }, { obj1: { obj2: 2 }, obj3: 'ta10obj3gg' }] },
   { a11: [{ obj3: 'ta11obj3hh', obj1: { obj2: 1 } }, { obj1: { obj2: 1 }, obj3: 'ta11obj3ii' }, { obj1: { obj2: 2 } }, { obj1: { obj2: 2 }, obj3: 'ta11obj3jj' }] },
   { a12: [{ obj1: { obj2_1: 1, obj2_2: -2 } }, { obj1: { obj2_1: 1, obj2_2: 0 } }, { obj1: { obj2_1: 0, obj2_2: -1 } }] },
   { a13: [{ obj1: { obj2: { obj3: 1 } }, obj4: 'ta13obj4aa' }, { obj1: { obj2: { obj3: 1 } } }, { obj1: { obj2: { obj3: 2 } }, obj4: 'ta13obj4bb' }] },
   { a14: [{ obj1: { obj2: { obj3_1: -2, obj3_2: 0 } } }, { obj1: { obj2: { obj3_1: -2, obj3_2: 2 } } }, { obj1: { obj2: { obj3_1: -2 } } }] },
   { a15: [{ obj1: { obj2_1: { obj3_1: -1 }, obj2_2: { obj3_2: 1 } } }, { obj1: { obj2_1: { obj3_1: -1 }, obj2_2: { obj3_2: 2 } } }] },
   { b5: [{ obj1: -1, obj2: 'testb51' }, { obj1: -1 }, { obj1: -2 }] },
   { b6: [{ obj1: 1, obj2: 'testb61' }, { obj1: 2, obj2: 'testb62' }, { obj1: 3, obj2: 'testb63' }] },
   { b7: [{ obj2: 'testb71', obj1: -1 }, { obj1: -1, obj2: 'testb72' }, { obj1: 1 }] },
   { b8: [{ obj1: -1, obj2: 'testb81', obj3: 'tb8obj3aa' }, { obj1: -1, obj2: 'testb81', obj3: 'tb8obj3bb' }, { obj1: -1, obj2: 'testb82' }] },
   { b9: [{ obj1: -1, obj2: 'testb91', obj3: 'tb9obj3cc' }, { obj2: 'testb92', obj1: 0, obj3: 'tb9obj3dd' }, { obj3: 'tb9obj3ee', obj2: 'testb93', obj1: 1 }, { obj1: -1 }, { obj2: 'testb92' }, { obj3: 'tb9obj3ee' }] },
   { b10: [{ obj1: { obj2: 1 }, obj3: 'tb10obj3ff' }, { obj1: { obj2: 1 } }, { obj1: { obj2: 2 }, obj3: 'tb10obj3gg' }] },
   { b11: [{ obj3: 'tb11obj3hh', obj1: { obj2: 1 } }, { obj1: { obj2: 1 }, obj3: 'tb11obj3ii' }, { obj1: { obj2: 2 } }, { obj1: { obj2: 2 }, obj3: 'tb11obj3jj' }] },
   { b12: [{ obj1: { obj2_1: 1, obj2_2: -2 } }, { obj1: { obj2_1: 1, obj2_2: 0 } }, { obj1: { obj2_1: 0, obj2_2: -1 } }] },
   { b13: [{ obj1: { obj2: { obj3: 1 } }, obj4: 'tb13obj4aa' }, { obj1: { obj2: { obj3: 1 } } }, { obj1: { obj2: { obj3: 2 } }, obj4: 'tb13obj4bb' }] },
   { b14: [{ obj1: { obj2: { obj3_1: -2, obj3_2: 0 } } }, { obj1: { obj2: { obj3_1: -2, obj3_2: 2 } } }, { obj1: { obj2: { obj3_1: -2 } } }] },
   { b15: [{ obj1: { obj2_1: { obj3_1: -1 }, obj2_2: { obj3_2: 1 } } }, { obj1: { obj2_1: { obj3_1: -1 }, obj2_2: { obj3_2: 2 } } }] }];
   testPara.testCL.insert( doc );

   //pull_by , without match
   var updateRule1 = {
      $pull_by: {
         a1: 4, a2: 'f', a3: 300,
         a4: -300, a5: { obj1: 3 }, a6: { obj3: 'obj3' },
         b1: 40, b2: 'xx', b3: -100,
         b4: 300, b5: { obj1: 100 }, b6: { obj3: 'obj3' }
      }
   };
   testPara.testCL.update( updateRule1 );
   //check result
   var expResult1 = [{ a1: [1, 2, 3], b1: [10, 20, 30] },
   { a2: ['a', 'b', 'c', 'd', 'e'], b2: ['x', 'y', 'z'] },
   { a3: [100, [-100, -200], 200], b3: [[-100, 0], 100, 200] },
   { a4: [[-300, [0, -100, 300]], [100, 200]], b4: [0, [100, [-300, 200, 300]], [-100, -200]] },
   { a5: [{ obj1: 1, obj2: 'testa51' }, { obj1: 1 }, { obj1: 2 }] },
   { a6: [{ obj1: 1, obj2: 'testa61' }, { obj1: 2, obj2: 'testa62' }, { obj1: 3, obj2: 'testa63' }] },
   { a7: [{ obj2: 'testa71', obj1: -1 }, { obj1: -1, obj2: 'testa72' }, { obj1: 1 }] },
   { a8: [{ obj1: 1, obj2: 'testa81', obj3: 'ta8obj3aa' }, { obj1: 1, obj2: 'testa81', obj3: 'ta8obj3bb' }, { obj1: 1, obj2: 'testa82' }] },
   { a9: [{ obj1: -1, obj2: 'testa91', obj3: 'ta9obj3cc' }, { obj2: 'testa92', obj1: 0, obj3: 'ta9obj3dd' }, { obj3: 'ta9obj3ee', obj2: 'testa93', obj1: 1 }] },
   { a10: [{ obj1: { obj2: 1 }, obj3: 'ta10obj3ff' }, { obj1: { obj2: 1 } }, { obj1: { obj2: 2 }, obj3: 'ta10obj3gg' }] },
   { a11: [{ obj3: 'ta11obj3hh', obj1: { obj2: 1 } }, { obj1: { obj2: 1 }, obj3: 'ta11obj3ii' }, { obj1: { obj2: 2 } }, { obj1: { obj2: 2 }, obj3: 'ta11obj3jj' }] },
   { a12: [{ obj1: { obj2_1: 1, obj2_2: -2 } }, { obj1: { obj2_1: 1, obj2_2: 0 } }, { obj1: { obj2_1: 0, obj2_2: -1 } }] },
   { a13: [{ obj1: { obj2: { obj3: 1 } }, obj4: 'ta13obj4aa' }, { obj1: { obj2: { obj3: 1 } } }, { obj1: { obj2: { obj3: 2 } }, obj4: 'ta13obj4bb' }] },
   { a14: [{ obj1: { obj2: { obj3_1: -2, obj3_2: 0 } } }, { obj1: { obj2: { obj3_1: -2, obj3_2: 2 } } }, { obj1: { obj2: { obj3_1: -2 } } }] },
   { a15: [{ obj1: { obj2_1: { obj3_1: -1 }, obj2_2: { obj3_2: 1 } } }, { obj1: { obj2_1: { obj3_1: -1 }, obj2_2: { obj3_2: 2 } } }] },
   { b5: [{ obj1: -1, obj2: 'testb51' }, { obj1: -1 }, { obj1: -2 }] },
   { b6: [{ obj1: 1, obj2: 'testb61' }, { obj1: 2, obj2: 'testb62' }, { obj1: 3, obj2: 'testb63' }] },
   { b7: [{ obj2: 'testb71', obj1: -1 }, { obj1: -1, obj2: 'testb72' }, { obj1: 1 }] },
   { b8: [{ obj1: -1, obj2: 'testb81', obj3: 'tb8obj3aa' }, { obj1: -1, obj2: 'testb81', obj3: 'tb8obj3bb' }, { obj1: -1, obj2: 'testb82' }] },
   { b9: [{ obj1: -1, obj2: 'testb91', obj3: 'tb9obj3cc' }, { obj2: 'testb92', obj1: 0, obj3: 'tb9obj3dd' }, { obj3: 'tb9obj3ee', obj2: 'testb93', obj1: 1 }, { obj1: -1 }, { obj2: 'testb92' }, { obj3: 'tb9obj3ee' }] },
   { b10: [{ obj1: { obj2: 1 }, obj3: 'tb10obj3ff' }, { obj1: { obj2: 1 } }, { obj1: { obj2: 2 }, obj3: 'tb10obj3gg' }] },
   { b11: [{ obj3: 'tb11obj3hh', obj1: { obj2: 1 } }, { obj1: { obj2: 1 }, obj3: 'tb11obj3ii' }, { obj1: { obj2: 2 } }, { obj1: { obj2: 2 }, obj3: 'tb11obj3jj' }] },
   { b12: [{ obj1: { obj2_1: 1, obj2_2: -2 } }, { obj1: { obj2_1: 1, obj2_2: 0 } }, { obj1: { obj2_1: 0, obj2_2: -1 } }] },
   { b13: [{ obj1: { obj2: { obj3: 1 } }, obj4: 'tb13obj4aa' }, { obj1: { obj2: { obj3: 1 } } }, { obj1: { obj2: { obj3: 2 } }, obj4: 'tb13obj4bb' }] },
   { b14: [{ obj1: { obj2: { obj3_1: -2, obj3_2: 0 } } }, { obj1: { obj2: { obj3_1: -2, obj3_2: 2 } } }, { obj1: { obj2: { obj3_1: -2 } } }] },
   { b15: [{ obj1: { obj2_1: { obj3_1: -1 }, obj2_2: { obj3_2: 1 } } }, { obj1: { obj2_1: { obj3_1: -1 }, obj2_2: { obj3_2: 2 } } }] }];
   checkResult( testPara.testCL, null, null, expResult1, { _id: 1 } );

   //pull_by , part of / all match
   var updateRule2 = {
      $pull_by: {
         a1: 1, a2: 'b',
         a3: [-100, -200],
         'a4.0.1': 300, a5: { obj1: 1 },
         a6: { obj1: 1, obj2: 'testa62' },
         a7: { obj1: -1, obj2: 'testa71' },
         a8: { obj1: 1, obj2: 'testa81' },
         a9: { obj1: -1, obj2: 'testa92', obj3: 'ta9obj3ee' },
         a10: { obj1: { obj2: 1 } },
         a11: { obj1: { obj2: 2 }, obj3: 'ta11obj3jj' },
         a12: { obj1: { obj2_1: 1 } },
         a13: { obj1: { obj2: { obj3: 1 } } },
         a14: { obj1: { obj2: { obj3_1: -2 } } },
         a15: { obj1: { obj2_1: { obj3_1: -1 } } },
         b1: 10, b2: 'z',
         b3: [-100, 0], 'b4.2': -100,
         b5: { obj1: -1 },
         b6: { obj1: 2, obj2: 'testb63' },
         b7: { obj1: -1, obj2: 'testb72' },
         b8: { obj1: -1, obj2: 'testb81' },
         b9: { obj1: -1, obj2: 'testb92', obj3: 'tb9obj3ee' },
         b10: { obj1: { obj2: 1 } },
         b11: { obj1: { obj2: 2 }, obj3: 'tb11obj3jj' },
         b12: { obj1: { obj2_1: 1 } },
         b13: { obj1: { obj2: { obj3: 1 } } },
         b14: { obj1: { obj2: { obj3_1: -2 } } },
         b15: { obj1: { obj2_1: { obj3_1: -1 } } }
      }
   };
   testPara.testCL.update( updateRule2 );
   //check result
   var expResult2 = [{ a1: [2, 3], b1: [20, 30] },
   { a2: ['a', 'c', 'd', 'e'], b2: ['x', 'y'] },
   { a3: [100, 200], b3: [100, 200] },
   { a4: [[-300, [0, -100]], [100, 200]], b4: [0, [100, [-300, 200, 300]], [-200]] },
   { a5: [{ obj1: 2 }] },
   { a6: [{ obj1: 1, obj2: 'testa61' }, { obj1: 2, obj2: 'testa62' }, { obj1: 3, obj2: 'testa63' }] },
   { a7: [{ obj1: -1, obj2: 'testa72' }, { obj1: 1 }] },
   { a8: [{ obj1: 1, obj2: 'testa82' }] },
   { a9: [{ obj1: -1, obj2: 'testa91', obj3: 'ta9obj3cc' }, { obj2: 'testa92', obj1: 0, obj3: 'ta9obj3dd' }, { obj3: 'ta9obj3ee', obj2: 'testa93', obj1: 1 }] },
   { a10: [{ obj1: { obj2: 2 }, obj3: 'ta10obj3gg' }] },
   { a11: [{ obj3: 'ta11obj3hh', obj1: { obj2: 1 } }, { obj1: { obj2: 1 }, obj3: 'ta11obj3ii' }, { obj1: { obj2: 2 } }] },
   { a12: [{ obj1: { obj2_1: 1, obj2_2: -2 } }, { obj1: { obj2_1: 1, obj2_2: 0 } }, { obj1: { obj2_1: 0, obj2_2: -1 } }] },
   { a13: [{ obj1: { obj2: { obj3: 2 } }, obj4: 'ta13obj4bb' }] },
   { a14: [{ obj1: { obj2: { obj3_1: -2, obj3_2: 0 } } }, { obj1: { obj2: { obj3_1: -2, obj3_2: 2 } } }] },
   { a15: [{ obj1: { obj2_1: { obj3_1: -1 }, obj2_2: { obj3_2: 1 } } }, { obj1: { obj2_1: { obj3_1: -1 }, obj2_2: { obj3_2: 2 } } }] },
   { b5: [{ obj1: -2 }] },
   { b6: [{ obj1: 1, obj2: 'testb61' }, { obj1: 2, obj2: 'testb62' }, { obj1: 3, obj2: 'testb63' }] },
   { b7: [{ obj2: 'testb71', obj1: -1 }, { obj1: 1 }] },
   { b8: [{ obj1: -1, obj2: 'testb82' }] },
   { b9: [{ obj1: -1, obj2: 'testb91', obj3: 'tb9obj3cc' }, { obj2: 'testb92', obj1: 0, obj3: 'tb9obj3dd' }, { obj3: 'tb9obj3ee', obj2: 'testb93', obj1: 1 }, { obj1: -1 }, { obj2: 'testb92' }, { obj3: 'tb9obj3ee' }] },
   { b10: [{ obj1: { obj2: 2 }, obj3: 'tb10obj3gg' }] },
   { b11: [{ obj3: 'tb11obj3hh', obj1: { obj2: 1 } }, { obj1: { obj2: 1 }, obj3: 'tb11obj3ii' }, { obj1: { obj2: 2 } }] },
   { b12: [{ obj1: { obj2_1: 1, obj2_2: -2 } }, { obj1: { obj2_1: 1, obj2_2: 0 } }, { obj1: { obj2_1: 0, obj2_2: -1 } }] },
   { b13: [{ obj1: { obj2: { obj3: 2 } }, obj4: 'tb13obj4bb' }] },
   { b14: [{ obj1: { obj2: { obj3_1: -2, obj3_2: 0 } } }, { obj1: { obj2: { obj3_1: -2, obj3_2: 2 } } }] },
   { b15: [{ obj1: { obj2_1: { obj3_1: -1 }, obj2_2: { obj3_2: 1 } } }, { obj1: { obj2_1: { obj3_1: -1 }, obj2_2: { obj3_2: 2 } } }] }];
   checkResult( testPara.testCL, null, null, expResult2, { _id: 1 } );
}

