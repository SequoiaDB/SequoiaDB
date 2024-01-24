/******************************************************************************
*@Description : seqDB-19696:检查list( SDB_LIST_BACKUPS )列表信息
*               TestLink : seqDB-19696
*@author      : wangkexin
*@Date        : 2019-09-27
******************************************************************************/
testConf.skipStandAlone = true;

main( test );

function test ()
{
   var groupName = commGetGroups( db )[0][0].GroupName;
   var backupName = "backup_19696";

   db.backup( { Name: backupName, GroupName: groupName } );
   try
   {
      var cursor = db.list( SDB_LIST_BACKUPS, { "Name": backupName } );
      while( cursor.next() )
      {
         var backup = cursor.current().toObj();
         for( var key in backup )
         {
            if( backup[key] == null )
            {
               throw new Error( "backup[" + i + "] is null" );
            }
         }
      }
   }
   finally
   {
      db.removeBackup( { Name: backupName } );
   }
}

