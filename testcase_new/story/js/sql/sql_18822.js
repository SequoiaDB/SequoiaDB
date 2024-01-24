/************************************
*@Description: seqDB-18822 聚集查询where子句使用“<>”判断
*@Author     : 2019.07.15 yinzhen 
**************************************/

main( test );
function test ()
{
   var clName = COMMCLNAME + "_sql_18822";
   commDropCL( db, COMMCSNAME, clName, true, true );
   var cl = commCreateCL( db, COMMCSNAME, clName );

   cl.insert( [{ a: 1, b: "201902" }, { a: 1, b: "201902" },
   { a: 2, b: "201902" }, { a: 2, b: "201902" },
   { a: 3, b: "201901" }, { a: 3, b: "201902" }] );

   var cursor = db.exec( "select * from (select min(b) as min, max(b) as max from "
      + COMMCSNAME + "." + clName + " group by a) as T where T.min <> T.max" );
   var actList = [];
   while( cursor.next() )
   {
      var obj = cursor.current().toObj();
      actList.push( obj );
   }

   // check result
   if( actList.length !== 1 )
   {
      throw new Error( "main RESULT ERROR check return result length" +
         "{\"min\": \"201901\", \"max\": \"201902\"}" +
         JSON.stringify( actList ) );
   }
   var expObj = { "min": "201901", "max": "201902" };
   if( JSON.stringify( expObj ) !== JSON.stringify( actList[0] ) )
   {
      throw new Error( "main RESULT ERROR check return result" +
         "{\"min\": \"201901\", \"max\": \"201902\"}" +
         JSON.stringify( actList ) );
   }

   commDropCL( db, COMMCSNAME, clName, true, true );
}