/************************************
*@Description: seqDB-7983:更新符inc的所有数值类型运算边界测试
*@author:      zhaoyu
*@createdate:  2016.5.16
**************************************/
testConf.clName = COMMCLNAME + "_inc7983";
main( test );

function test ( testPara )
{
   //insert numberic data 
   var doc1 = [{ a: -2147483648 },
   { a: 2147483647 },
   { a: { $numberLong: "-9223372036854775808" } },
   { a: { $numberLong: "9223372036854775807" } },
   { a: -1.7E+308 },
   { a: 1.7E+308 },
   { a: -4.9E-324 },
   { a: 4.9E-324 }];
   testPara.testCL.insert( doc1 );

   //update use $inc,result out of range
   var updateCondition1 = { $inc: { a: -1 } };
   testPara.testCL.update( updateCondition1 );
   var expRecs1 = [{ a: -2147483649 },
   { a: 2147483646 },
   { a: { $decimal: "-9223372036854775809" } },
   { a: { $numberLong: "9223372036854775806" } },
   { a: -1.7E+308 },
   { a: 1.7E+308 },
   { a: -1 },
   { a: -1 }];

   //check result
   var expRecsFindByType1 = [{ a: -2147483649 },
   { a: { $numberLong: "9223372036854775806" } }];
   var expRecsFindByDecimailType1 = [{ a: { $decimal: "-9223372036854775809" } }];
   checkResult( testPara.testCL, null, null, expRecs1, { _id: 1 } );
   checkResult( testPara.testCL, { a: { $type: 1, $et: 18 } }, null, expRecsFindByType1, { _id: 1 } );
   checkResult( testPara.testCL, { a: { $type: 1, $et: 100 } }, null, expRecsFindByDecimailType1, { _id: 1 } );

   //update use $inc,result out of range
   var updateCondition2 = { $inc: { a: 2 } };
   testPara.testCL.update( updateCondition2 );

   //check result
   var expRecs2 = [{ a: -2147483647 },
   { a: 2147483648 },
   { a: { $decimal: "-9223372036854775807" } },
   { a: { $decimal: "9223372036854775808" } },
   { a: -1.7E+308 },
   { a: 1.7E+308 },
   { a: 1 },
   { a: 1 }];
   var expRecsFindByType2 = [{ a: -2147483647 }, { a: 2147483648 }];
   var expRecsFindByDecimailType2 = [{ a: { $decimal: "-9223372036854775807" } },
   { a: { $decimal: "9223372036854775808" } }]
   checkResult( testPara.testCL, null, null, expRecs2, { _id: 1 } );
   checkResult( testPara.testCL, { a: { $type: 1, $et: 18 } }, null, expRecsFindByType2, { _id: 1 } );
   checkResult( testPara.testCL, { a: { $type: 1, $et: 100 } }, null, expRecsFindByDecimailType2, { _id: 1 } );
}
