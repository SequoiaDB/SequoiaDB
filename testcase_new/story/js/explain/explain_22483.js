/******************************************************************************
*@Description : seqDB-22483:查询cond条件带$exists/$isnull，检查访问计划中扫描方式 
*@author      : Zhao Xiaoni
*@Date        : 2020.7.27
******************************************************************************/
testConf.clName = CHANGEDPREFIX + "_22483";

main( test );

function test ( testPara )
{
   testPara.testCL.createIndex( "index_22483", { "a": 1 }, false );

   checkScanType( testPara.testCL, { "a": { "$isnull": 0 } }, "tbscan" );

   checkScanType( testPara.testCL, { "a": { "$isnull": 1 } }, "ixscan" );

   checkScanType( testPara.testCL, { "a": { "$exists": 0 } }, "ixscan" );

   checkScanType( testPara.testCL, { "a": { "$exists": 1 } }, "tbscan" );
}

function checkScanType ( cl, cond, expResult )
{
   var scanType = cl.find( cond ).explain().current().toObj().ScanType;
   if( !commCompareObject( expResult, scanType ) )
   {
      throw new Error( "expResult: " + expResult + ", scanType: " + scanType );
   }
}
