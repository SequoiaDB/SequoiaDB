/******************************************************************************
 * @Description   : seqDB-26705:匹配使用$or，使用$Range查询访问计划
 * @Author        : liuli
 * @CreateTime    : 2022.07.05
 * @LastEditTime  : 2022.07.05
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.clName = COMMCLNAME + "_26705";

main( test );

function test ( args )
{
   var cl = args.testCL;
   var idxName = "idx_26705";
   var recsNum = 10000;

   var docs = [];
   for( var i = 0; i < recsNum; i++ )
   {
      docs.push( { a: i, b: i } );
   }
   cl.insert( docs );
   cl.createIndex( idxName, { a: 1, b: 1 } );

   // 匹配符 $or 匹配不到记录
   var explainObject = cl.find( { $or: [{ a: 1, b: 2 }, { a: 3, b: 4 }] } ).sort( { "a": 1, "b": 1 } ).hint( {
      "": idxName,
      "$Range": {
         "IsAllEqual": true,
         "PrefixNum": 2,
         "IndexValue": [{ "1": 1, "2": 2 }, { "1": 3, "2": 4 }]
      }
   } ).explain( { Run: true } );

   var dataRead = explainObject.current().toObj().DataRead;
   var indexRead = explainObject.current().toObj().IndexRead;
   var scanType = explainObject.current().toObj().ScanType;
   assert.equal( scanType, "ixscan" );

   if( dataRead >= recsNum )
   {
      throw new Error( "DataRead is " + dataRead + ", expext DataRead to be less than 10000" );
   }

   if( indexRead >= recsNum )
   {
      throw new Error( "DataRead is " + IndexRead + ", expext IndexRead to be less than 10000" );
   }

   // 匹配符 $or 匹配到记录
   var explainObject = cl.find( { $or: [{ a: 100, b: 100 }, { a: 300, b: 300 }] } ).sort( { "a": 1, "b": 1 } ).hint( {
      "": idxName,
      "$Range": {
         "IsAllEqual": true,
         "PrefixNum": 2,
         "IndexValue": [{ "1": 100, "2": 100 }, { "1": 300, "2": 300 }]
      }
   } ).explain( { Run: true } );

   var dataRead = explainObject.current().toObj().DataRead;
   var indexRead = explainObject.current().toObj().IndexRead;
   var scanType = explainObject.current().toObj().ScanType;
   assert.equal( scanType, "ixscan" );

   if( dataRead >= recsNum )
   {
      throw new Error( "DataRead is " + dataRead + ", expext DataRead to be less than 10000" );
   }

   if( indexRead >= recsNum )
   {
      throw new Error( "DataRead is " + IndexRead + ", expext IndexRead to be less than 10000" );
   }
}