/******************************************************************************
*@Description : test find special decimal value with match symbol
*               $gt $gte $lt $lte $et $ne $mod $type $elemMatch 
*               $+标识符 $field 
*               seqDB-13993:使用匹配符查询特殊decimal值           
*@author      : Liang XueWang 
******************************************************************************/
main( test )
function test ()
{
   var clName = COMMCLNAME + "_13993";
   var docs = [{ a: { $decimal: "MAX" } },
   { a: { $decimal: "MIN" } },
   { a: { $decimal: "NaN" } }];

   commDropCL( db, COMMCSNAME, clName );
   var cl = commCreateCL( db, COMMCSNAME, clName );
   cl.insert( docs );

   var cursor = cl.find( { a: { $gt: 0 } } );
   var expRecs = [docs[0]];
   commCompareResults( cursor, expRecs );

   cursor = cl.find( { a: { $gte: 0 } } );
   expRecs = [docs[0]];
   commCompareResults( cursor, expRecs );

   // test $lt NaN < 0 ?
   cursor = cl.find( { a: { $lt: 0 } } ).sort( { _id: 1 } );
   expRecs = [docs[1], docs[2]];
   commCompareResults( cursor, expRecs );

   // test $lte NaN < 0 ?
   cursor = cl.find( { a: { $lte: 0 } } ).sort( { _id: 1 } );
   expRecs = [docs[1], docs[2]];
   commCompareResults( cursor, expRecs );

   for( var i = 0; i < docs.length; i++ )
   {
      cursor = cl.find( { a: { $et: docs[i].a } } );
      expRecs = [docs[i]];
      commCompareResults( cursor, expRecs );
   }

   for( var i = 0; i < docs.length; i++ )
   {
      cursor = cl.find( { a: { $ne: docs[i].a } } ).sort( { _id: 1 } );
      var idx1 = ( i + 1 ) % docs.length;
      var idx2 = ( i + 2 ) % docs.length;
      var minIdx = ( idx1 > idx2 ) ? idx2 : idx1;
      var maxIdx = idx1 + idx2 - minIdx;
      expRecs = [docs[minIdx], docs[maxIdx]];
      commCompareResults( cursor, expRecs );
   }

   cursor = cl.find( { a: { $mod: [5, 3] } } );
   expRecs = [];
   commCompareResults( cursor, expRecs );

   cursor = cl.find( { a: { $type: 1, $et: 100 } } ).sort( { _id: 1 } );
   expRecs = [docs[0], docs[1], docs[2]];
   commCompareResults( cursor, expRecs );

   testElemMatch( cl );
   testIdentifier( cl );
   testField( cl );

   commDropCL( db, COMMCSNAME, clName );
}

function testElemMatch ( cl )
{
   var docs = [{ b: { no: { $decimal: "MAX" } } },
   { b: { no: { $decimal: "MIN" } } },
   { b: { no: { $decimal: "NaN" } } }];
   cl.insert( docs );
   for( var i = 0; i < docs.length; i++ )
   {
      var cursor = cl.find( { b: { $elemMatch: { no: docs[i].b.no } } } );
      var expRecs = [docs[i]];
      commCompareResults( cursor, expRecs );
   }
   cl.remove( { b: { $exists: 1 } } );
}

function testIdentifier ( cl )
{
   var docs = [{ b: [{ $decimal: "MAX" }] },
   { b: [{ $decimal: "MIN" }] },
   { b: [{ $decimal: "NaN" }] }];
   cl.insert( docs );
   for( var i = 0; i < docs.length; i++ )
   {
      var cursor = cl.find( { "b.$1": docs[i]["b"][0] } );
      var expRecs = [docs[i]];
      commCompareResults( cursor, expRecs );
   }
   cl.remove( { b: { $exists: 1 } } );
}

function testField ( cl )
{
   var docs = [{ b1: { $decimal: "MAX" }, b2: { $decimal: "MAX" } },
   { b1: { $decimal: "MIN" }, b2: { $decimal: "MIN" } },
   { b1: { $decimal: "NaN" }, b2: { $decimal: "NaN" } },
   { b1: { $decimal: "MAX" }, b2: { $decimal: "MIN" } },
   { b1: { $decimal: "MAX" }, b2: { $decimal: "NaN" } },
   { b1: { $decimal: "MIN" }, b2: { $decimal: "NaN" } }];
   cl.insert( docs );
   var cursor = cl.find( { b1: { $field: "b2" } } ).sort( { _id: 1 } );
   var expRecs = [docs[0], docs[1], docs[2]];
   commCompareResults( cursor, expRecs );
   cl.remove( { b1: { $exists: 1 } } );
}
