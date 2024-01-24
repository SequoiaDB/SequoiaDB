/******************************************************************************
@Description: [seqDB-20267] Query by $not with index;
              使用$not查询, 走索引查询
@Author: 2020/08/06 Zixian Yan
******************************************************************************/
testConf.clName = COMMCLNAME + "_20267";
main( test );

function test ( testPara )
{
   var cl = testPara.testCL;

   var data = [{ number: 7 }, { number: 5 }, { number: 3 }, { number: 6 },
   { number: 4 }, { number: 8 }, { number: 2 }, { number: 1 }]

   cl.insert( data );

   commCreateIndex( cl, "numIndex", { number: 1 } );

   var rc1 = cl.find( { $not: [{ number: { $lt: 5 } }] } );
   var rc2 = cl.find( { $not: [{ number: { $lt: 5 } }] } ).sort( { number: 1 } );

   var expectationOne = [{ number: 7 },
   { number: 5 },
   { number: 6 },
   { number: 8 }];

   var expectationTwo = [{ number: 5 },
   { number: 6 },
   { number: 7 },
   { number: 8 }];

   checkRec( rc1, expectationOne );
   checkRec( rc2, expectationTwo );
}
