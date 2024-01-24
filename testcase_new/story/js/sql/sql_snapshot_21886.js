/******************************************************************************
@Description seqDB-21886:内置SQL语句查询$SNAPSHOT_CS
@author liyuanyue
@date 2020-3-19
******************************************************************************/
testConf.skipStandAlone = true;
testConf.csName = COMMCSNAME + "_21886";
testConf.csOpt = { PageSize: 16384, LobPageSize: 131072 };
testConf.clName = COMMCLNAME + "_21886";

main( test );

function test ()
{
   // 使用内置SQL语句查询快照信息
   var cur = db.exec( "select * from $SNAPSHOT_CS" );
   while( cur.next() )
   {
      var tmpObj = cur.current().toObj();
      if( tmpObj["Name"] === COMMCSNAME + "_21886" )
      {
         var actObj = {
            PageSize: tmpObj["PageSize"], LobPageSize: tmpObj["LobPageSize"],
            TotalRecords: tmpObj["TotalRecords"], TotalLobSize: tmpObj["TotalLobSize"]
         };
         var expObj = {
            PageSize: 16384, LobPageSize: 131072,
            TotalRecords: 0, TotalLobSize: 0
         };
         if( !( commCompareObject( expObj, actObj ) ) )
         {
            throw new Error( "$SNAPSHOT_CS result error\n" + "expObj :" + JSON.stringify( expObj ) + "\nactObj :" + JSON.stringify( actObj ) );
         }
      }
   }
}