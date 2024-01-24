/******************************************************************************
*@Description : seqDB-20084:指定sort为嵌套字段且是sel的下层嵌套，执行查询
*@author      : Zhao xiaoni 2019-10-24
*               liuli 2020-10-14                        
******************************************************************************/
testConf.clName = COMMCLNAME + "_20085";

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
      records.push( { a: { b: { c: i }, e: ( insertNum - i ) }, f: ( insertNum - i ) } );
      expResult.push( { a: { b: { c: ( insertNum - ( i + 1 ) ) } } } );
   }
   cl.insert( records );

   //query
   var sel = { _id: { "$include": 0 }, "a.b": 1 };
   var sort = { "a.e": 1 };
   var cursor = cl.find( {}, sel ).sort( sort );

   commCompareResults( cursor, expResult );
}
