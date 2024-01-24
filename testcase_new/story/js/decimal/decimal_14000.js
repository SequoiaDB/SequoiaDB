/******************************************************************************
*@Description : test special decimal value with index
*               seqDB-14000:创建decimal字段索引后插入特殊decimal值         
*@author      : Liang XueWang 
******************************************************************************/
main( test )
function test ()
{
   var clName = COMMCLNAME + "_14000";
   commDropCL( db, COMMCSNAME, clName );

   var cl = commCreateCL( db, COMMCSNAME, clName );
   commCreateIndex( cl, "aIndex", { a: 1 }, { Unique: true } );

   var docs = [{ a: { $decimal: "MAX" } },
   { a: { $decimal: "MIN" } },
   { a: { $decimal: "NaN" } }];
   cl.insert( docs );

   for( var i = 0; i < docs.length; i++ )
   {
      var cursor = cl.find( docs[i] );
      var expRecs = [docs[i]];
      commCompareResults( cursor, expRecs );
   }

   for( var i = 0; i < docs.length; i++ )
   {
      var cursor = cl.find( docs[i] );
      var expRes = { ScanType: "ixscan", IndexName: "aIndex" };
      checkExplain( cursor, expRes );
   }
   commDropCL( db, COMMCSNAME, clName );
}

function checkExplain ( cursor, expRes )
{
   var actRes = cursor.explain().next().toObj();
   for( var k in expRes )
   {
      if( expRes[k] !== actRes[k] )
      {
         throw new Error( "expRes: " + expRes + "\nactRes: " + actRes );
      }
   }
}
