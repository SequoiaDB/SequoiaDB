/******************************************************************************
*@Description : seqDB-18824:as定义集合别名，使用别名查询记录 
*@Author      : 2019-07-15  XiaoNi Huang init
******************************************************************************/
main( test );
function test ()
{
   var csName = COMMCSNAME;
   var clName = "cl18824";

   commDropCL( db, csName, clName, true, true, "Failed to drop CL in the pre-condition." );
   var cl = commCreateCL( db, csName, clName );
   var recs = [{ a: 1, b: 1 }, { a: 1, b: 2 }];
   cl.insert( recs );

   var sql = "select T.a, T.b from " + csName + "." + clName + " as T where T.a = T.b";
   var cursor = db.exec( sql );
   var recsNum = 0;
   var actRecs;
   while( cursor.next() )
   {
      actRecs = cursor.current().toObj();
      recsNum++;
   }

   var expRecs = { a: 1, b: 1 };
   if( 1 !== recsNum || JSON.stringify( expRecs ) !== JSON.stringify( actRecs ) )
   {
      throw new Error( "main [check index]" +
         "[recsNum = 1, expRecs = " + JSON.stringify( expRecs ) + "]" +
         "[recsNum = " + recsNum + ", actRecs = " + JSON.stringify( actRecs ) + "]" );
   }

   commDropCL( db, csName, clName, false, false, "Failed to drop CL in the end-condition." );
}