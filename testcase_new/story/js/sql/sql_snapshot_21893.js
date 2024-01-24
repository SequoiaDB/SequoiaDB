/******************************************************************************
@Description seqDB-21893:内置SQL语句查询$SNAPSHOT_HEALTH
@author liyuanyue
@date 2020-3-23
******************************************************************************/
testConf.skipStandAlone = true;

main( test );

function test ()
{
   // 使用内置SQL语句查询快照信息
   var cur = db.exec( "select * from $SNAPSHOT_HEALTH" );
   while( cur.next() )
   {
      var tmpObj = cur.current().toObj();
      // snapshot 带条件查询快照信息
      var snapshotCur = db.snapshot( SDB_SNAP_HEALTH, { NodeName: tmpObj["NodeName"] } );
      var snapshotCount = 0;
      while( snapshotCur.next() )
      {
         snapshotCount++;
         var snapshotTmpObj = snapshotCur.current().toObj();
         var expObj = {
            IsPrimary: tmpObj["IsPrimary"], NodeID: JSON.stringify( tmpObj["NodeID"] ),
            CoreFileSize: tmpObj["Ulimit"]["CoreFileSize"], VirtualMemory: tmpObj["Ulimit"]["VirtualMemory"],
            OpenFiles: tmpObj["Ulimit"]["OpenFiles"], NumProc: tmpObj["Ulimit"]["NumProc"],
            FileSize: tmpObj["Ulimit"]["FileSize"], VMLimit: tmpObj["Memory"]["VMLimit"],
            TotalSpace: tmpObj["Disk"]["TotalSpace"], TotalNum: tmpObj["FileDesp"]["TotalNum"],
         };
         var actObj = {
            IsPrimary: snapshotTmpObj["IsPrimary"], NodeID: JSON.stringify( snapshotTmpObj["NodeID"] ),
            CoreFileSize: snapshotTmpObj["Ulimit"]["CoreFileSize"], VirtualMemory: snapshotTmpObj["Ulimit"]["VirtualMemory"],
            OpenFiles: snapshotTmpObj["Ulimit"]["OpenFiles"], NumProc: snapshotTmpObj["Ulimit"]["NumProc"],
            FileSize: snapshotTmpObj["Ulimit"]["FileSize"], VMLimit: snapshotTmpObj["Memory"]["VMLimit"],
            TotalSpace: snapshotTmpObj["Disk"]["TotalSpace"], TotalNum: snapshotTmpObj["FileDesp"]["TotalNum"],
         };
         if( !( commCompareObject( expObj, actObj ) ) )
         {
            throw new Error( "$SNAPSHOT_HEALTH result error\n" + "expObj :" + JSON.stringify( expObj ) + "\nactObj :" + JSON.stringify( actObj ) );
         }
      }
      if( snapshotCount != 1 )
      {
         throw new Error( "result is 1, but actually result is " + snapshotCount );
      }
   }
}
