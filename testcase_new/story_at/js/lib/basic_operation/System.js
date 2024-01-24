if ( tmpSystem == undefined )
{
   var tmpSystem = {
      _getInfo: System.prototype._getInfo,
      addAHostMap: System.prototype.addAHostMap,
      addGroup: System.prototype.addGroup,
      addUser: System.prototype.addUser,
      buildTrusty: System.prototype.buildTrusty,
      delAHostMap: System.prototype.delAHostMap,
      delGroup: System.prototype.delGroup,
      delUser: System.prototype.delUser,
      getAHostMap: System.prototype.getAHostMap,
      getCpuInfo: System.prototype.getCpuInfo,
      getCurrentUser: System.prototype.getCurrentUser,
      getDiskInfo: System.prototype.getDiskInfo,
      getEWD: System.prototype.getEWD,
      getHostName: System.prototype.getHostName,
      getHostsMap: System.prototype.getHostsMap,
      getInfo: System.prototype.getInfo,
      getIpTablesInfo: System.prototype.getIpTablesInfo,
      getMemInfo: System.prototype.getMemInfo,
      getNetcardInfo: System.prototype.getNetcardInfo,
      getPID: System.prototype.getPID,
      getProcUlimitConfigs: System.prototype.getProcUlimitConfigs,
      getReleaseInfo: System.prototype.getReleaseInfo,
      getSystemConfigs: System.prototype.getSystemConfigs,
      getTID: System.prototype.getTID,
      getUserEnv: System.prototype.getUserEnv,
      help: System.prototype.help,
      isGroupExist: System.prototype.isGroupExist,
      isProcExist: System.prototype.isProcExist,
      isUserExist: System.prototype.isUserExist,
      killProcess: System.prototype.killProcess,
      listAllUsers: System.prototype.listAllUsers,
      listGroups: System.prototype.listGroups,
      listLoginUsers: System.prototype.listLoginUsers,
      listProcess: System.prototype.listProcess,
      ping: System.prototype.ping,
      removeTrusty: System.prototype.removeTrusty,
      runService: System.prototype.runService,
      setProcUlimitConfigs: System.prototype.setProcUlimitConfigs,
      setUserConfigs: System.prototype.setUserConfigs,
      snapshotCpuInfo: System.prototype.snapshotCpuInfo,
      snapshotDiskInfo: System.prototype.snapshotDiskInfo,
      snapshotMemInfo: System.prototype.snapshotMemInfo,
      snapshotNetcardInfo: System.prototype.snapshotNetcardInfo,
      sniffPort: System.prototype.sniffPort,
      type: System.prototype.type
   };
}
var funcSystem = ( funcSystem == undefined ) ? System : funcSystem;
var funcSystem_createSshKey = funcSystem._createSshKey;
var funcSystem_getHomePath = funcSystem._getHomePath;
var funcSystem_listAllUsers = funcSystem._listAllUsers;
var funcSystem_listGroups = funcSystem._listGroups;
var funcSystem_listLoginUsers = funcSystem._listLoginUsers;
var funcSystem_listProcess = funcSystem._listProcess;
var funcSystemaddAHostMap = funcSystem.addAHostMap;
var funcSystemaddGroup = funcSystem.addGroup;
var funcSystemaddUser = funcSystem.addUser;
var funcSystemdelAHostMap = funcSystem.delAHostMap;
var funcSystemdelGroup = funcSystem.delGroup;
var funcSystemdelUser = funcSystem.delUser;
var funcSystemgetAHostMap = funcSystem.getAHostMap;
var funcSystemgetCpuInfo = funcSystem.getCpuInfo;
var funcSystemgetCurrentUser = funcSystem.getCurrentUser;
var funcSystemgetDiskInfo = funcSystem.getDiskInfo;
var funcSystemgetEWD = funcSystem.getEWD;
var funcSystemgetHostName = funcSystem.getHostName;
var funcSystemgetHostsMap = funcSystem.getHostsMap;
var funcSystemgetIpTablesInfo = funcSystem.getIpTablesInfo;
var funcSystemgetMemInfo = funcSystem.getMemInfo;
var funcSystemgetNetcardInfo = funcSystem.getNetcardInfo;
var funcSystemgetObj = funcSystem.getObj;
var funcSystemgetPID = funcSystem.getPID;
var funcSystemgetProcUlimitConfigs = funcSystem.getProcUlimitConfigs;
var funcSystemgetReleaseInfo = funcSystem.getReleaseInfo;
var funcSystemgetSystemConfigs = funcSystem.getSystemConfigs;
var funcSystemgetTID = funcSystem.getTID;
var funcSystemgetUserEnv = funcSystem.getUserEnv;
var funcSystemhelp = funcSystem.help;
var funcSystemisGroupExist = funcSystem.isGroupExist;
var funcSystemisProcExist = funcSystem.isProcExist;
var funcSystemisUserExist = funcSystem.isUserExist;
var funcSystemkillProcess = funcSystem.killProcess;
var funcSystemlistAllUsers = funcSystem.listAllUsers;
var funcSystemlistGroups = funcSystem.listGroups;
var funcSystemlistLoginUsers = funcSystem.listLoginUsers;
var funcSystemlistProcess = funcSystem.listProcess;
var funcSystemping = funcSystem.ping;
var funcSystemrunService = funcSystem.runService;
var funcSystemsetProcUlimitConfigs = funcSystem.setProcUlimitConfigs;
var funcSystemsetUserConfigs = funcSystem.setUserConfigs;
var funcSystemsnapshotCpuInfo = funcSystem.snapshotCpuInfo;
var funcSystemsnapshotDiskInfo = funcSystem.snapshotDiskInfo;
var funcSystemsnapshotMemInfo = funcSystem.snapshotMemInfo;
var funcSystemsnapshotNetcardInfo = funcSystem.snapshotNetcardInfo;
var funcSystemsniffPort = funcSystem.sniffPort;
var funcSystemtype = funcSystem.type;
System=function(){try{return funcSystem.apply( this, arguments ); } catch( e ) { throw new Error(e) } };
System._createSshKey = function(){try{ return funcSystem_createSshKey.apply( this, arguments ); } catch( e ) { throw new Error(e) } };
System._getHomePath = function(){try{ return funcSystem_getHomePath.apply( this, arguments ); } catch( e ) { throw new Error(e) } };
System._listAllUsers = function(){try{ return funcSystem_listAllUsers.apply( this, arguments ); } catch( e ) { throw new Error(e) } };
System._listGroups = function(){try{ return funcSystem_listGroups.apply( this, arguments ); } catch( e ) { throw new Error(e) } };
System._listLoginUsers = function(){try{ return funcSystem_listLoginUsers.apply( this, arguments ); } catch( e ) { throw new Error(e) } };
System._listProcess = function(){try{ return funcSystem_listProcess.apply( this, arguments ); } catch( e ) { throw new Error(e) } };
System.addAHostMap = function(){try{ return funcSystemaddAHostMap.apply( this, arguments ); } catch( e ) { throw new Error(e) } };
System.addGroup = function(){try{ return funcSystemaddGroup.apply( this, arguments ); } catch( e ) { throw new Error(e) } };
System.addUser = function(){try{ return funcSystemaddUser.apply( this, arguments ); } catch( e ) { throw new Error(e) } };
System.delAHostMap = function(){try{ return funcSystemdelAHostMap.apply( this, arguments ); } catch( e ) { throw new Error(e) } };
System.delGroup = function(){try{ return funcSystemdelGroup.apply( this, arguments ); } catch( e ) { throw new Error(e) } };
System.delUser = function(){try{ return funcSystemdelUser.apply( this, arguments ); } catch( e ) { throw new Error(e) } };
System.getAHostMap = function(){try{ return funcSystemgetAHostMap.apply( this, arguments ); } catch( e ) { throw new Error(e) } };
System.getCpuInfo = function(){try{ return funcSystemgetCpuInfo.apply( this, arguments ); } catch( e ) { throw new Error(e) } };
System.getCurrentUser = function(){try{ return funcSystemgetCurrentUser.apply( this, arguments ); } catch( e ) { throw new Error(e) } };
System.getDiskInfo = function(){try{ return funcSystemgetDiskInfo.apply( this, arguments ); } catch( e ) { throw new Error(e) } };
System.getEWD = function(){try{ return funcSystemgetEWD.apply( this, arguments ); } catch( e ) { throw new Error(e) } };
System.getHostName = function(){try{ return funcSystemgetHostName.apply( this, arguments ); } catch( e ) { throw new Error(e) } };
System.getHostsMap = function(){try{ return funcSystemgetHostsMap.apply( this, arguments ); } catch( e ) { throw new Error(e) } };
System.getIpTablesInfo = function(){try{ return funcSystemgetIpTablesInfo.apply( this, arguments ); } catch( e ) { throw new Error(e) } };
System.getMemInfo = function(){try{ return funcSystemgetMemInfo.apply( this, arguments ); } catch( e ) { throw new Error(e) } };
System.getNetcardInfo = function(){try{ return funcSystemgetNetcardInfo.apply( this, arguments ); } catch( e ) { throw new Error(e) } };
System.getObj = function(){try{ return funcSystemgetObj.apply( this, arguments ); } catch( e ) { throw new Error(e) } };
System.getPID = function(){try{ return funcSystemgetPID.apply( this, arguments ); } catch( e ) { throw new Error(e) } };
System.getProcUlimitConfigs = function(){try{ return funcSystemgetProcUlimitConfigs.apply( this, arguments ); } catch( e ) { throw new Error(e) } };
System.getReleaseInfo = function(){try{ return funcSystemgetReleaseInfo.apply( this, arguments ); } catch( e ) { throw new Error(e) } };
System.getSystemConfigs = function(){try{ return funcSystemgetSystemConfigs.apply( this, arguments ); } catch( e ) { throw new Error(e) } };
System.getTID = function(){try{ return funcSystemgetTID.apply( this, arguments ); } catch( e ) { throw new Error(e) } };
System.getUserEnv = function(){try{ return funcSystemgetUserEnv.apply( this, arguments ); } catch( e ) { throw new Error(e) } };
System.help = function(){try{ return funcSystemhelp.apply( this, arguments ); } catch( e ) { throw new Error(e) } };
System.isGroupExist = function(){try{ return funcSystemisGroupExist.apply( this, arguments ); } catch( e ) { throw new Error(e) } };
System.isProcExist = function(){try{ return funcSystemisProcExist.apply( this, arguments ); } catch( e ) { throw new Error(e) } };
System.isUserExist = function(){try{ return funcSystemisUserExist.apply( this, arguments ); } catch( e ) { throw new Error(e) } };
System.killProcess = function(){try{ return funcSystemkillProcess.apply( this, arguments ); } catch( e ) { throw new Error(e) } };
System.listAllUsers = function(){try{ return funcSystemlistAllUsers.apply( this, arguments ); } catch( e ) { throw new Error(e) } };
System.listGroups = function(){try{ return funcSystemlistGroups.apply( this, arguments ); } catch( e ) { throw new Error(e) } };
System.listLoginUsers = function(){try{ return funcSystemlistLoginUsers.apply( this, arguments ); } catch( e ) { throw new Error(e) } };
System.listProcess = function(){try{ return funcSystemlistProcess.apply( this, arguments ); } catch( e ) { throw new Error(e) } };
System.ping = function(){try{ return funcSystemping.apply( this, arguments ); } catch( e ) { throw new Error(e) } };
System.runService = function(){try{ return funcSystemrunService.apply( this, arguments ); } catch( e ) { throw new Error(e) } };
System.setProcUlimitConfigs = function(){try{ return funcSystemsetProcUlimitConfigs.apply( this, arguments ); } catch( e ) { throw new Error(e) } };
System.setUserConfigs = function(){try{ return funcSystemsetUserConfigs.apply( this, arguments ); } catch( e ) { throw new Error(e) } };
System.snapshotCpuInfo = function(){try{ return funcSystemsnapshotCpuInfo.apply( this, arguments ); } catch( e ) { throw new Error(e) } };
System.snapshotDiskInfo = function(){try{ return funcSystemsnapshotDiskInfo.apply( this, arguments ); } catch( e ) { throw new Error(e) } };
System.snapshotMemInfo = function(){try{ return funcSystemsnapshotMemInfo.apply( this, arguments ); } catch( e ) { throw new Error(e) } };
System.snapshotNetcardInfo = function(){try{ return funcSystemsnapshotNetcardInfo.apply( this, arguments ); } catch( e ) { throw new Error(e) } };
System.sniffPort = function(){try{ return funcSystemsniffPort.apply( this, arguments ); } catch( e ) { throw new Error(e) } };
System.type = function(){try{ return funcSystemtype.apply( this, arguments ); } catch( e ) { throw new Error(e) } };
System.prototype._getInfo=function(){try{return tmpSystem._getInfo.apply(this,arguments);}catch(e){throw new Error(e);}};
System.prototype.addAHostMap=function(){try{return tmpSystem.addAHostMap.apply(this,arguments);}catch(e){throw new Error(e);}};
System.prototype.addGroup=function(){try{return tmpSystem.addGroup.apply(this,arguments);}catch(e){throw new Error(e);}};
System.prototype.addUser=function(){try{return tmpSystem.addUser.apply(this,arguments);}catch(e){throw new Error(e);}};
System.prototype.buildTrusty=function(){try{return tmpSystem.buildTrusty.apply(this,arguments);}catch(e){throw new Error(e);}};
System.prototype.delAHostMap=function(){try{return tmpSystem.delAHostMap.apply(this,arguments);}catch(e){throw new Error(e);}};
System.prototype.delGroup=function(){try{return tmpSystem.delGroup.apply(this,arguments);}catch(e){throw new Error(e);}};
System.prototype.delUser=function(){try{return tmpSystem.delUser.apply(this,arguments);}catch(e){throw new Error(e);}};
System.prototype.getAHostMap=function(){try{return tmpSystem.getAHostMap.apply(this,arguments);}catch(e){throw new Error(e);}};
System.prototype.getCpuInfo=function(){try{return tmpSystem.getCpuInfo.apply(this,arguments);}catch(e){throw new Error(e);}};
System.prototype.getCurrentUser=function(){try{return tmpSystem.getCurrentUser.apply(this,arguments);}catch(e){throw new Error(e);}};
System.prototype.getDiskInfo=function(){try{return tmpSystem.getDiskInfo.apply(this,arguments);}catch(e){throw new Error(e);}};
System.prototype.getEWD=function(){try{return tmpSystem.getEWD.apply(this,arguments);}catch(e){throw new Error(e);}};
System.prototype.getHostName=function(){try{return tmpSystem.getHostName.apply(this,arguments);}catch(e){throw new Error(e);}};
System.prototype.getHostsMap=function(){try{return tmpSystem.getHostsMap.apply(this,arguments);}catch(e){throw new Error(e);}};
System.prototype.getInfo=function(){try{return tmpSystem.getInfo.apply(this,arguments);}catch(e){throw new Error(e);}};
System.prototype.getIpTablesInfo=function(){try{return tmpSystem.getIpTablesInfo.apply(this,arguments);}catch(e){throw new Error(e);}};
System.prototype.getMemInfo=function(){try{return tmpSystem.getMemInfo.apply(this,arguments);}catch(e){throw new Error(e);}};
System.prototype.getNetcardInfo=function(){try{return tmpSystem.getNetcardInfo.apply(this,arguments);}catch(e){throw new Error(e);}};
System.prototype.getPID=function(){try{return tmpSystem.getPID.apply(this,arguments);}catch(e){throw new Error(e);}};
System.prototype.getProcUlimitConfigs=function(){try{return tmpSystem.getProcUlimitConfigs.apply(this,arguments);}catch(e){throw new Error(e);}};
System.prototype.getReleaseInfo=function(){try{return tmpSystem.getReleaseInfo.apply(this,arguments);}catch(e){throw new Error(e);}};
System.prototype.getSystemConfigs=function(){try{return tmpSystem.getSystemConfigs.apply(this,arguments);}catch(e){throw new Error(e);}};
System.prototype.getTID=function(){try{return tmpSystem.getTID.apply(this,arguments);}catch(e){throw new Error(e);}};
System.prototype.getUserEnv=function(){try{return tmpSystem.getUserEnv.apply(this,arguments);}catch(e){throw new Error(e);}};
System.prototype.help=function(){try{return tmpSystem.help.apply(this,arguments);}catch(e){throw new Error(e);}};
System.prototype.isGroupExist=function(){try{return tmpSystem.isGroupExist.apply(this,arguments);}catch(e){throw new Error(e);}};
System.prototype.isProcExist=function(){try{return tmpSystem.isProcExist.apply(this,arguments);}catch(e){throw new Error(e);}};
System.prototype.isUserExist=function(){try{return tmpSystem.isUserExist.apply(this,arguments);}catch(e){throw new Error(e);}};
System.prototype.killProcess=function(){try{return tmpSystem.killProcess.apply(this,arguments);}catch(e){throw new Error(e);}};
System.prototype.listAllUsers=function(){try{return tmpSystem.listAllUsers.apply(this,arguments);}catch(e){throw new Error(e);}};
System.prototype.listGroups=function(){try{return tmpSystem.listGroups.apply(this,arguments);}catch(e){throw new Error(e);}};
System.prototype.listLoginUsers=function(){try{return tmpSystem.listLoginUsers.apply(this,arguments);}catch(e){throw new Error(e);}};
System.prototype.listProcess=function(){try{return tmpSystem.listProcess.apply(this,arguments);}catch(e){throw new Error(e);}};
System.prototype.ping=function(){try{return tmpSystem.ping.apply(this,arguments);}catch(e){throw new Error(e);}};
System.prototype.removeTrusty=function(){try{return tmpSystem.removeTrusty.apply(this,arguments);}catch(e){throw new Error(e);}};
System.prototype.runService=function(){try{return tmpSystem.runService.apply(this,arguments);}catch(e){throw new Error(e);}};
System.prototype.setProcUlimitConfigs=function(){try{return tmpSystem.setProcUlimitConfigs.apply(this,arguments);}catch(e){throw new Error(e);}};
System.prototype.setUserConfigs=function(){try{return tmpSystem.setUserConfigs.apply(this,arguments);}catch(e){throw new Error(e);}};
System.prototype.snapshotCpuInfo=function(){try{return tmpSystem.snapshotCpuInfo.apply(this,arguments);}catch(e){throw new Error(e);}};
System.prototype.snapshotDiskInfo=function(){try{return tmpSystem.snapshotDiskInfo.apply(this,arguments);}catch(e){throw new Error(e);}};
System.prototype.snapshotMemInfo=function(){try{return tmpSystem.snapshotMemInfo.apply(this,arguments);}catch(e){throw new Error(e);}};
System.prototype.snapshotNetcardInfo=function(){try{return tmpSystem.snapshotNetcardInfo.apply(this,arguments);}catch(e){throw new Error(e);}};
System.prototype.sniffPort=function(){try{return tmpSystem.sniffPort.apply(this,arguments);}catch(e){throw new Error(e);}};
System.prototype.type=function(){try{return tmpSystem.type.apply(this,arguments);}catch(e){throw new Error(e);}};
