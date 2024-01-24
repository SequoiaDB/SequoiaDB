/******************************************************************************
 * @Description   : seqDB-26720:rc隔离级别事务中进行$Range查询
 * @Author        : liuli
 * @CreateTime    : 2022.07.13
 * @LastEditTime  : 2022.07.13
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.clName = COMMCLNAME + "_26720";

main( test );

function test ( args )
{
   var cl = args.testCL;
   var idxName = "idx_26720";
   var recsNum = 10000;

   var docs = [];
   for( var i = 0; i < recsNum; i++ )
   {
      docs.push( { a: i, b: i } );
   }
   cl.insert( docs );
   cl.createIndex( idxName, { a: 1, b: 1 } );

   // 设置隔离级别后开启事务
   db.setSessionAttr( { TransIsolation: 1 } );
   db.transBegin();

   // 匹配符 $or 匹配不到记录
   var explainObject = cl.find( { $or: [{ a: 1, b: 2 }, { a: 3, b: 4 }] } ).sort( { "a": 1, "b": 1 } ).hint( {
      "": idxName,
      "$Range": {
         "IsAllEqual": true,
         "PrefixNum": 2,
         "IndexValue": [{ "1": 1, "2": 2 }, { "1": 3, "2": 4 }]
      }
   } ).explain( { Run: true } );

   var scanType = explainObject.current().toObj().ScanType;
   assert.equal( scanType, "ixscan" );

   // 校验数据
   var cursor = cl.find().sort( { a: 1 } );
   commCompareResults( cursor, docs );

   // 提交事务
   db.transCommit();
}