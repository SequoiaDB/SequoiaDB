/******************************************************************************
 * @Description   : seqDB-28453:批量范围查，PrefixNum范围不一致，大的区间在前面，部分区间能匹配到数据
 * @Author        : liuli
 * @CreateTime    : 2022.10.25
 * @LastEditTime  : 2022.10.25
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.clName = COMMCLNAME + "_28453";

main( test );

function test ( args )
{
   var dbcl = args.testCL;
   var idxName = "idx_28453";
   var recsNum = 10;

   var docs = [];
   for( var i = 0; i < recsNum; i++ )
   {
      docs.push( { a: 1, b: 1, c: i } );
   }
   dbcl.insert( docs );
   dbcl.createIndex( idxName, { a: 1, b: 1, c: 1 } );

   // 大区间在前面，只有大区间能匹配到数据
   var cursor = dbcl.find().hint( {
      "": idxName,
      "$Range": {
         "IsAllEqual": false,
         "PrefixNum": [3, 2],
         "IndexValueIncluded": [[true, false], [true, false]],
         "IndexValue": [[{ "1": 1, "2": 1, "3": 0 }, { "1": 1, "2": 1, "3": { "$maxKey": 1 } }], [{ "1": 1, "2": 1 }, { "1": 1, "2": { "$maxKey": 1 } }]]
      }
   } );

   commCompareResults( cursor, docs );

   // 大区间在前面，区间都能匹配到数据
   var cursor = dbcl.find().hint( {
      "": idxName,
      "$Range": {
         "IsAllEqual": false,
         "PrefixNum": [3, 2],
         "IndexValueIncluded": [[true, true], [true, true]],
         "IndexValue": [[{ "1": 1, "2": 1, "3": 0 }, { "1": 1, "2": 1, "3": { "$maxKey": 1 } }], [{ "1": 1, "2": 1 }, { "1": 1, "2": { "$maxKey": 1 } }]]
      }
   } );

   commCompareResults( cursor, docs );

   // 大区间在后面，只有大区间能匹配到数据
   var cursor = dbcl.find().hint( {
      "": idxName,
      "$Range": {
         "IsAllEqual": false,
         "PrefixNum": [2, 3],
         "IndexValueIncluded": [[false, true], [true, true]],
         "IndexValue": [[{ "1": 1, "2": 1 }, { "1": 1, "2": { "$maxKey": 1 } }], [{ "1": 1, "2": 1, "3": 0 }, { "1": 1, "2": 1, "3": { "$maxKey": 1 } }]]
      }
   } );

   commCompareResults( cursor, docs );

   // 大区间在后面，区间都能匹配到数据
   var cursor = dbcl.find().hint( {
      "": idxName,
      "$Range": {
         "IsAllEqual": false,
         "PrefixNum": [2, 3],
         "IndexValueIncluded": [[true, true], [true, true]],
         "IndexValue": [[{ "1": 1, "2": 1 }, { "1": 1, "2": { "$maxKey": 1 } }], [{ "1": 1, "2": 1, "3": 0 }, { "1": 1, "2": 1, "3": { "$maxKey": 1 } }]]
      }
   } );

   commCompareResults( cursor, docs );
}