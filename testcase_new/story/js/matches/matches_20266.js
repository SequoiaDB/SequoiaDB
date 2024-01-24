/******************************************************************************
@Description: [seqDB-20266] Query by $nin with index
              使用$nin查询, 走索引查询
@Author: 2020/08/06 Zixian Yan
******************************************************************************/
testConf.clName = COMMCLNAME + "_20266";
main( test );

function test ( testPara )
{
   var cl = testPara.testCL;
   var data = [];
   for( var x = 6; x >= 1; x-- )
   {
      data.push( { number: x } );
   }
   cl.insert( data );

   commCreateIndex( cl, "numIndex", { number: 1 } );

   var rc1 = cl.find( { number: { $nin: [3, 4] } } );
   var rc2 = cl.find( { number: { $nin: [3, 4] } } ).sort( { number: 1 } );

   var expectationOne = [{ number: 6 },
   { number: 5 },
   { number: 2 },
   { number: 1 }];

   var expectationTwo = [{ number: 1 },
   { number: 2 },
   { number: 5 },
   { number: 6 }];

   checkRec( rc1, expectationOne );
   checkRec( rc2, expectationTwo );
}
