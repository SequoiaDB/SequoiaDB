/******************************************************************************
 * @Description   : seqDB-25684:PrefixNum参数校验
 * @Author        : XiaoNi Huang
 * @CreateTime    : 2022.04.05
 * @LastEditTime  : 2022.04.06
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

   // PrefixNum为任意字符串
   var cursor = cl.find().sort( { "a": 1, "b": 1 } ).hint( {
      "$Range": {
         "IsAllEqual": true,
         "PrefixNum": "test",
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

   // IndexValueIncluded元素取值为负数
   var cursor = cl.find().sort( { "a": 1, "b": 1 } ).hint( {
      "$Range": {
         "IsAllEqual": false,
         "IndexValueIncluded": [[true, false]],
         "PrefixNum": [-1],
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

   // PrefixNum为null
   var cursor = cl.find().sort( { "a": 1, "b": 1 } ).hint( {
      "$Range": {
         "IsAllEqual": true,
         "PrefixNum": null,
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

   // PrefixNum为""
   var cursor = cl.find().sort( { "a": 1, "b": 1 } ).hint( {
      "$Range": {
         "IsAllEqual": false,
         "IndexValueIncluded": [[true, false]],
         "PrefixNum": [""],
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

   // 点查，"IsAllEqual": true，PrefixNum为数组
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
      assert.equal( e, -6 );
   }

   // PrefixNum为"  "
   var cursor = cl.find().sort( { "a": 1, "b": 1 } ).hint( {
      "$Range": {
         "IsAllEqual": true,
         "PrefixNum": "  ",
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

   // 批量范围查询，PrefixNum非数组
   var cursor = cl.find().sort( { "a": 1, "b": 1 } ).hint( {
      "$Range": {
         "IsAllEqual": true,
         "IndexValueIncluded": [[true, false]],
         "PrefixNum": 2,
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