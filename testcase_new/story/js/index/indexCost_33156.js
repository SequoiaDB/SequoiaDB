/******************************************************************************
 * @Description   : seqDB-33156 : 存在相同前缀的索引，索引的MCV不均匀时选择索引
 * @Author        : wuyan
 * @CreateTime    : 2022.09.01
 * @LastEditTime  : 2023.09.01
 * @LastEditors   : wu yan
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.clName = COMMCLNAME + "_index33156";

main( test );
function test ()
{
   var cl = testPara.testCL;
   cl.createIndex( "ac", { "a": 1, "c": 1 } );
   cl.createIndex( "abc", { "a": 1, "b": 1, "c": 1 } );

   insertData( cl );
   db.analyze( { Collection: COMMCSNAME + "." + testConf.clName, SampleNum: 100, Index: "abc" } );

   //插入数据，执行analyze使索引的MCV不均匀；构造不均匀的阈值占sample的比率超过0.05
   var data = [];
   for( i = 0; i < 3000; i++ )
   {
      data.push( {
         a: 80,
         b: "a",
         c: i,
         d: i
      } );
   }
   cl.insert( data );
   db.analyze( { Collection: COMMCSNAME + "." + testConf.clName, SampleNum: 100, Index: "ac" } );
   db.analyze( { Mode: 5, Collection: COMMCSNAME + "." + testConf.clName } )
   var indexName = cl.find( { a: 80, d: 10 } ).sort( { c: -1 } ).explain().current().toObj()["IndexName"];
   assert.equal( indexName, "ac", "use index is " + indexName );
}

function randomString ( maxLength )
{
   var arr = new Array( Math.round( Math.random() * maxLength ) )
   return arr.join( "a" )
}

function insertData ( dbcl )
{
   var batchSize = 10000;
   for( j = 0; j < 5; j++ ) 
   {
      data = [];
      for( i = 0; i < batchSize; i++ )
      {
         data.push( {
            a: Math.round( ( Math.random() * 80 ) ),
            b: randomString( 800 ),
            c: i,
            d: i
         } );
      }
      dbcl.insert( data );
   }
}

