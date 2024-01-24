/******************************************************************************
 * @Description   : seqDB-24279:查询 cond 条件带 $or，检查访问计划信息
 * @Author        : Yang Qincheng
 * @CreateTime    : 2021.07.01
 * @LastEditTime  : 2021.07.01
 * @LastEditors   : Yang Qincheng
 ******************************************************************************/

testConf.clName = CHANGEDPREFIX + "_24279";

main( test );

function test ( testPara )
{
   testPara.testCL.createIndex( "index_24279", { "a": 1 }, false );

   // case 1: sub-conditions of the same fields and all support index
   var arr = new Array();
   arr.push( { "a": 1 } );
   arr.push( { "a": { "$gt": 2, "$lt": 4 } } );
   arr.push( { "a": { "$gte": 5, "$lte": 7 } } );
   arr.push( { "a": { "$in": [8, 9] } } );
   arr.push( { "a": { "$all": [10, 11] } } );
   arr.push( { "a": { "$et": 12 } } );
   arr.push( { "a.$1": 13 } );
   arr.push( { "a": { "$exists": 0 } } );
   arr.push( { "a": { "$isnull": 1 } } );
   var result1 = testPara.testCL.find( { "$or": arr } ).explain();

   var iXBoundArr = new Array();
   iXBoundArr.push( [{ "$undefined": 1 }, { "$undefined": 1 }] );
   iXBoundArr.push( [null, null] );
   iXBoundArr.push( [1, 1] );
   iXBoundArr.push( [2, 4] );
   iXBoundArr.push( [5, 7] );
   iXBoundArr.push( [8, 8] );
   iXBoundArr.push( [9, 9] );
   iXBoundArr.push( [10, 10] );
   iXBoundArr.push( [12, 12] );
   iXBoundArr.push( [13, 13] );
   checkResult( result1, "ixscan", { "a": iXBoundArr }, true );

   // case 2: sub-conditions of the same fields and partially support index
   arr.push( { "a": { "$ne": 14 } } );
   var result2 = testPara.testCL.find( { "$or": arr } ).explain();
   checkResult( result2, "tbscan", null, true );

   // case 3: sub-conditions of the different fields
   var result3 = testPara.testCL.find( { "$or": [{ "a": 1 }, { "b": 1 }, { "c": 1 }] } ).explain();
   checkResult( result3, "tbscan", null, true );
}

function checkResult ( actResult, expScanType, expIXBound, expNeedMatch )
{
   var object = actResult.current().toObj();
   var ixBound = object.IXBound;
   var needMatch = object.NeedMatch;
   var scanType = object.ScanType;

   if( !commCompareObject( expIXBound, ixBound ) )
   {
      throw new Error( "expected ixBound:" + JSON.stringify( expIXBound ) +
         "\nactual ixBound:" + JSON.stringify( ixBound ) );
   }
   if( !commCompareObject( expNeedMatch, needMatch ) )
   {
      throw new Error( "expected needMatch: " + expNeedMatch +
         "\nactual needMatch: " + needMatch );
   }
   if( !commCompareObject( expScanType, scanType ) )
   {
      throw new Error( "expected scanType:" + expScanType +
         "\nactual scanType:" + scanType );
   }
}
