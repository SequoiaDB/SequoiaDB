/******************************************************************************
 * @Description   : 备份恢复固定集合 seqDB-11833
 * @Author        : liuxiaoxuan
 * @CreateTime    : 2019.07.17
 * @LastEditTime  : 2023.08.18
 * @LastEditors   : liuli
 ******************************************************************************/

main( test );

function test ()
{
   if( commIsStandalone( db ) )
   {
      return;
   }

   var csName = COMMCAPPEDCSNAME + "_11833";
   commDropCS( db, csName, true, "drop CS in the beginning" );
   commCreateCS( db, csName, true, "", { Capped: true } );
   var options = { Capped: true, Size: 1, Max: 100, AutoIndexId: false };
   var dbcl = commCreateCL( db, csName, clName, options, false, false, "create cappedCL" );

   var groupNames = commGetCLGroups( db, csName + "." + clName );
   var backupName = CHANGEDPREFIX + "_backup11833";
   bakInsertData( dbcl );
   bakRemoveBackups( db, backupName, true );
   // back up
   bakBackup( db, { "Name": backupName, GroupName: groupNames } );

   var nodeinfo;
   var cmd = new Remote( COORDHOSTNAME, CMSVCNAME ).getCmd();
   try
   {
      checkBackupInfo( db, "check default backup failed", backupName );
      var rg = db.getRG( groupNames[0] ).getDetail().next().toObj();
      for( var i = 0; i < rg.Group.length; ++i )
      {
         if( rg.PrimaryNode === rg.Group[i].NodeID )
         {
            var hostName = rg.Group[i].HostName;
            var svcName = rg.Group[i].Service[0].Name;
            var dbPath = rg.Group[i].dbpath;
            nodeinfo = new nodeInfo( groupNames[0], hostName, svcName, dbPath );
            cmd = getCmdByHostName( cmd, hostName );
            break;
         }
      }
      var bakInfo = new backUpInfo( backupName, nodeinfo.dbPath + "bakfile" );
      // restore
      sdbRestore( db, cmd, bakInfo, nodeinfo );
   }
   finally
   {
      bakRemoveBackups( db, backupName, false );
   }
   commDropCS( db, csName, true, "drop CS in the end" );
}
