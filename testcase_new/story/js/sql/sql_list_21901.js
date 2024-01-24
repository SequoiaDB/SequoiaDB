/******************************************************************************
@Description seqDB-21901:内置SQL语句查询$LIST_CS
@author liyuanyue
@date 2020-3-19
******************************************************************************/
testConf.skipStandAlone = true;
testConf.csName = COMMCSNAME + "_21901";
testConf.csOpt = { PageSize: 16384, LobPageSize: 131072 };
testConf.clName = COMMCLNAME + "_21901";

main( test );

function test ()
{
   // 使用内置SQL语句查询快照信息
   var cur = db.exec( "select * from $LIST_CS" );
   while( cur.next() )
   {
      var tmpObj = cur.current().toObj();
      if( tmpObj["Name"] === COMMCSNAME + "_21901" )
      {
         var expObj = {
            PageSize: tmpObj["PageSize"], LobPageSize: tmpObj["LobPageSize"], Name: tmpObj["Collection"][0]["Name"]
         };
         var actObj = {
            PageSize: 16384, LobPageSize: 131072, Name: COMMCLNAME + "_21901"
         };
         if( !( commCompareObject( expObj, actObj ) ) )
         {
            throw new Error( "$LIST_CS result error\n" + "expObj :" + JSON.stringify( expObj ) + "\nactObj :" + JSON.stringify( actObj ) );
         }
      }
   }
}