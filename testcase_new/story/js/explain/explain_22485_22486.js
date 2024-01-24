/******************************************************************************
*@Description : seqDB-22485:查询cond条件带$regex指定字符串前序匹配时，检查访问计划索引生成范围无冗余项 
                seqDB-22486:查询cond条件带$et查询正则表达式类型时，检查访问计划索引生成范围为对应正则表达式的范围
*@author      : Zhao Xiaoni
*@Date        : 2020.7.27
******************************************************************************/
testConf.clName = CHANGEDPREFIX + "_22485_22486";

main( test );

function test ( testPara )
{
   var indexName = "index_22485_22486";
   testPara.testCL.createIndex( indexName, { "a": 1 }, false );

   var expIXBound = { "a": [["", {}]] };
   var cond = { "a": { "$regex": "^rg", "$options": "i" } };
   var ixBound = testPara.testCL.find( cond ).hint( { "": indexName } ).explain().current().toObj().IXBound;
   if( !commCompareObject( expIXBound, ixBound ) )
   {
      throw new Error( "expIXBound: " + JSON.stringify( expIXBound ) + ", ixBound: " + JSON.stringify( ixBound ) );
   }

   expIXBound = { "a": [[{ "$regex": "^rg", "$options": "i" }, { "$regex": "^rg", "$options": "i" }]] };
   cond = { "a": { "$et": { "$regex": "^rg", "$options": "i" } } };
   ixBound = testPara.testCL.find( cond ).hint( { "": indexName } ).explain().current().toObj().IXBound;
   if( !commCompareObject( expIXBound, ixBound ) )
   {
      throw new Error( "expIXBound: " + JSON.stringify( expIXBound ) + ", ixBound: " + JSON.stringify( ixBound ) );
   }
}