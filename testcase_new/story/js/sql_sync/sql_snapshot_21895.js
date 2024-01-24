/******************************************************************************
@Description seqDB-21895:内置SQL语句查询$SNAPSHOT_SVCTASKS
@author liyuanyue
@date 2020-3-23
******************************************************************************/
testConf.skipStandAlone = true;

main( test );

function test ()
{
   // 使用 snapshot 语句查询快照信息
   var cur = db.snapshot( SDB_SNAP_SVCTASKS, { RawData: true } );
   while( cur.next() )
   {
      var tmpObj = cur.current().toObj();
      // 将内置 sql 执行结果累计验证
      var execObj = db.exec( "select * from $SNAPSHOT_SVCTASKS where NodeName='" + tmpObj.NodeName + "'" ).current().toObj();
      var actObj = {
         TotalIndexRead: tmpObj["TotalIndexRead"], TotalDataWrite: tmpObj["TotalDataWrite"], TotalIndexWrite: tmpObj["TotalIndexWrite"],
         TotalUpdate: tmpObj["TotalUpdate"], TotalDelete: tmpObj["TotalDelete"], TotalInsert: tmpObj["TotalInsert"], TotalWrite: tmpObj["TotalWrite"]
      };
      var expObj = {
         TotalIndexRead: execObj["TotalIndexRead"], TotalDataWrite: execObj["TotalDataWrite"], TotalIndexWrite: execObj["TotalIndexWrite"],
         TotalUpdate: execObj["TotalUpdate"], TotalDelete: execObj["TotalDelete"], TotalInsert: execObj["TotalInsert"], TotalWrite: execObj["TotalWrite"]

      };
      if( !( commCompareObject( expObj, actObj ) ) )
      {
         throw new Error( "$SNAPSHOT_SVCTASKS result error on " + tmpObj.NodeName + "\n" + "expObj :" + JSON.stringify( expObj ) + "\nactObj :" + JSON.stringify( actObj ) );
      }
   }
}
