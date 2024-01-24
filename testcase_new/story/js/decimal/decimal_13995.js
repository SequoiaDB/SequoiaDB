/******************************************************************************
*@Description : test find special decimal value with update symbol
*               $inc $addtoset
*               seqDB-13995:使用更新符更新特殊decimal值           
*@author      : Liang XueWang 
******************************************************************************/
main( test )
function test ()
{
   var clName = COMMCLNAME + "_13995";
   var docs = [{ a: { $decimal: "MAX" } },
   { a: { $decimal: "MIN" } },
   { a: { $decimal: "NaN" } }];

   commDropCL( db, COMMCSNAME, clName );
   var cl = commCreateCL( db, COMMCSNAME, clName );
   cl.insert( docs );

   testUpdateData( cl, { a: { $inc: 1 } }, -6 );

   testAddToSet( cl );
   commDropCL( db, COMMCSNAME, clName );
}

function testUpdateData ( cl, rule, errno )
{
   try
   {
      cl.update( rule );
      throw new Error( "need throw error" );
   }
   catch( e )
   {
      if( e.message != errno )
      {
         throw e;
      }
   }
}

function testAddToSet ( cl )
{
   var doc = { b: [] };
   cl.insert( doc );
   cl.update( {
      $addtoset: {
         b: [{ "$decimal": "MAX" },
         { "$decimal": "MIN" },
         { "$decimal": "NaN" }]
      }
   },
      { b: { $exists: 1 } } );
   var cursor = cl.find( { b: { $exists: 1 } } );
   var expRecs = [{
      b: [{ "$decimal": "NaN" },
      { "$decimal": "MIN" },
      { "$decimal": "MAX" }]
   }];
   commCompareResults( cursor, expRecs );
}
