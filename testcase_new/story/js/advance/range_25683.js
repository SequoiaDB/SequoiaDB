/******************************************************************************
 * @Description   : seqDB-25683:IndexValueIncluded参数校验
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
   cl.createIndex( idxName, { a: 1, b: 1 } );

   // IndexValueIncluded为任意字符串
   var cursor = cl.find().sort( { "a": 1, "b": 1 } ).hint( {
      "$Range": {
         "IsAllEqual": false,
         "IndexValueIncluded": "test",
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
      assert.equal( e, -6 );
   }
   // IndexValueIncluded元素取值为任意字符串
   var cursor = cl.find().sort( { "a": 1, "b": 1 } ).hint( {
      "$Range": {
         "IsAllEqual": false,
         "IndexValueIncluded": [["test", false]],
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
      assert.equal( e, -6 );
   }

   // IndexValueIncluded为null
   var cursor = cl.find().sort( { "a": 1, "b": 1 } ).hint( {
      "$Range": {
         "IsAllEqual": false,
         "IndexValueIncluded": null,
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
      assert.equal( e, -6 );
   }
   // IndexValueIncluded元素取值为null
   var cursor = cl.find().sort( { "a": 1, "b": 1 } ).hint( {
      "$Range": {
         "IsAllEqual": false,
         "IndexValueIncluded": [[true, null]],
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
      assert.equal( e, -6 );
   }

   // IndexValueIncluded为""
   var cursor = cl.find().sort( { "a": 1, "b": 1 } ).hint( {
      "$Range": {
         "IsAllEqual": false,
         "IndexValueIncluded": "",
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
      assert.equal( e, -6 );
   }
   // IndexValueIncluded元素取值为空串
   var cursor = cl.find().sort( { "a": 1, "b": 1 } ).hint( {
      "$Range": {
         "IsAllEqual": false,
         "IndexValueIncluded": [["  ", ""]],
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
      assert.equal( e, -6 );
   }

   // IndexValueIncluded为"  "
   var cursor = cl.find().sort( { "a": 1, "b": 1 } ).hint( {
      "$Range": {
         "IsAllEqual": false,
         "IndexValueIncluded": "  ",
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
      assert.equal( e, -6 );
   }
}