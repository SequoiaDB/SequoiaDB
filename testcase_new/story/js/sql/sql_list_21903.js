/******************************************************************************
@Description seqDB-21903:内置SQL语句查询$LIST_GROUP
@author liyuanyue
@date 2020-3-23
******************************************************************************/
testConf.skipStandAlone = true;

main( test );

function test ()
{
   // 使用内置SQL语句查询快照信息
   var cur = db.exec( "select * from $LIST_GROUP" );
   while( cur.next() )
   {
      var tmpObj = cur.current().toObj();
      // snapshot 带条件查询快照信息
      var snapshotCur = db.list( SDB_LIST_GROUPS, { GroupName: tmpObj["GroupName"] } );
      var snapshotCount = 0;
      // 对比查询信息是否一致
      while( snapshotCur.next() )
      {
         snapshotCount++;
         var snapshotTmpObj = snapshotCur.current().toObj();
         var expObj = {
            GroupID: tmpObj["GroupID"], PrimaryNode: tmpObj["PrimaryNode"], Role: tmpObj["Role"],
            SecretID: tmpObj["SecretID"], Status: tmpObj["Status"],
            Version: tmpObj["Version"], oid: tmpObj["_id"]["$oid"]
         };
         var actObj = {
            GroupID: snapshotTmpObj["GroupID"], PrimaryNode: snapshotTmpObj["PrimaryNode"],
            Role: snapshotTmpObj["Role"], SecretID: snapshotTmpObj["SecretID"],
            Status: snapshotTmpObj["Status"], Version: snapshotTmpObj["Version"], oid: snapshotTmpObj["_id"]["$oid"]
         };
         if( !( commCompareObject( expObj, actObj ) ) )
         {
            throw new Error( "$LIST_GROUP result error\n" + "expObj :" + JSON.stringify( expObj ) + "\nactObj :" + JSON.stringify( actObj ) );
         }
      }
      if( snapshotCount != 1 )
      {
         throw new Error( "expected result is 1,but actually result is " + snapshotCount );
      }
   }
}