/******************************************************************************
 * @Description   : Test backupOffline, specify["OverWrite"]
 * @Author        : xiaojun Hu
 * @CreateTime    : 2014.06.20
 * @LastEditTime  : 2022.01.21
 * @LastEditors   : 钟子明
 ******************************************************************************/

main( test );

function test ()
{
   var alreadStart = false;
   var path = "";
   var clName = COMMCLNAME + "_cl14055";
   commDropCL( db, csName, clName, true, true, "Drop CL in the beginning" );
   var cl = commCreateCL( db, COMMCSNAME, clName, { ReplSize: -1 }, true, false );
   bakInsertData( cl );
   bakRemoveBackups( db, CHANGEDPREFIX, true );
   // Backup specify the GroupName/Path/Description
   var runMode = commIsStandalone( db );
   // In standalone,GroupName can be specified all kinds.{"GroupName":""/"abcde"}
   var groups = commGetGroups( db );
   if( false == runMode )
   {
      for( var i = 0; i < groups.length; ++i )
      {
         var bakName = CHANGEDPREFIX + "BAK" + i;
         var backup = { EnsureInc: false, OverWrite: false, Description: "backup description" };
         backup["GroupName"] = groups[i][0].GroupName;
         backup["Name"] = bakName;
         bakBackup( db, backup );
         bakBackupByCheckError( db, backup );
         backup["OverWrite"] = true;
         bakBackup( db, backup );
         checkBackupInfo( db, "", bakName, path, true );
      }
      bakRemoveBackups( db, bakName, alreadStart, path );
   }
   else
   {
      var bakName = COMMCLNAME + "BAKstandalone";
      var bakName = CHANGEDPREFIX + "BAK" + i;
      var backup = { EnsureInc: false, OverWrite: false, Description: "backup description" };
      backup["Name"] = bakName;
      bakBackup( db, backup );
      bakBackupByCheckError( db, backup );
      backup["OverWrite"] = true;
      bakBackup( db, backup );
      checkBackupInfo( db, "", bakName, path, true );
      bakRemoveBackups( db, bakName, alreadStart, path );
   }

   commDropCL( db, COMMCSNAME, clName, true, false, "Drop CL in the end" );
}

function bakBackupByCheckError ( db, backUpOpt )
{
   try
   {
      bakBackup( db, backUpOpt );
   }
   catch( e )
   {
      if( SDB_BAR_BACKUP_EXIST == e.message ) 
      {
         alreadStart = true;
      }
      else
      {
         commDropCL( db, COMMCSNAME, clName, true, false, "Drop CL in the end" );
         throw e;
      }
   }
}