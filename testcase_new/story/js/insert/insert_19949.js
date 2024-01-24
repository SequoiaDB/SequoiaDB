/******************************************************************************
*@Description: 插入记录，指定oid值非法
*@author:      liuxiaoxuan
*@createdate:  2019.10.10
*@testlinkCase: seqDB-19949:插入记录，指定oid值非法
******************************************************************************/

main( test );
function test ()
{
   var clName = COMMCLNAME + "_insert19949";
   commDropCL( db, COMMCSNAME, clName, true, true, "drop collection in the beginning" );
   var dbcl = commCreateCL( db, COMMCSNAME, clName );

   //插入Oid值长度小于24字节
   try
   {
      dbcl.insert( { a: { "$oid": "123abcd00af12358902300" } } );
      throw new Error( "need throw error" );
   }
   catch( e )
   {
      if( SDB_INVALIDARG != e.message )
      {
         throw e;
      }
   }

   //插入Oid值长度大于24字节
   try
   {
      dbcl.insert( { a: { "$oid": "123abcd00af12358902300123456" } } );
      throw new Error( "need throw error" );
   }
   catch( e )
   {
      if( SDB_INVALIDARG != e.message )
      {
         throw e;
      }
   }

   //插入Oid值长度等于24字节但内容不正确
   try
   {
      dbcl.insert( { a: ObjectId( "123abcd00ef12358902300eg" ) } );
      throw new Error( "need throw error" );
   }
   catch( e )
   {
      if( SDB_INVALIDARG != e.message )
      {
         throw e;
      }
   }

   commDropCL( db, COMMCSNAME, clName, true, true, "drop collection in the end" );
}
