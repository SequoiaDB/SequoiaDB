/******************************************************************************
 * @Description   : Test db.backupOffline().Specify [IsSubDir].{IsSubDir:false}
 * @Author        : xiaojun Hu
 * @CreateTime    : 2014.06.20
 * @LastEditTime  : 2022.01.20
 * @LastEditors   : 钟子明
 ******************************************************************************/

main( test );

function test ()
{
   var alreadStart = false;
   var path = "";
   var clName = COMMCLNAME + "_cl14054";
   commDropCL( db, COMMCSNAME, clName, true, true );
   var cl = commCreateCL( db, COMMCSNAME, clName, { ReplSize: -1 }, true, false );
   bakInsertData( cl );
   bakRemoveBackups( db, CHANGEDPREFIX, true );
   // Backup Offline specify the [groups]
   if( false == commIsStandalone( db ) )
   {
      var groups = commGetGroups( db );
      for( var i = 0; i < groups.length; ++i )
      {
         var bakName = CHANGEDPREFIX + "_bak_" + i;
         var backup = { "IsSubDir": true };
         backup["Name"] = bakName;
         backup["Gruop"] = groups[i][0].GroupName;
         bakBackup( db, backup );
         checkBackupInfo( db, "", bakName, path, alreadStart );
         bakRemoveBackups( db, bakName, alreadStart, path );
      }
   }
   else
   {
      var bakName = CHANGEDPREFIX + "bakStandalone";
      var backup = { "IsSubDir": true };
      backup["Name"] = bakName;
      bakBackup( db, backup );
      checkBackupInfo( db, "", bakName, path, alreadStart );
      bakRemoveBackups( db, bakName, alreadStart, path );
   }
   commDropCL( db, COMMCSNAME, clName, true, false, "Drop CL in the end" );
}
