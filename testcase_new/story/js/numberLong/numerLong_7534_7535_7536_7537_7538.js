/*******************************************************************************
*@Description : seqDB-7534:shell_用普通方式查询numberLong数据
seqDB-7535:shell_使用匹配符查询numberLong类型
seqDB-7536:shell_使用选择符查询numberLong类型
seqDB-7537:shell_使用更新符更新numberLong类型
seqDB-7538:shell_索引值为numberLong类型
*@Modify List : 2016-3-28  Ting YU  Init
*******************************************************************************/
main( test );

function test ()
{
   var clName = COMMCLNAME + "_7534";
   var indexName = "idx1";

   commDropCL( db, COMMCSNAME, clName );
   var cl = commCreateCL( db, COMMCSNAME, clName );
   cl.createIndex( indexName, { a: 1 } );
   istAndQuery( cl, indexName );
   commDropCL( db, COMMCSNAME, clName );
}

function istAndQuery ( cl, indexName )
{
   var recs = [];
   var longVal = -9007199254740000;
   recs.push( { a: { $numberLong: longVal.toString() } } );
   recs.push( { a: 9007199254740992 } );
   cl.insert( recs );

   var rc = cl.find( { a: longVal } );
   var expRecs = [{ a: longVal }];
   commCompareResults( rc, expRecs );
   checkExplain( rc, indexName );

   var rc = cl.find( { a: { $type: 1, $et: 18 } } );
   var expRecs = [{ a: longVal }];
   commCompareResults( rc, expRecs );

   var rc = cl.find( { a: { $type: 1, $et: 18 } }, { a: { $subtract: 10 } } );
   var expRecs = [{ a: ( longVal - 10 ) }];
   commCompareResults( rc, expRecs );

   var rc = cl.find( { a: 9007199254740992 }, { a: { $cast: "int64" } } );
   var expRecs = [{ a: { $numberLong: "9007199254740992" } }];
   commCompareResults( rc, expRecs );

   cl.update( { $inc: { a: 1 } }, { a: { $lt: 0 } } );
   var rc = cl.find( { a: { $lt: 0 } } );
   var expRecs = [{ a: ( longVal + 1 ) }];
   commCompareResults( rc, expRecs );
}
