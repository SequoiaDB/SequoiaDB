/******************************************************************************
 * @Description   : seqDB-25677~25681
 * @Author        : XiaoNi Huang
 * @CreateTime    : 2022.04.05
 * @LastEditTime  : 2022.04.05
 * @LastEditors   : XiaoNi Huang
 ******************************************************************************/
testConf.clName = CHANGEDPREFIX + "_range_25677";

main( test );

function test ( args )
{
   var cl = args.testCL;
   var idxName = "idx";
   var recsNum = 1000;

   var docs = [];
   for( var i = 0; i < recsNum; i++ )
   {
      docs.push( { a: i, b: i, c: i } );
   }
   cl.insert( docs );

   // seqDB-25677:不带索引，使用$Range操作符查询
   // 点查
   var cursor = cl.find().sort( { "a": 1, "b": 1 } ).hint( {
      "$Range": {
         "IsAllEqual": true,
         "PrefixNum": [2],
         "IndexValue": [
            [{ "a": 0, "b": 0 }, { "a": 2, "b": 2 }]]
      }
   } );
   try
   {
      cursor.next();
      throw new Error( "expect fail but actual success." );
   }
   catch( e )
   {
      assert.equal( e, -32 );
   }

   // 批量范围查询
   var cursor = cl.find().sort( { "a": 1, "b": 1 } ).hint( {
      "$Range": {
         "IsAllEqual": false,
         "IndexValueIncluded": [[true, true]],
         "PrefixNum": [2],
         "IndexValue": [
            [{ "a": 0, "b": 0 }, { "a": 2, "b": 2 }]]
      }
   } );
   try
   {
      cursor.next();
      throw new Error( "expect fail but actual success." );
   }
   catch( e )
   {
      assert.equal( e, -32 );
   }

   // seqDB-25678:不带sort，使用$Range操作符查询
   cl.createIndex( idxName, { a: 1, b: 1 } );
   var cursor = cl.find().hint( {
      "$Range": {
         "IsAllEqual": false,
         "IndexValueIncluded": [[true, true]],
         "PrefixNum": [2],
         "IndexValue": [
            [{ "a": 0, "b": 0 }, { "a": 2, "b": 2 }]]
      }
   } );
   try
   {
      cursor.next();
      throw new Error( "expect fail but actual success." );
   }
   catch( e )
   {
      assert.equal( e, -32 );
   }
   cl.dropIndex( idxName );

   // seqDB-25679:查询带sort并且是索引扫描，count使用$Range操作符查询
   cl.createIndex( idxName, { a: 1, b: 1 } );
   try
   {
      cl.count().hint( {
         "$Range": {
            "IsAllEqual": false,
            "IndexValueIncluded": [[true, true]],
            "PrefixNum": [2],
            "IndexValue": [
               [{ "a": 0, "b": 0 }, { "a": 2, "b": 2 }]]
         }
      } );
      // SEQUOIADBMAINSTREAM-8271
      // throw new Error( "expect fail but actual success." );
   }
   catch( e )
   {
      assert.equal( e, -32 );
   }
   cl.dropIndex( idxName );

   // seqDB-25680:查询带sort并且是索引扫描，queryAndModify使用$Range操作符查询
   cl.createIndex( idxName, { a: 1, b: 1 } );
   var cursor = cl.find().sort( { "a": 1, "b": 1 } ).update( { "$set": { "a": 1 } } ).hint( {
      "$Range": {
         "IsAllEqual": false,
         "IndexValueIncluded": [[true, true]],
         "PrefixNum": [2],
         "IndexValue": [
            [{ "a": 0, "b": 0 }, { "a": 2, "b": 2 }]]
      }
   } );
   try
   {
      cursor.next();
      // SEQUOIADBMAINSTREAM-8271
      // throw new Error( "expect fail but actual success." );
   }
   catch( e )
   {
      assert.equal( e, -32 );
   }
   cl.dropIndex( idxName );
}