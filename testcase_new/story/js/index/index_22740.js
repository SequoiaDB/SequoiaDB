/*******************************************************************************
*@Description:  [seqDB-22740] multiple index field equal with matches conditon, checkout Result
                多字段索引，排序字段与匹配条件等值，验证非索引使用情况
*@Author: 2020/09/08  Zixian Yan
*******************************************************************************/
testConf.clName = COMMCLNAME + "_22740";
var indexName = "Index_22740";

main( test );
function test ( testPara )
{
   var cl = testPara.testCL;
   cl.createIndex( indexName, { a: 1, b: 1, c: 1 } );

   // findCondition, sortCondition, expectResult("tableScan"/"indexScan")
   var condList = [[{ a: 1, b: 1, c: 1 }, { a: 1, c: 1, d: 1 }, [true, true]],
   [{ a: 1, b: 1, c: 1 }, { a: 1, b: 1, d: 1 }, [true, true]],
   [{ a: 1, b: 1, d: { $gt: 1 } }, { a: 1, b: 1, d: 1 }, [true, true]],
   [{ a: 1, b: 1, d: { $et: 1 } }, { a: 1, b: 1, d: 1 }, [false, false]],
   [{ a: 1, b: 1, c: 1 }, { a: 1, b: 1 }, [false, false]]];

   var scanTypeList = ["tableScan", "indexScan"];

   for( var i in condList )
   {
      var findCond = condList[i][0];
      var sortCond = condList[i][1];
      var expectList = condList[i][2];
      for( var index in scanTypeList )
      {
         var scanType = scanTypeList[index];
         var expectResult = expectList[index];
         checkResult( cl, findCond, sortCond, scanType, expectResult );
      }
   }

}

function checkResult ( cl, findCond, sortCond, scanType, expectResult )
{
   if( scanType == "tableScan" )
   {
      scanType = { "": null };
   }
   else
   {
      scanType = { "": indexName };
   }

   var actuallyResult = cl.find( findCond ).sort( sortCond ).hint( scanType ).explain().current().toObj()["UseExtSort"];
   assert.equal( actuallyResult, expectResult );
}
