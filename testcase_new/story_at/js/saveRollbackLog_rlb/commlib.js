import("../lib/basic_operation/commlib.js");
import("../lib/main.js");
import("../lib/location_commlib.js");


/* ****************************************************
@description: get the data directory of the sequoiadb node
@return: dir
**************************************************** */
function getSecondaryDBPath(groupName) {
   var info = db.list(SDB_SNAP_SYSTEM, { "GroupName": groupName }).current().toObj();
   var primaryNode = info.PrimaryNode;
   var groups = info.Group;
   for (i = 0; i < groups.length; i++) {
      var group = groups[i];
      var nodeID = group.NodeID;
      if (nodeID != primaryNode) {
         var dbpath = group.dbpath;
         break;
      }
   }
   return dbpath;
}

/* ****************************************************
@description: get install_dir of sequoiadb
@return: install_dir
**************************************************** */
function getInstallDir() {
   var localDir = cmd.run("pwd").split("\n")[0] + "/";
   var installDir = "";

   try {
      cmd.run("find ./bin/sdbreplay").split("\n")[0];
      installDir = localDir;
   } catch (e) {
      installDir = commGetInstallPath() + "/";
   }

   return installDir;
}

/* ****************************************************
@description: exec sdbreplay
@return: results
**************************************************** */
function execSdbReplay(rtCmd, groupName, type, host, svcname) {
   // ready file
   var dbPath = getSecondaryDBPath(groupName);
   var logPath = "";
   logPath = dbPath + "tmp/*";
   var installDir = getInstallDir();

   var command = installDir + 'bin/sdbreplay'
      + ' --type ' + type
      + ' --path ' + logPath
      + ' --host ' + host
      + ' --svcname ' + svcname

   println(command);
   sleep(15000);

   var rcSdbreplay = rtCmd.run(command);

   return rcSdbreplay;
}
