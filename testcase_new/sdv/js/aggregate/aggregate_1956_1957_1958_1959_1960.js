/************************************************************************
*@Description: seqDB-1956:$sort指定多个字段排序,且field1有多个相同的字段值
               seqDB-1957:$project+$sort组合查询,$project中field1字段值为0,$sort按field1排序
               seqDB-1958:$group+$match组合查询,$match中field1字段在$group中不存在
               seqDB-1959:$project+$group组合查询,$group中field1字段在$project中不存在
               seqDB-1960:$group+$project组合查询,$project中field1字段在$group中不存在
*@Author: 2015/11/3  huangxiaoni
************************************************************************/
testConf.clName = COMMCLNAME + "_1956_1957_1958_1959_1960";

main( test );

function test( testPara )
{
   testPara.testCL.insert( { "a": 1, "b": 2, "c": 3 } );
   testPara.testCL.insert( { "a": 2, "b": 2, "c": 4 } );
   testPara.testCL.insert( { "a": 1, "b": 1, "c": 2 } );
   testPara.testCL.insert( { "a": 2, "b": 2, "c": 3 } );

   var expResult = [ { "a": 1, "b": 2, "c": 3 }, { "a": 1, "b": 1, "c": 2 }, { "a": 2, "b": 2, "c": 3 }, { "a": 2, "b": 2, "c": 4 } ];
   var cursor = testPara.testCL.aggregate( { "$sort": { "a": 1, "b": -1, "c": 1 } } );   
   commCompareResults ( cursor, expResult );

   try
   {
      testPara.testCL.aggregate( { "$project": { "a": 0, "b": 1 } }, { "$sort": { "a": 1 } } );
      throw "$project + $sort should be failed!";
   }
   catch( e )
   {
      if( e !== -6 )
      {
         throw new Error( e );
      }
   }

   expResult = [];
   cursor = testPara.testCL.aggregate( { "$group": { "_id": "$a", "a": { "$avg": "$b" } } }, { "$match": { "c": 3 } } );
   commCompareResults ( cursor, expResult );

   try
   {
      testPara.testCL.aggregate( { "$project": { "a": 1 } }, { "$group": { "_id": "$b" } } );
      throw "$project + $group should be failed!";
   }
   catch( e )
   {
      if( e !== -6 )
      {
         throw new Error( e );
      }
   }

   try
   {
      testPara.testCL.aggregate( { "$group": { "_id": "$a", "avg_a": { "$avg": "$a" } } }, { "$project": { "a": 1 } } );
      throw "$group + $project should be failed!";
   }
   catch( e )
   {
      if( e !== -6 )
      {
         throw new Error( e );
      }
   }
}
