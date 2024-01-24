/******************************************************************************
@Description seqDB-21907:内置SQL语句查询$LIST_BACKUP
@author liyuanyue
@date 2020-3-24
******************************************************************************/
testConf.skipStandAlone = true;

// main( test );

function test ()
{
   var groupName = commGetDataGroupNames( db )[0];
   var groupID = db.list( SDB_LIST_GROUPS, { GroupName: groupName } ).current().toObj()["GroupID"];

   var options = { GroupID: groupID, GroupName: groupName, OverWrite: true, Name: "backup_21907" };
   db.backup( options );

   // 内置 sql 语句查询备份列表信息
   var cur = db.exec( "select * from $LIST_BACKUP" );
   while( cur.next() )
   {
      var tmpObj = cur.current().toObj();
      // 选取部分字段信息验证
      var snapshotCur = db.list( SDB_LIST_BACKUPS, { ID: tmpObj["ID"] } );
      var snapshotCount = 0;
      while( snapshotCur.next() )
      {
         snapshotCount++;
         var snapshotTmpObj = snapshotCur.current().toObj();
         var expObj = {
            Version: tmpObj["Version"], Name: tmpObj["Name"], NodeName: tmpObj["NodeName"],
            GroupName: tmpObj["GroupName"], StartTime: tmpObj["StartTime"], LastLSNCode: tmpObj["LastLSNCode"]
         };
         var actObj = {
            Version: snapshotTmpObj["Version"], Name: snapshotTmpObj["Name"],
            NodeName: snapshotTmpObj["NodeName"], GroupName: snapshotTmpObj["GroupName"],
            StartTime: snapshotTmpObj["StartTime"], LastLSNCode: snapshotTmpObj["LastLSNCode"]
         };
         if( !( commCompareObject( expObj, actObj ) ) )
         {
            throw new Error( "$LIST_BACKUP result error\n" + "expObj :" + JSON.stringify( expObj ) + "\nactObj :" + JSON.stringify( actObj ) );
         }
      }
      if( snapshotCount != 1 )
      {
         throw new Error( "expected result is 1,but actually result is " + snapshotCount );
      }
   }

   var options = { GroupID: groupID, GroupName: groupName, Name: "backup_21907" };
   db.removeBackup( options );
}
