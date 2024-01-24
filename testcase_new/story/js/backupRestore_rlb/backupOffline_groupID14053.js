/******************************************************************************
 * @Description   : Test db.backupOffline().Specify [GroupID].{GroupID:[1000,1001]}
 * @Author        : xiaojun Hu
 * @CreateTime    : 2014.06.20
 * @LastEditTime  : 2022.01.20
 * @LastEditors   : 钟子明
 ******************************************************************************/
testConf.skipStandAlone = true;

main( test );

function test ()
{
   var alreadStart = false;
   var path = "";
   var clName = COMMCLNAME + "_cl14053";
   commDropCL( db, COMMCSNAME, clName, true, true, "Drop CL in the beginning" );
   var cl = commCreateCL( db, COMMCSNAME, clName, { ReplSize: -1 }, true, false );
   bakInsertData( cl );
   bakRemoveBackups( db, CHANGEDPREFIX, true );
   // Backup Offline specify the [GroupID]
   var groupID = commGetGroups( db );
   for( var i = 0; i < groupID.length; ++i )
   {
      if( groupID.length == 1 ) continue;
      var bakName = CHANGEDPREFIX + "_bak_" + i;
      var backup = { "GroupID": [] };
      backup["GroupID"].push( groupID[i][0].GroupID );
      backup["Name"] = bakName;
      bakBackup( db, backup );
      checkBackupInfo( db, "", bakName, path, alreadStart );
      bakRemoveBackups( db, bakName, alreadStart, path );
   }
   commDropCL( db, COMMCSNAME, clName, true, false, "Drop CL in the end" );
}
