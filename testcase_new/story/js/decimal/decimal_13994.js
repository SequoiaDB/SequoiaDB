/******************************************************************************
*@Description : test find special decimal value with func symbol
*               $abs $ceiling $floor $mod $add $subtract 
*               $multiply $divide $cast
*               seqDB-13994:使用函数操作符查询特殊decimal值           
*@author      : Liang XueWang 
******************************************************************************/
main( test )
function test ()
{
   var clName = COMMCLNAME + "_13994";
   var docs = [{ a: { $decimal: "MAX" } },
   { a: { $decimal: "MIN" } },
   { a: { $decimal: "NaN" } }];

   commDropCL( db, COMMCSNAME, clName );
   var cl = commCreateCL( db, COMMCSNAME, clName );
   cl.insert( docs );

   // test $abs -6 ?
   testFindData( cl, {}, { a: { $abs: 1 } }, -6 );
   testFindData( cl, {}, { a: { $ceiling: { $decimal: "MAX" } } }, -6 );
   cursor = cl.find( {}, { a: { $ceiling: 1 } } );
   expRecs = [{ a: { $decimal: "NaN" } },
   { a: { $decimal: "NaN" } },
   { a: { $decimal: "NaN" } }];
   commCompareResults( cursor, expRecs );

   cursor = cl.find( {}, { a: { $floor: 1 } } );
   commCompareResults( cursor, expRecs );

   cursor = cl.find( {}, { a: { $mod: 2 } } );
   commCompareResults( cursor, expRecs );

   cursor = cl.find( {}, { a: { $add: 1 } } );
   commCompareResults( cursor, expRecs );

   cursor = cl.find( {}, { a: { $subtract: 1 } } );
   commCompareResults( cursor, expRecs );

   cursor = cl.find( {}, { a: { $multiply: 1 } } );
   commCompareResults( cursor, expRecs );

   cursor = cl.find( {}, { a: { $divide: 1 } } );
   commCompareResults( cursor, expRecs );

   testCast( cl );
   commDropCL( db, COMMCSNAME, clName );
}

function testFindData ( cl, cond, sel, errno )
{
   try
   {
      var cursor = cl.find( cond, sel );
      cursor.next();
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

function testCast ( cl )
{
   var cursor = cl.find( {}, { a: { $cast: "minkey" } } );
   var expRecs = [{ a: { $minKey: 1 } },
   { a: { $minKey: 1 } },
   { a: { $minKey: 1 } }];
   commCompareResults( cursor, expRecs );

   //cast Double

   cursor = cl.find( {}, { a: { $cast: "double" } } ).sort( { _id: 1 } );
   expRecs = [{ a: 0 },
   { a: 0 },
   { a: NaN }];
   commCompareResults( cursor, expRecs );

   cursor = cl.find( {}, { a: { $cast: "string" } } ).sort( { _id: 1 } );
   expRecs = [{ a: "MAX" },
   { a: "MIN" },
   { a: "NaN" }];
   commCompareResults( cursor, expRecs );

   cursor = cl.find( {}, { a: { $cast: "bool" } } ).sort( { _id: 1 } );
   expRecs = [{ a: true },
   { a: true },
   { a: true }];
   commCompareResults( cursor, expRecs );

   cursor = cl.find( {}, { a: { $cast: "date" } } ).sort( { _id: 1 } );
   expRecs = [{ a: null },
   { a: null },
   { a: null }];
   commCompareResults( cursor, expRecs );

   cursor = cl.find( {}, { a: { $cast: "null" } } ).sort( { _id: 1 } );
   expRecs = [{ a: null },
   { a: null },
   { a: null }];
   commCompareResults( cursor, expRecs );

   cursor = cl.find( {}, { a: { $cast: "int32" } } ).sort( { _id: 1 } );
   expRecs = [{ a: 0 },
   { a: 0 },
   { a: 0 }];
   commCompareResults( cursor, expRecs );

   cursor = cl.find( {}, { a: { $cast: "timestamp" } } ).sort( { _id: 1 } );
   expRecs = [{ a: null },
   { a: null },
   { a: null }];
   commCompareResults( cursor, expRecs );

   cursor = cl.find( {}, { a: { $cast: "int64" } } ).sort( { _id: 1 } );
   expRecs = [{ a: 0 },
   { a: 0 },
   { a: 0 }];
   commCompareResults( cursor, expRecs );

   cursor = cl.find( {}, { a: { $cast: "maxkey" } } ).sort( { _id: 1 } );
   expRecs = [{ a: { $maxKey: 1 } },
   { a: { $maxKey: 1 } },
   { a: { $maxKey: 1 } }];
   commCompareResults( cursor, expRecs );
}
