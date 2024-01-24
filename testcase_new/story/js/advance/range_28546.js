/******************************************************************************
 * @Description   : seqDB-28546:普通表不指定hint和sort，使用$Range操作符后使用count
 * @Author        : HuangHaimei
 * @CreateTime    : 2022.10.31
 * @LastEditTime  : 2022.11.01
 * @LastEditors   : HuangHaimei
 ******************************************************************************/
testConf.clName = COMMCLNAME + "_28546_28547";

main( test );

function test ( args )
{
   var dbcl = args.testCL;
   var idxName = "idx_28546_28547";
   var recsNum = 10;

   var docs = [];
   for( var i = 0; i < recsNum; i++ )
   {
      for( var j = 0; j < recsNum; j++ )
      {
         docs.push( { a: i, b: j } );
      }
   }
   dbcl.insert( docs );
   dbcl.createIndex( idxName, { a: 1, b: 1 } );

   // hint不指定索引，不指定sort批量点查
   var result = dbcl.find().hint( {
      "$Range": {
         "IsAllEqual": true,
         "PrefixNum": 2,
         "IndexValue": [{ "1": 1, "2": 1 }, { "1": 3, "2": 3 }]
      }
   } ).count();
   assert.tryThrow( SDB_OPTION_NOT_SUPPORT, function()
   {
      println( result );
   } );

   // hint不指定索引,不指定sort批量范围查
   var result = dbcl.find().hint( {
      "$Range": {
         "IsAllEqual": false,
         "PrefixNum": [2, 2],
         "IndexValueIncluded": [[true, false], [true, false]],
         "IndexValue": [[{ "1": 2, "2": 2 }, { "1": 2, "2": 6 }], [{ "1": 3, "2": 2 }, { "1": 3, "2": 6 }]]
      }
   } ).count();
   assert.tryThrow( SDB_OPTION_NOT_SUPPORT, function()
   {
      println( result );
   } );
}