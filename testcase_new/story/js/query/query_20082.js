/******************************************************************************
*@Description : seqDB-20082:指定sel字段为$include:1且与排序字段不相同，执行查询
*@author      : Zhao xiaoni 2019-10-24
*               liuli 2020-10-14                       
******************************************************************************/
testConf.clName = COMMCLNAME + "_20082";

main( test );

function test ( args )
{
   var cl = args.testCL;

   //insert records and get expect result
   var insertNum = 100;
   var expResult = [];
   var records = [];
   for( var i = 0; i < insertNum; i++ )
   {
      records.push( { a: i, b: ( insertNum - i ) } );
      expResult.push( { a: ( insertNum - ( i + 1 ) ) } );
   }
   cl.insert( records );

   //query
   var sel = { a: { "$include": 1 } };
   var sort = { b: 1 };
   var cursor = cl.find( {}, sel ).sort( sort );

   commCompareResults( cursor, expResult );
}
