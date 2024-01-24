/******************************************************************************
 * @Description   : seqDB-25686:$Range格式错误
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

   // "$Range"少字段
   var cursor = cl.find().sort( { "a": 1, "b": 1 } ).hint( {
      "$Range": {
         "IsAllEqual": true
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

   // "$Range"多字段
   var cursor = cl.find().sort( { "a": 1, "b": 1 } ).hint( {
      "$Range": {
         "IsAllEqual": false,
         "IndexValueIncluded": [[true, false]],
         "PrefixNum": [2],
         "IndexValue":
            [{ "a": 0, "b": 0 }, { "a": 2, "b": 2 }],
         "": 1
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

   // "$Range"取值非bson对象
   var cursor = cl.find().sort( { "a": 1, "b": 1 } ).hint( {
      "$Range": ""
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