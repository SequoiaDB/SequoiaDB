/******************************************************************************
@Description seqDB-21908:内置SQL语句查询$LIST_SVCTASKS
@author liyuanyue
@date 2020-3-24
******************************************************************************/
testConf.skipStandAlone = true;

main( test );

function test ()
{
   // 内置 sql 语句查询备份列表信息
   var cur = db.exec( "select * from $LIST_SVCTASKS" );
   while( cur.next() )
   {
      var tmpObj = cur.current().toObj();
      // 选取部分字段信息验证
      var snapshotCur = db.list( SDB_LIST_SVCTASKS, { NodeName: tmpObj["NodeName"] } );
      var snapshotCount = 0;
      while( snapshotCur.next() )
      {
         snapshotCount++;
         var snapshotTmpObj = snapshotCur.current().toObj();
         var expObj = {
            TaskID: tmpObj["TaskID"], TaskName: tmpObj["TaskName"]
         };
         var actObj = {
            TaskID: snapshotTmpObj["TaskID"], TaskName: snapshotTmpObj["TaskName"]
         };
         if( !( commCompareObject( expObj, actObj ) ) )
         {
            throw new Error( "$LIST_SVCTASKS result error\n" + "expObj :" + JSON.stringify( expObj ) + "\nactObj :" + JSON.stringify( actObj ) );
         }
      }
      if( snapshotCount != 1 )
      {
         throw new Error( "expected result is 1,but actually result is " + snapshotCount );
      }
   }
}