/******************************************************************************
*@Description : seqDB-20080:指定sel字段为$include:0且与排序字段相同，执行查询
*@author      : Zhao xiaoni
*@Date        : 2019-10-24
******************************************************************************/
main( test );

function test ()
{
   var clName = "cl_20080";
   commDropCL( db, COMMCSNAME, clName );
   var cl = commCreateCL( db, COMMCSNAME, clName );

   //insert records and get expect result
   var insertNum = 100;
   var records = [];
   var expResult = [];
   for( var i = 0; i < insertNum; i++ )
   {
      records.push( { a: i, b: ( insertNum - i ) } );
      expResult.push( { b: ( insertNum - i ) } );
   }
   cl.insert( records );

   //query
   var sel = { _id: { "$include": 0 }, a: { "$include": 0 } };
   var sort = { a: 1 };
   var cursor = cl.find( {}, sel ).sort( sort );

   //get actual result
   var actResult = [];
   while( cursor.next() )
   {
      actResult.push( cursor.current().toObj() );
   }

   //check Result
   assert.equal( actResult.length, expResult.length );
   for( var i = 0; i < actResult.length; i++ )
   {
      assert.equal( actResult[i], expResult[i] );
   }
   commDropCL( db, COMMCSNAME, clName, false, false );
}
