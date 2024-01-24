/******************************************************************************
*@Description : seqDB-20083:指定sel字段为$include:0且与排序字段不相同相同，执行查询
*@author      : Zhao xiaoni 2019-10-24
*               liuli 2020-10-14                       
******************************************************************************/
testConf.clName = COMMCLNAME + "_20083";

main( test );

function test ( args )
{
   var cl = args.testCL;

   //insert records and get expect result
   var insertNum = 100;
   var records = [];
   var expResult = [];
   for( var i = 0; i < insertNum; i++ )
   {
      records.push( { a: i, b: ( insertNum - i ) } );
      expResult.push( { b: ( i + 1 ) } );
   }
   cl.insert( records );

   //query
   var sel = { _id: { "$include": 0 }, a: { "$include": 0 } };
   var sort = { b: 1 };
   var cursor = cl.find( {}, sel ).sort( sort );

   commCompareResults( cursor, expResult );
}
