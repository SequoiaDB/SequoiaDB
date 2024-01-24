/*******************************************************************
* @Description : test case for SdbQuery and SdbCursor
*                seqDB-14664:使用arrayAccess获取SdbCursor中指定下标元素
*                seqDB-14665:使用下标获取SdbQuery中指定元素
*                seqDB-14666:指定flags执行SdbQuery
*                seqDB-14667:使用close关闭SdbQuery
*                seqDB-14668:执行getQueryMeta获取SdbQuery的查询元数据
* @author      : Liang XueWang
*                2018-03-12
*******************************************************************/
var clName = COMMCLNAME + "_dbClasses14664";

main( test );

function test ()
{
   var cl = commCreateCL( db, COMMCSNAME, clName );

   cl.insert( { a: 1 } );
   cl.insert( { a: 2 } );

   cursorArrayAccess( cl );
   queryIndex( cl );
   queryArrayAccess( cl );
   queryFlags( cl );
   queryClose( cl );
   queryMeta( cl );

   commDropCL( db, COMMCSNAME, clName );
}

function cursorArrayAccess ( cl )
{
   var cursor = cl.find().sort( { a: 1 } );

   var a1 = JSON.parse( cursor.arrayAccess( 0 ) )["a"];
   var a2 = JSON.parse( cursor.arrayAccess( 1 ) )["a"];
   if( a1 !== 1 || a2 !== 2 )
   {
      throw new Error( "expect: 1 2, actual: " + a1 + " " + a2 );
   }

   var illegalIdx = ['b', -1, 1.2, 2];
   for( var i = 0; i < illegalIdx.length; i++ )
   {

      if( cursor.arrayAccess( illegalIdx[i] ) !== undefined )
      {
         throw new Error( "check arrayAccess with idx " + illegalIdx[i] );
      }
   }

}

function queryIndex ( cl )
{
   var a1 = JSON.parse( cl.find().sort( { a: 1 } )[0] )["a"];
   var a2 = JSON.parse( cl.find().sort( { a: 1 } )[1] )["a"];
   if( a1 !== 1 || a2 !== 2 )
   {
      throw new Error( "expect: 1 2, actual: " + a1 + " " + a2 );
   }

   var illegalIdx = ['b', -1, 1.2, 2];
   for( var i = 0; i < illegalIdx.length; i++ )
   {
      if( cl.find().sort( { a: 1 } )[illegalIdx[i]] !== undefined )
      {
         throw new Error( "check queryIndex with idx " + illegalIdx[i] );
      }
   }

}

function queryArrayAccess ( cl )
{
   var a1 = JSON.parse( cl.find().sort( { a: 1 } ).arrayAccess( 0 ) )["a"];
   var a2 = JSON.parse( cl.find().sort( { a: 1 } ).arrayAccess( 1 ) )["a"];
   if( a1 !== 1 || a2 !== 2 )
   {
      throw new Error( "expect: 1 2, actual: " + a1 + " " + a2 );
   }

   var illegalIdx = ['b', -1, 1.2, 2];
   for( var i = 0; i < illegalIdx.length; i++ )
   {
      if( cl.find().sort( { a: 1 } ).arrayAccess( illegalIdx[i] ) !== undefined )
      {
         throw new Error( "check queryArrayAccess with idx " + illegalIdx[i] );
      }
   }

}

function queryFlags ( cl )
{
   cl.find().flags( 0 );

   try
   {
      cl.find().flags( 'a' ); // nothrow
   }
   catch( e )
   {
   }
}

function queryClose ( cl )
{
   cl.find().close();
}

function queryMeta ( cl )
{
   var scanType = cl.find().getQueryMeta().next().toObj()["ScanType"];
   if( scanType !== "tbscan" )
   {
      throw new Error( "expect is tbscan, actual is: " + scanType );
   }

}
