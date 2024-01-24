/******************************************************************************
@Description seqDB-21894:内置SQL语句查询$SNAPSHOT_CONFIGS
@author liyuanyue
@date 2020-3-23
******************************************************************************/
testConf.skipStandAlone = true;

main( test );

function test ()
{
   // 使用内置SQL语句查询快照信息
   var cur = db.exec( "select * from $SNAPSHOT_CONFIGS" );
   while( cur.next() )
   {
      var tmpObj = cur.current().toObj();
      // snapshot 带条件查询快照信息
      var snapshotCur = db.snapshot( SDB_SNAP_CONFIGS, { NodeName: tmpObj["NodeName"] } );
      var snapshotCount = 0;
      while( snapshotCur.next() )
      {
         snapshotCount++;
         var snapshotTmpObj = snapshotCur.current().toObj();
         var expObj = {
            confpath: tmpObj["confpath"], svcname: tmpObj["svcname"],
            role: tmpObj["role"], catalogaddr: tmpObj["catalogaddr"]
         };
         var actObj = {
            confpath: snapshotTmpObj["confpath"], svcname: snapshotTmpObj["svcname"],
            role: snapshotTmpObj["role"], catalogaddr: snapshotTmpObj["catalogaddr"]
         };
         if( !( commCompareObject( expObj, actObj ) ) )
         {
            throw new Error( "$SNAPSHOT_CONFIGS result error\n" + "expObj :" + JSON.stringify( expObj ) + "\nactObj :" + JSON.stringify( actObj ) );
         }
      }
      if( snapshotCount != 1 )
      {
         throw new Error( "result is 1, but actually result is " + snapshotCount );
      }
   }
}
