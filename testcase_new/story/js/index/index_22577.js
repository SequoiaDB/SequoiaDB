/*******************************************************************************
*@Description:  In multiple IndexDef, SortCondition doesn't match whith findCondition, Checout result
               多字段索引, 排序字段与索引字段不匹配， 验证索引使用情况。 [seqDB-22577]
*@Author: 2020/08/19  Zixian Yan
*******************************************************************************/
testConf.clName = COMMCLNAME + "_22577";
var indexName = "Index_22577";

main( test );
function test ( testPara )
{
   var cl = testPara.testCL;
   cl.createIndex( indexName, { a: 1, b: 1, c: 1 } );

   // findCondition, sortCondition, expectResult("tableScan"/"indexScan")
   var condList = [[{ c: 1 }, { c: 1, a: 1, b: 1 }, [true, false]],
   [{ c: { $gte: 1 } }, { c: 1, a: 1, b: 1 }, [true, true]],
   [{ a: 1, b: 1, c: 1 }, { a: 1, c: 1 }, [false, false]],
   [{ a: 1, b: { $gte: 1 }, c: 1 }, { a: 1, c: 1 }, [false, false]],
   [{ a: { $lt: 1 }, b: 1, c: { $lt: 1 } }, { a: 1, c: 1 }, [true, false]],
   [{ a: { $lt: 1 }, b: { $gte: 1 }, c: { $lt: 1 } }, { a: 1, c: 1 }, [true, true]],
   [{ d: 1 }, { a: 1, d: 1, b: 1 }, [true, false]],
   [{ c: 1 }, { a: 1, c: 1, b: 1 }, [true, false]],
   [{ d: { $gte: 1 } }, { a: 1, d: 1, b: 1 }, [true, true]],
   [{ c: { $gte: 1 } }, { a: 1, c: 1, b: 1 }, [true, true]],
   [{ d: 1 }, { a: 1, b: 1, d: 1 }, [true, false]],
   [{ d: { $gte: 1 } }, { a: 1, b: 1, d: 1 }, [true, true]],
   [{ d: 1, f: 1, e: 1 }, { d: 1, e: 1, f: 1 }, [false, false]],
   [{ d: { $lt: 1 }, f: { $lt: 1 }, e: { $lt: 1 } }, { d: 1, e: 1, f: 1 }, [true, true]]];

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
