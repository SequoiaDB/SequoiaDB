/******************************************************************************
 * @Description   : Test backupOffline, specify["GroupName","Path","Description"]
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
   var clName = COMMCLNAME + "_cl14051";
   // Clear the collection space in the beginning
   commDropCL( db, COMMCSNAME, clName, true, true );
   var cl = commCreateCL( db, COMMCSNAME, clName, { ReplSize: -1 }, true, false );
   // Insert data to SDB
   bakInsertData( cl );
   // Clear backup in the begnning
   bakRemoveBackups( db, CHANGEDPREFIX, true );

   // Backup specify the GroupName/Path/Description
   if( false == commIsStandalone( db ) )
   {
      var groups = commGetGroups( db );
      for( var i = 0; i < groups.length; ++i )
      {
         var j = groups[i][0].PrimaryPos;
         path = groups[i][j].dbpath;
         for( var j = 1; j < groups[i].length; ++j )
         {
            if( path !== groups[i][j].dbpath )
            {
               path = "";
               break;
            }
         }
         assert.notEqual( undefined, path );

         var bakName = CHANGEDPREFIX + getDateString();
         bakName += i;
         var backup = { "Description": "backup description", "EnsureInc": false, "OverWrite": false };
         backup["Name"] = bakName;
         if( path !== "" )
         {
            backup["Path"] = path;
         }

         bakBackup( db, backup );
         checkBackupInfo( db, "check description backup failed", bakName, path, alreadStart );
         bakRemoveBackups( db, bakName, alreadStart );
      }
   }
   else   // run mode standalone
   {
      var bakName = COMMCSNAME + "bakstandalone";
      var backup = { "Description": "backup description", "EnsureInc": false, "OverWrite": false };
      backup["Name"] = bakName;
      bakBackup( db, backup );
      checkBackupInfo( db, "check description backup failed", bakName, path, alreadStart );
      // backup and check over, then remove backup
      bakRemoveBackups( db, bakName, alreadStart, path );
   }

   // Clear backup in the end
   commDropCL( db, COMMCSNAME, clName, true, false, "Drop CL in the " );
}