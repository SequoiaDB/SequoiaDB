/******************************************************************************
*@Description : seqDB-22484:查询cond条件带$exists/$isnull，强制走索引，检查访问计划索引生成范围和NeedMatch情况 
*@author      : Zhao Xiaoni
*@Date        : 2020.7.27
******************************************************************************/
testConf.clName = CHANGEDPREFIX + "_22484";

main( test );

function test( testPara )
{
   var indexName = "index_22484";
   testPara.testCL.createIndex( indexName, { "a": 1 }, false );

   var expIXBound = { "a": [[{ "$minElement": 1 }, { "$undefined": 1 }], [ null, { "$maxElement": 1 }]]};
   checkScanType( testPara.testCL, { "a": { "$isnull": 0 } }, indexName, expIXBound, false ); 
    
   expIXBound = { "a": [[{ "$undefined": 1 }, { "$undefined": 1 }], [ null, null ]]};
   checkScanType( testPara.testCL, { "a": { "$isnull": 1 } }, indexName, expIXBound, true );
   
   expIXBound = { "a": [[{ "$undefined": 1 }, { "$undefined": 1 }]]};
   checkScanType( testPara.testCL, { "a": { "$exists": 0 } }, indexName, expIXBound, false );

   expIXBound = { "a": [[{ "$minElement": 1 }, { "$undefined": 1 }], [{ "$undefined": 1 }, { "$maxElement": 1 }]]};
   checkScanType( testPara.testCL, { "a": { "$exists": 1 } }, indexName, expIXBound, false );
}

function checkScanType( cl, cond, indexName, expIXBound, expNeedMatch )
{
   var object = cl.find( cond ).hint( { "": indexName } ).explain().current().toObj();
   var ixBound = object.IXBound;
   var needMatch = object.NeedMatch;
   if( !commCompareObject( expIXBound, ixBound ) || !commCompareObject( expNeedMatch, needMatch ) )
   {
      throw new Error( "expIXBound: " + JSON.stringify( expIXBound ) + ", ixBound: " + JSON.stringify( ixBound ) +
            "\nexpNeedMatch: " + expNeedMatch + ", needMatch: " + needMatch );
   }
}
