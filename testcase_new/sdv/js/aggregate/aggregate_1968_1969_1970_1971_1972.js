/************************************************************************
*@Description: seqDB-1968:使用$group时没有指定_id
               seqDB-1969:使用$group时指定的_id为null
               seqDB-1970:使用$group时指定的_id字段值不存在
               seqDB-1971:$sort中字段值为0
               seqDB-1972:$sort指定的字段不存在
*@Author: 2015/11/3  huangxiaoni
************************************************************************/
testConf.clName = COMMCLNAME + "_1968_1969_1970_1971_1972";

main( test );

function test( testPara )
{
   testPara.testCL.insert( { "a": 1, "b": 2, "c": 3 } );
   testPara.testCL.insert( { "a": 2, "b": 2, "c": 4 } );
   testPara.testCL.insert( { "a": 1, "b": 1, "c": 2 } );
   testPara.testCL.insert( { "a": 2, "b": 2, "c": 3 } );

   var expResult = [ { "a": 1 }, { "a": 2 }, { "a": 1 }, { "a": 2 } ];
   var cursor = testPara.testCL.aggregate( { "$group": { "a": "$a" }  } );
   commCompareResults ( cursor, expResult );

   expResult = [ { "a": 1, "b": 2, "c": 3 }, { "a": 2, "b": 2, "c": 4 }, { "a": 1, "b": 1, "c": 2 }, { "a": 2, "b": 2, "c": 3 } ]; 
   cursor = testPara.testCL.aggregate( { "$group": { "_id": null }  } );
   commCompareResults ( cursor, expResult );

   expResult = [ { "a": 1, "b": 2, "c": 3 } ];
   cursor = testPara.testCL.aggregate( { "$group": { "_id": "$e" } } );
   commCompareResults ( cursor, expResult );
 
   //SEQUOIADBMAINSTREAM-5802
/*   try
   {
      cursor = testPara.testCL.aggregate( { "$sort": { "a": 0 } } );
      throw "$sort should be failed!";
   }
   catch( e )
   {
      if( e !== -6 )
      {
         throw new Error( e );
      }
   }*/

   expResult = [ { "a": 1, "b": 2, "c": 3 }, { "a": 2, "b": 2, "c": 4 }, { "a": 1, "b": 1, "c": 2 }, { "a": 2, "b": 2, "c": 3 } ];
   cursor = testPara.testCL.aggregate( { "$sort": { "e": 1 }  } );
   commCompareResults ( cursor, expResult );
}
