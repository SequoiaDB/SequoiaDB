/******************************************************************************
*@Description : test find special decimal value with aggregate symbol
*               $sort $group($addtoset $max $min $avg $sum)
*               seqDB-13996:使用聚集符查询特殊decimal值           
*@author      : Liang XueWang 
******************************************************************************/
main( test )
function test ()
{
   var clName = COMMCLNAME + "_13996";
   var docs = [{ a: { $decimal: "MAX" } },
   { a: { $decimal: "MIN" } },
   { a: { $decimal: "NaN" } }];

   commDropCL( db, COMMCSNAME, clName );
   var cl = commCreateCL( db, COMMCSNAME, clName );
   cl.insert( docs );

   var cursor = cl.aggregate( { $sort: { a: 1 } } );
   var expRecs = [{ a: { $decimal: "NaN" } },
   { a: { $decimal: "MIN" } },
   { a: { $decimal: "MAX" } }];
   commCompareResults( cursor, expRecs );

   // insert MAX MIN NAN to diff group to test $addtoset
   cl.remove();
   docs = [{ gid: 3, a: { $decimal: "MAX" } },
   { gid: 2, a: { $decimal: "NaN" } },
   { gid: 1, a: { $decimal: "MIN" } }];
   cl.insert( docs );

   cursor = cl.aggregate( { $group: { _id: "$gid", b: { $addtoset: "$a" } } } );
   expRecs = [{ b: [{ $decimal: "MIN" }] },
   { b: [{ $decimal: "NaN" }] },
   { b: [{ $decimal: "MAX" }] }];
   commCompareResults( cursor, expRecs );

   // insert MAX MIN NAN to same group to test $max $min $avg $sum
   cl.remove();
   docs = [{ gid: 1, a: { $decimal: "MAX" } },
   { gid: 1, a: { $decimal: "NaN" } },
   { gid: 1, a: { $decimal: "MIN" } }];
   cl.insert( docs );

   cursor = cl.aggregate( { $group: { _id: "$gid", b: { $max: "$a" } } } );
   expRecs = [{ b: { $decimal: "MAX" } }];
   commCompareResults( cursor, expRecs );

   cursor = cl.aggregate( { $group: { _id: "$gid", b: { $min: "$a" } } } );
   expRecs = [{ b: { $decimal: "NaN" } }];
   commCompareResults( cursor, expRecs );

   cursor = cl.aggregate( { $group: { _id: "$gid", b: { $avg: "$a" } } } );
   expRecs = [{ b: { $decimal: "NaN" } }];
   commCompareResults( cursor, expRecs );

   cursor = cl.aggregate( { $group: { _id: "$gid", b: { $sum: "$a" } } } );
   expRecs = [{ b: { $decimal: "NaN" } }];
   commCompareResults( cursor, expRecs );

   commDropCL( db, COMMCSNAME, clName );
}
