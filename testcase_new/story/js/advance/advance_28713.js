/******************************************************************************
 * @Description   : seqDB-28713:游标advance,IndexValue指定嵌套字段检查内存泄漏
 * @Author        : HuangHaimei
 * @CreateTime    : 2022.11.25
 * @LastEditTime  : 2022.11.29
 * @LastEditors   : HuangHaimei
 ******************************************************************************/
testConf.skipStandAlone = true;

main( test );
function test ( testPara )
{
   var csName = "cs_28713";
   var mainCLName = "mainCL_28713";
   var subCLName1 = "subCL_28713_1";
   var subCLName2 = "subCL_28713_2";
   var subCLName3 = "subCL_28713_3";
   var groupName1 = testPara.groups[0][0].GroupName;

   commDropCS( db, csName );
   var maincl = commCreateCL( db, csName, mainCLName, { ShardingKey: { b: 1 }, IsMainCL: true } );
   commCreateCL( db, csName, subCLName1, { Group: groupName1 } );
   commCreateCL( db, csName, subCLName2, { Group: groupName1 } );
   commCreateCL( db, csName, subCLName3, { Group: groupName1 } );
   maincl.attachCL( csName + "." + subCLName1, { LowBound: { b: 1 }, UpBound: { b: 2 } } );
   maincl.attachCL( csName + "." + subCLName2, { LowBound: { b: 2 }, UpBound: { b: 3 } } );
   maincl.attachCL( csName + "." + subCLName3, { LowBound: { b: 3 }, UpBound: { b: 4 } } );

   var docs = [];
   for( var i = 0; i < 100; i++ )
   {
      for( var j = 0; j < 1000; j++ )
      {
         docs.push( { a: i * 1000, b: i % 3 + 1 } );
      }
   }

   maincl.insert( docs );

   var recsNum = 50;
   for( var i = 0; i < recsNum; i++ )
   {
      docs.push( { a: 20, b: 2 } );
      docs.push( { a: 80, b: 2 } );
      docs.push( { a: 30, b: 3 } );
      docs.push( { a: 70, b: 3 } );
   }
   maincl.insert( docs );

   var cursor = maincl.find().sort( { a: 1 } ).hint( { "": "a" } );
   cursor.next();
   cursor._cursor.advance( { "IndexValue": { "a": recsNum }, "Type": 1, "PrefixNum": 1 } );
   var expResult1 = [70, 3];
   cursor.next();
   var actResult1 = [cursor.next().toObj()["a"], cursor.next().toObj()["b"]];
   assert.equal( actResult1, expResult1 );
   var expResult2 = [80, 2];
   for( i = 0; i < 50; i++ ) { cursor.next() }
   var actResult2 = [cursor.next().toObj()["a"], cursor.next().toObj()["b"]];
   assert.equal( actResult2, expResult2 );
   cursor.close();
}