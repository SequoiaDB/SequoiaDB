/************************************************************************
*@Description: seqDB-1963:$project中指定字段的字段值为0
               seqDB-1964:$project中指定的字段名不存在
               seqDB-1965:$project中字段值为非法
*@Author: 2015/11/3  huangxiaoni
************************************************************************/
testConf.clName = COMMCLNAME + "_1963_1964_1965";

main( test );

function test()
{
   testPara.testCL.insert( { "a": 1, "b": 2, "c": 3 } );
   testPara.testCL.insert( { "a": 2, "b": 2, "c": 4 } );
   testPara.testCL.insert( { "a": 1, "b": 1, "c": 2 } );
   testPara.testCL.insert( { "a": 2, "b": 2, "c": 3 } );

   var expResult = [ { "a": 1, "b": 2, "c": 3 }, { "a": 2, "b": 2, "c": 4 }, { "a": 1, "b": 1, "c": 2 }, { "a": 2, "b": 2, "c": 3 } ];
   var cursor = testPara.testCL.aggregate( { "$project": { "a": 0, "b": 0 } } );
   commCompareResults ( cursor, expResult );

   cursor = testPara.testCL.aggregate( { "$project": { "d": 0 } } );
   commCompareResults ( cursor, expResult );

   try
   {
      testPara.testCL.aggregate( { "$project": { "a": "test" } } );   
      throw "$project has invalid value!";
   }
   catch( e )
   {
      if( e !== -6 )
      {
         throw new Error( e );
      }
   }
}

