/******************************************************************************
 * @Description   : seqDB-24233:不同类型字段做 hash join 关联查询
 * @Author        : liuli
 * @CreateTime    : 2021.05.24
 * @LastEditTime  : 2021.05.27
 * @LastEditors   : liuli
 ******************************************************************************/
main( test );
function test ()
{
   var clName1 = "clName_24233_1";
   var clName2 = "clName_24233_2";

   commDropCL( db, COMMCSNAME, clName1 );
   commDropCL( db, COMMCSNAME, clName2 );
   var dbcl1 = commCreateCL( db, COMMCSNAME, clName1 );
   var dbcl2 = commCreateCL( db, COMMCSNAME, clName2 );

   dbcl1.insert( { a: 1 } );
   dbcl2.insert( { b: { "$numberLong": "1" } } );

   var expRecs = [{ "cnt": 1 }];
   var cursor = db.exec( "select count(c1.a) as cnt from " + COMMCSNAME + "." + clName1 + " as c1 inner join " + COMMCSNAME + "." + clName2 + " as c2 on c1.a = c2.b /*+use_hash()*/" )
   commCompareResults( cursor, expRecs );

   commDropCL( db, COMMCSNAME, clName1 );
   commDropCL( db, COMMCSNAME, clName2 );
}