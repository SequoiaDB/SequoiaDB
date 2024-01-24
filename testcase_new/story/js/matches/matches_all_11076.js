/************************************
*@Description: 匹配符$all，匹配不存在的记录 
*@author:      wangkexin
*@createdate:  2019.3.5
*@testlinkCase:seqDB-11706
**************************************/

main( test );

function test ()
{
   var csName = COMMCSNAME;
   var clName = "cl11076";

   var cl = commCreateCL( db, csName, clName, {}, true, false, "create cl in the begin" );

   cl.insert( { a: [Regex( "^W", "i" ), 3] } );

   var cursor = cl.find( { a: { $all: [Regex( "^W", "i" ), Regex( "^s", "i" )] } } );
   if( cursor.next() != null )
   {
      throw new Error( "find() fail,have data : " + cursor.current() );
   }

   commDropCL( db, csName, clName, true, true, "drop CL in the end" );
}