/******************************************************************************
 * @Description   : Test Backup SDB by use default argument.[db.backupOffline()]
 * @Author        : xiaojun Hu
 * @CreateTime    : 2014.06.20
 * @LastEditTime  : 2022.01.20
 * @LastEditors   : 钟子明
 ******************************************************************************/

main( test );

function test ()
{
   var backupName = "";   // in default backup, use time as backup name
   var alreadStart = false;
   var clName = COMMCLNAME + "_cl14050";
   commDropCL( db, csName, clName, true, true );
   var cl = commCreateCL( db, csName, clName, { ReplSize: -1 }, true, false );
   bakInsertData( cl );
   bakRemoveBackups( db, backupName, true );
   // Backup don't have options [Test]. -67:Backup always begin
   bakBackup( db );
   // check backup operation is success or not
   try
   {
      checkBackupInfo( db, "check default backup failed" );
   }
   finally
   {
      bakRemoveBackups( db, backupName, alreadStart );
   }
   // Drop collection
   commDropCL( db, csName, clName, true, false, "Drop CL in the end" );
}
