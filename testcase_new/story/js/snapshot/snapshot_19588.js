/******************************************************************************
*@Description : seqDB-19588:检查snapshot(SDB_SNAP_SVCTASKS)快照信息
*               TestLink : seqDB-19588
*@author      : wangkexin
*@Date        : 2019-09-27
******************************************************************************/
main( test );

function test ()
{
   var fields = ["TaskName", "TaskID", "Time", "TotalContexts", "TotalDataRead", "TotalIndexRead", "TotalDataWrite", "TotalIndexWrite", "TotalUpdate", "TotalDelete", "TotalInsert", "TotalSelect", "TotalRead", "TotalWrite"];
   var cursor = db.snapshot( SDB_SNAP_SVCTASKS );
   while( cursor.next() )
   {
      var obj = cursor.current().toObj();
      for( var index in fields )
      {
         if( !obj.hasOwnProperty( fields[index] ) )
         {
            throw new Error( fields[index] + "does not exist in own properties!" );
         }
      }
   }
}
