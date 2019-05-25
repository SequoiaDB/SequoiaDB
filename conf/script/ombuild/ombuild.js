/*******************************************************************************

   Copyright (C) 2012-2014 SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

*******************************************************************************/
/*
@description: rebuild OM
@modify list:
   2016-5-27 Zhaobo Tan  Init
@parameter
   OM_CONF_FILE: OM configure file
   ACTION      : tell the current script what to do
@return
   void 
*/

//setLogLevel( PDDEBUG ) ;

var FILE_NAME_REBUILD_OM = "rebuildOM.js" ;
var logger = new Logger(FILE_NAME_REBUILD_OM) ;

var ts = null ;

function _init() {
   logger.log( PDEVENT, "Begin to run tasks" ) ;
   ts = genTimeStamp() ;
} ;

function _final() {
   
   logger.log( PDEVENT, "Finish running all the tasks" ) ;
} ;

var Helper = function() {} ;

Helper.prototype.changeToObj = function Helper_changeToObj( str ) {
   var retObj = null ;
   try {
      retObj = eval( '(' + str + ')' ) ;
   } catch( e ) {
      var exp = new SdbError( e, 
         sprintf( "failed to transform [?] to an object", str ) ) ;
      logger.log( PDERROR, exp ) ;
      throw exp ;
   }
   if ( !isObject( retObj ) ) {
      var exp = new SdbError( SDB_SYS, 
         sprintf( "the transforming result of string [?] is not an object", 
         str ) ) ;
      logger.log( PDERROR, exp ) ;
      throw exp ;
   }
   return retObj ;
} ;

Helper.prototype.execCommand = 
   function Helper_execCommand( ssh, installPath, cmd, quit ) {
   var retStr  = null ;
   var exp     = null ;
   var str     = adaptPath(installPath) + OMA_PROG_BIN_SDB ;
   var execCmd = str + " -s " + "\' " +  cmd  + " \'" ;
   var quitCmd = str + " -s " + "\' quit \'" ;

   try {
      retStr = ssh.exec( execCmd ) ;
   } catch( e ) {
      var exp = new SdbError( e, 
         sprintf( "failed to execute command[?] in host[?], error detail is[?]", 
            execCmd, ssh.getPeerIP(), ssh.getLastOut() ) ) ;
      logger.log( PDERROR, exp ) ;
      throw exp ;
   } finally {
      if ( quit == true || exp != null ) {
         try { ssh.exec( quitCmd ); } catch(e) {}
      }
   }
   return retStr ;
} ;

Helper.prototype.getMatchedSize = 
   function Helper_getMatchedSize( numberStr ) {
   var size = 0 ;
   try {
      size = parseInt( numberStr ) ;
   } catch( e ) {
      exp = new SdbError( e, 
         sprintf( "failed to parse [?] to a number", numberStr ) ) ;
      logger.log( PDERROR, exp ) ;
      throw exp ;
   }
   if ( !isNumber(size) ) {
      exp = new SdbError( SDB_SYS, 
         sprintf( "the parse result is [?], " +
            "but it's not a number", size ) ) ;
      logger.log( PDERROR, exp ) ;
      throw exp ;
   }
   return size ;
} ;

Helper.prototype.arrayContainElement = 
   function Helper_arrayContainElement( arr, ele ) {
   if ( !isArray(arr) ) {
      var exp = new SdbError( SDB_INVALIDARG, 
         sprintf( "the input argument is not an array[?]", arr ) ) ;
      logger.log( PDERROR, exp ) ;
      throw exp ;
   }
   for ( var i = 0; i < arr.length; i++ ) {
      if ( ele == arr[i] ) return true ;
   }
   return false ;
} ;

Helper.prototype.mergeArray = function Helper_mergeArray( arr1, arr2 ) {
   var retArr = [] ;
   if ( !isArray(arr1) || !isArray(arr2) ) {
      var exp = new SdbError( SDB_INVALIDARG, 
         sprintf( "offer invalid array1[?], array2[?]", arr1, arr2 ) ) ;
      logger.log( PDERROR, exp ) ;
      throw exp ;
   }
   retArr = arr1.splice( 0, 0 ) ;
   for (var i = 0; i < arr2.length; i++ ) {    
      if ( !this.arrayContainElement( retArr, arr2[i] ) ) {
         retArr.push( arr2[i] ) ;
      }
   }
   return retArr ;
   
} ;

Helper.prototype.getAgentPort = function Helper_getAgentPort() {
   var retStr = null ;
   retStr     = Oma.getAOmaSvcName( "127.0.0.1" );
   logger.log( PDEVENT, 
      sprintf( "local sdbcm's service is [?]", retStr ) ) ;
   return retStr ;
} ;

Helper.prototype.getOMSvc = function Helper_getOMSvc() {
   var exp = null ;
   var arr = Sdbtool.listNodes( { "role":"om", "mode":"local" } ) ;
   if ( arr.size() == 0 ) {
      exp = new SdbError( SDBCM_NODE_NOTEXISTED, "om does not exist in local host" ) ;
      logger.log( PDWARNING, exp ) ;
      throw exp ;
   }
   var obj    = JSON.parse( arr.next() ) ;
   var retStr = obj[SvcName3] ;
   if ( !isString(retStr) ) {
      exp = new SdbError( SDB_SYS, 
         sprintf( "the port[?] of om we got from local host is not " + 
            "a string", retStr ) ) ;
      logger.log( PDERROR, exp ) ;
      throw exp ;
   }
   return retStr ;
} ;

Helper.prototype.getOMAddr = function Helper_getOMAddr() {
   var exp = null ;
   var fileName  = Oma.getOmaConfigFile() ;
   var configObj = JSON.parse( Oma.getOmaConfigs( fileName ) ) ;
   var retStr    = configObj["OMAddress"] ;
   if ( !isString(retStr) ) {
      exp = new SdbError( SDB_SYS, 
         sprintf( "the address[?] of om we got from local sdbcm's " + 
            "config file is not a string", retStr ) ) ;
      logger.log( PDERROR, exp ) ;
      throw exp ;
   }
   return retStr ;
} ;

Helper.prototype.omExist = function Helper_omExist() {
   var omSvc = null ;
   var exp = null ;
   try {
	   omSvc = this.getOMSvc();
   } catch( e ) {
	   exp = new SdbError( e, "failed to get the port of om" ) ;
	   if ( exp.getErrCode() == SDBCM_NODE_NOTEXISTED ) {
		 return false ;
	   } else {
		  logger.log( PDEROR, exp ) ;
	     throw exp ;
	  }
   }
   if ( isString(omSvc) ) {
	  return true ;
   } else {
	  exp = new SdbError( SDB_SYS, 
	        sprintf( "the port[?] of om is not a string", omSvc ) ) ;
      throw exp ;
   }
} ;

var ConfigMgr = function ( omConfigFile, action ) {
   this._configFile        = omConfigFile ;
   this._action            = action ;
   this._omConfObj         = null ;
   this._sdb               = null ;
   this._clusterInfo       = null ;
   this._moduleInfo        = null ;
   this._dbConfigOption    = null ;
   this._dbInstallPacket   = null ;
   this._coordInfoFromHost = [] ;
   this._coordInfoInFormat = [] ;
   this._rgInfoArr         = [] ;
   this._hostInfoArr       = [] ;
   this._omAuthInfo        = new AuthInfo() ;

} ;

ConfigMgr.prototype = new Helper() ;

ConfigMgr.prototype._final = function ConfigMgr__final() {
   for ( var i = 0; i < this._hostInfoArr.length; i++ ) {
      try {
         var rootSshObj  = this._hostInfoArr[i].rootSshObj ;
         rootSshObj.close() ;
      } catch(e) {
      }
      try {
         var adminSshObj = this._hostInfoArr[i].adminSshObj ;
         adminSshObj.close() ;
      } catch(e) {
      }
   }
} ;

ConfigMgr.prototype._init = function ConfigMgr__init() {
   var errMsg      = null ;
   var exp         = null ;

   if ( SYS_LINUX != SYS_TYPE ) {
      exp = new SdbError( SDB_SYS, 
         "not support current operating system[" + SYS_TYPE + "]" ) ;
      logger.log( PDERROR, exp ) ;
      throw exp ;
   }
   
   // 1. check input action
   if ( this._action != "" && 
        this._action != ACTION_BUILD_OM && 
        this._action != ACTION_REMOVE_OM &&
        this._action != ACTION_ADD_BUSINESS &&
        this._action != ACTION_UPDATE_COORD &&
        this._action != ACTION_FLUSH_CONFIG ) {
         exp = new SdbError( SDB_INVALIDARG, 
            sprintf( "--action should be one of the follow: ?|?|?|?|?", 
               ACTION_BUILD_OM, ACTION_REMOVE_OM, ACTION_ADD_BUSINESS,
               ACTION_UPDATE_COORD, ACTION_FLUSH_CONFIG ) ) ;
      logger.log( PDERROR, exp ) ;
      throw exp ;
   }
   if ( this._action == "" ) {
      this._action = ACTION_ADD_BUSINESS ;
   }
   if ( this._action == ACTION_REMOVE_OM ) {
      return ;
   }
   
   // check input config file
   if ( this._configFile == null || this._configFile == "" ) {
      errMsg = "configure file should be specified" ;
      exp = new SdbError( SDB_INVALIDARG, errMsg ) ;
      logger.log( PDERROR, exp ) ;
      throw exp ;
   } else if ( !File.exist( this._configFile ) ) {
      errMsg = "configure file[" + 
         this._configFile + "] does not exist" ;
      exp = new SdbError( SDB_INVALIDARG, errMsg ) ;
      logger.log( PDERROR, exp ) ;
      throw exp ;
   }
   
   // 2. get configure info
   try {
      this._omConfObj = 
         eval( '(' + Oma.getOmaConfigs( this._configFile ) + ')' ) ;
      logger.log( PDEVENT, 
         "configure info is: " + JSON.stringify( this._omConfObj ) ) ;
   } catch( e ) {
      errMsg = 
         "failed to get configure info from file[" + this._configFile + "]" ;
      exp = new SdbError( SDB_INVALIDARG, errMsg ) ;
      logger.log( PDERROR, exp ) ;
      throw exp ;
   }
} ;

ConfigMgr.prototype._getAction = function ConfigMgr__getAction() {
   return this._action ;
} ;

ConfigMgr.prototype._getRGInfoArr = 
   function ConfigMgr__getRGInfoArr( refresh ) {
   if ( refresh ) {
      this._appendRGInfo() ;
   }
   return this._rgInfoArr ;
} ;

ConfigMgr.prototype._getHostInfoArr = function ConfigMgr__getHostInfoArr() {
   return this._hostInfoArr ;
} ;

ConfigMgr.prototype._getSdbObj = function ConfigMgr__getSdbObj() {
   return this._sdb ;
} ;

ConfigMgr.prototype._getClusterInfo = function ConfigMgr__getClusterInfo() {
   return this._clusterInfo ;
} ;

ConfigMgr.prototype._getModuleInfo = function ConfigMgr__getModuleInfo() {
   return this._moduleInfo ;
} ;

ConfigMgr.prototype._getDBConfigInfo = function ConfigMgr__getDBConfigInfo() {
   return this._dbConfigOption ;
} ;

ConfigMgr.prototype._getDBInstallPacket = 
   function ConfigMgr__getDBInstallPacket() {
   return this._dbInstallPacket ;
} ;

ConfigMgr.prototype._getCoordInfoWrapperArr = 
   function ConfigMgr__getCoordInfoWrapperArr() {
   return this._coordInfoInFormat ;
} ;

ConfigMgr.prototype._getOMAuthInfo = function ConfigMgr__getOMAuthInfo() {
   return this._omAuthInfo ;
} ;

ConfigMgr.prototype._getField = 
   function ConfigMgr__getField( fieldName ) {
   var ret    = null ;
   var errMsg = null ;
   var err    = null ;
   var field  = this._omConfObj[fieldName] ;
   if ( "" == field || null == field || "undefined" == typeof(field) ) {
      errMsg = "can not get value according to field name [" + fieldName + "]" ;
      err = new SdbError( SDB_INVALIDARG, errMsg ) ;
      logger.log( PDERROR, err ) ;
      throw err ;
   }
   ret = removeQuotes( field ) ;
   ret = strTrim( ret ) ;
   return ret ;
} ;

ConfigMgr.prototype._appendClusterInfo = 
   function ConfigMgr__appendClusterInfo() {
   retObj               = new ClusterInfo() ;
   retObj[ClusterName]  = this._getField( FIELD_CLUSTER_NAME ) ;
   retObj[Desc]         = this._getField( FIELD_CLUSTER_DESCRIPTION ) ;
   retObj[SdbUser]      = this._getField( FIELD_SDB_ADMIN_USER_NAME ) ;
   retObj[SdbPasswd]    = this._getField( FIELD_SDB_ADMIN_PASSWORD ) ;
   retObj[SdbUserGroup] = this._getField( FIELD_SDB_ADMIN_GROUP_NAME ) ;
   retObj[InstallPath]  = 
      adaptPath( this._getField( FIELD_SDB_DEFAULT_INSTALL_PATH ) ) ;
   this._clusterInfo    = retObj ;
} ;

ConfigMgr.prototype._appendModuleInfo = 
   function ConfigMgr__appendModuleInfo() {
   retObj               = new ModuleInfo() ;
   retObj[ClusterName]  = this._getField( FIELD_CLUSTER_NAME ) ;
   retObj[BusinessName] = this._getField( FIELD_MODULE_NAME ) ;
   retObj[Time]         = { "$timestamp" : Timestamp()._t } ;
   this._moduleInfo     = retObj ;
} ;

ConfigMgr.prototype._appendAddress = 
   function ConfigMgr__appendAddress() {
   var offeredAddrArr = [] ;
   var value          = this._getField( FIELD_HOST_NAME ) ;
   var nameArr        = value.split( ',' ) ;
   for ( var i = 0; i < nameArr.length; i++ ) {
      var address  = strTrim( nameArr[i] ) ;
      if ( address == "" ||
           this.arrayContainElement( offeredAddrArr, address ) ) {
         continue ;
      }
      offeredAddrArr.push( address ) ;
   }
   // keep the address to local
   for ( var i = 0; i < offeredAddrArr.length; i++ ) {
      var hostInfo      = new HostInfo() ;
      hostInfo.address  = offeredAddrArr[i] ;
      this._hostInfoArr.push( hostInfo ) ;
   }  
} ;

ConfigMgr.prototype._appendRootInfo = 
   function ConfigMgr__appendRootInfo() {
   var defaultRootUserName  = this._getField( FIELD_DEFAULT_ROOT_USER ) ;
   var defaultRootPassword  = this._getField( FIELD_DEFAULT_ROOT_PASSWD ) ;
   var len = this._hostInfoArr.length ;
   for ( var i = 0; i < len; i++ ) {
      var address       = this._hostInfoArr[i].address ;
      var rootUserName  = null ;
      var rootPassword  = null ;
      var field         = null ;
      var value         = null ;
      
      // get specific root info
      field          = address + FIELD_ROOT_USER ;
      value          = this._omConfObj[field] ;
      rootUserName   = ( value == undefined ) ? defaultRootUserName : 
         removeQuotes( value ) ;
      field          = address + FIELD_ROOT_PASSWD ;
      value          = this._omConfObj[field] ;
      rootPassword   = ( value == undefined ) ? defaultRootPassword : 
         removeQuotes( value ) ;
      // append root info
      this._hostInfoArr[i].rootUserName  = rootUserName ;
      this._hostInfoArr[i].rootPassword  = rootPassword ;
   }
} ;

ConfigMgr.prototype._appendSshPort = 
   function ConfigMgr__appendSshPort() {
   var value          = this._getField( FIELD_DEFAULT_SSH_PORT ) ;
   var defaultSshPort = parseInt( value ) ;
   var len            = this._hostInfoArr.length ;
   for ( var i = 0; i < len; i++ ) {
      var address     = this._hostInfoArr[i].address ;
      var sshPort     = null ;
      var field       = null ;
      var value       = null ;
      // get specific ssh port
      field        = address + FIELD_SSH_PORT ;
      value        = this._omConfObj[field] ;
      sshPort      = ( value == undefined ) ? defaultSshPort : 
         parseInt( removeQuotes( value ) ) ;
      // append ssh info
      this._hostInfoArr[i].sshPort = sshPort ;
   }
} ;

ConfigMgr.prototype._appendSdbAdminInfo = 
   function ConfigMgr__appendSdbAdminInfo() {
   var sdbUserName       = this._getField( FIELD_SDB_ADMIN_USER_NAME ) ;
   var sdbPassword       = this._getField( FIELD_SDB_ADMIN_PASSWORD ) ;
   var sdbUserGroupName  = this._getField( FIELD_SDB_ADMIN_GROUP_NAME ) ;
   var len               = this._hostInfoArr.length ;
   for ( var i = 0; i < len; i++ ) {
      var hostInfo = this._hostInfoArr[i] ;
      // append admin info
      hostInfo.sdbUserName      = sdbUserName ;
      hostInfo.sdbPassword      = sdbPassword ;      
      hostInfo.sdbUserGroupName = sdbUserGroupName ;
   }
} ;

ConfigMgr.prototype._appendOMAdminInfo = 
   function ConfigMgr__appendOMAdminInfo() {
   var sdbUserName       = this._getField( FIELD_OM_ADMIN_USER_NAME ) ;
   var sdbPassword       = this._getField( FIELD_OM_ADMIN_PASSWORD ) ;
   this._omAuthInfo[AuthUser]   = sdbUserName ;
   this._omAuthInfo[AuthPasswd] = sdbPassword ;
} ;

ConfigMgr.prototype._appendDBConfigOption = 
   function ConfigMgr__appendDBConfigOption() {
   this._dbConfigOption = new Object() ;
   this._dbConfigOption[ClusterName3]  = this._clusterInfo[ClusterName] ;
   this._dbConfigOption[BusinessName3] = this._moduleInfo[BusinessName] ;
   this._dbConfigOption[UserTag3]      = "" ;
} ;

ConfigMgr.prototype._appendDBInstallPacket = 
   function ConfigMgr__appendDBInstallPacket() {
   this._dbInstallPacket = this._getField( FIELD_DB_INSTALL_PACKET ) ;
} ;

ConfigMgr.prototype._checkRootName = function ConfigMgr__checkRootName() {
   var errMsg    = null ;
   var errMsgArr = [] ;
   var exp       = null ;
   var rootName  = null ;
   var address   = null ;

   for ( var i = 0; i < this._hostInfoArr.length; i++ ) {
      userName = this._hostInfoArr[i].rootUserName ;
      address  = this._hostInfoArr[i].address ;
      if ( userName != "root" ) {
         errMsg = 
            sprintf( "should offer root account info in host[?]", address ) ;
         errMsgArr.push( errMsg ) ;
         if ( exp == null ) exp = new SdbError( SDB_INVALIDARG ) ;
         logger.log( PDERROR, errMsg ) ;     
      }
   }
   if ( exp != null ) {
      throw new SdbError( SDB_INVALIDARG, errMsgArr.toString() ) ;
   }
} ;

ConfigMgr.prototype._checkSsh = 
   function ConfigMgr__checkSsh( account ) {
   var errMsg    = null ;
   var errMsgArr = [] ;
   var exp       = null ;
   var address   = null ;
   var user      = null ;
   var passwd    = null ;
   var sshPort   = null ;
   for ( var i = 0; i < this._hostInfoArr.length; i++ ) {
      address = this._hostInfoArr[i].address ;
      sshPort = this._hostInfoArr[i].sshPort ;
      if ( account == "root" ) {
         user   = this._hostInfoArr[i].rootUserName ;
         passwd = this._hostInfoArr[i].rootPassword ;
      } else {
         user   = this._hostInfoArr[i].sdbUserName ;
         passwd = this._hostInfoArr[i].sdbPassword ;
      }
      
      try {
         var ssh = new Ssh( address, user, passwd, sshPort ) ;
      } catch( e ) {
         errMsg = "can not use account[" + user + 
            "] to ssh to host[" + address + "]" ;
         errMsgArr.push( new SdbError( e, errMsg ).toString() ) ;
         if ( exp == null ) exp = new SdbError( SDB_INVALIDARG ) ;
         logger.log( PDERROR, errMsg ) ;
      }
   }
   if ( exp != null ) {
      throw new SdbError( SDB_INVALIDARG, errMsgArr.toString() ) ;
   }
} ;

ConfigMgr.prototype._checkDBConfigOption = 
   function ConfigMgr__checkDBConfigOption( field ) {
   var exp = null ;
   var value = this._dbConfigOption[field] ;
   if ( value == null ) {
      exp = new SdbError( SDB_INVALIDARG, 
         sprintf( "the value[?] of field[?] in db " +
            "config option is invalid", value, field ) ) ;
      logger.log( PDERROR, exp ) ;
      throw exp ;
   }
} ;

ConfigMgr.prototype._firstlyCheck = 
   function ConfigMgr_firstlyCheck() {
   var errMsg    = null ;
   var errMsgArr = [] ;
   var exp       = null ;
   var address   = null ;
   logger.log( PDEVENT, "begin to firstly check" ) ;
   // 1. check install packet is ok or not
   if ( this._action == ACTION_BUILD_OM ) {
      if ( this._dbInstallPacket == null || 
           this._dbInstallPacket == "" ) {
         exp = new SdbError( SDB_INVALIDARG, 
            sprintf( "should specify SequoiaDB install packet in " + 
               "building om configure file[?]", this._configFile ) ) ;
         logger.log( PDERROR, exp ) ;
         throw exp ;
      }
      if ( !File.exist( this._dbInstallPacket ) ) {
         errMsg = sprintf( "SequoiaDB install packet[?] does not exist " + 
            "in local host", this._dbInstallPacket ) ;
         exp = new SdbError( SDB_INVALIDARG, errMsg ) ;
         logger.log( PDERROR, exp ) ;
         throw exp ;
      }
      var len = this._dbInstallPacket.length ;
      if ( len < 4 ) {
         errMsg = sprintf( "the giving packet[?] is not a SequoiaDB install " + 
            "packet", this._dbInstallPacket ) ;
         exp = new SdbError( SDB_INVALIDARG, errMsg ) ;
         logger.log( PDERROR, exp ) ;
         throw exp ;
      }
      var subStr = this._dbInstallPacket.substring( len - 4 ) ;
      if ( subStr != ".run" ) {
         errMsg = sprintf( "the giving SequoiaDB install packet should " + 
            "be in the format of xxx.run, but not[?]", this._dbInstallPacket ) ;
         exp = new SdbError( SDB_INVALIDARG, errMsg ) ;
         logger.log( PDERROR, exp ) ;
         throw exp ;         
      }
   }
   
   // 2. host is ip or not
   for ( var i = 0; i < this._hostInfoArr.length; i++ ) {
      address = this._hostInfoArr[i].address ;
      errMsg  = null ;
      if ( isIP(address) ) {
         errMsg = "should offer hostname but not ip[" + address + "]" ;
      } else if ( address == "localhost" ) {
         errMsg = "should offer hostname but not localhost" ;        
      }
      if ( errMsg != null ) {
         errMsgArr.push( errMsg ) ;
         if ( exp == null ) exp = new SdbError( SDB_INVALIDARG ) ;
         logger.log( PDERROR, errMsg ) ;
      }
   }
   if ( exp != null ) {
      throw new SdbError( SDB_INVALIDARG, errMsgArr.toString() ) ;
   }

   // 3. can ping or not
   for ( var i = 0; i < this._hostInfoArr.length; i++ ) {
      address = this._hostInfoArr[i].address ;
      var result = eval( '(' + System.ping( address ) + ')' ) ;
      if ( result[Reachable] != true ) {
         errMsg = "host [" + address + "] is unreachable" ;
         errMsgArr.push( errMsg ) ;
         if ( exp == null ) exp = new SdbError( SDB_INVALIDARG ) ;
         logger.log( PDERROR, errMsg ) ;
      }
   }
   if ( exp != null ) {
      throw new SdbError( SDB_INVALIDARG, errMsgArr.toString() ) ;
   }

   // 4. check the root name is "root" or not
   this._checkRootName() ;

   // 5. root account can ssh or not
   this._checkSsh( "root" ) ;

   // 6. admin account can ssh or not
   this._checkSsh( "admin" ) ;
   
   // 8. check cluster info
   if ( this._clusterInfo[ClusterName] == null ||
        this._clusterInfo[ClusterName] == "" ) {
      exp = new SdbError( SDB_INVALIDARG, 
         sprintf("cluster name[?] can't be null or empty", 
            this._clusterInfo[ClusterName] ) ) ;
      logger.log( PDERROR, exp ) ;
      throw exp ;
   }

   // 9. check module info
   if ( this._moduleInfo[BusinessName] == null ||
        this._moduleInfo[BusinessName] == "" ) {
      exp = new SdbError( SDB_INVALIDARG, 
         sprintf("module name[?] can't be null or empty", 
            this._moduleInfo[BusinessName] ) ) ;
      logger.log( PDERROR, exp ) ;
      throw exp ;
   } 
      
   logger.log( PDEVENT, "finishing firstly check" ) ;
} ;

ConfigMgr.prototype._appendSshObj = function ConfigMgr__appendSshObj() {
   var errMsg  = null ;
   var exp     = null ;
   var address = null ;
   for ( var i = 0; i < this._hostInfoArr.length; i++ ) {
      address = this._hostInfoArr[i].address ;
      try {
         var ssh = new Ssh( address, this._hostInfoArr[i].rootUserName, 
            this._hostInfoArr[i].rootPassword, this._hostInfoArr[i].sshPort ) ;
         this._hostInfoArr[i].rootSshObj = ssh ;
      } catch( e ) {
         errMsg = "can not use root account to ssh to host[" + address + "]" ;
         exp = new SdbError( e, errMsg )
         logger.log( PDERROR, exp ) ;
         throw exp ; 
      }
      try {
         var ssh = new Ssh( address, this._hostInfoArr[i].sdbUserName, 
            this._hostInfoArr[i].sdbPassword, this._hostInfoArr[i].sshPort ) ;
         this._hostInfoArr[i].adminSshObj = ssh ;
      } catch( e ) {
         errMsg = "can not use sdb admin account to ssh to host[" + address + "]" ;
         exp = new SdbError( e, errMsg )
         logger.log( PDERROR, exp ) ;
         throw exp ; 
      }
   }
} ;

ConfigMgr.prototype._appendIP = function ConfigMgr__appendIP() {
   var exp       = null ;
   var errMsg    = null ;
   var uniqueHostInfoArr = [] ;
   var uniqueIPArr       = [] ;
   var errMsgArr         = [] ;
   for ( var i = 0; i < this._hostInfoArr.length; i++ ) {
      var address = this._hostInfoArr[i].address ;
      var ssh = this._hostInfoArr[i].rootSshObj ;
      try {
         var ip = ssh.getPeerIP() ;
         if ( !isString(ip) ) {
            errMsg = sprintf( "the return ip of host[?] is " + 
               "not a string", address ) ;
            errMsgArr.push( errMsg ) ;
            if ( exp == null ) exp = new SdbError( SDB_INVALIDARG ) ;
            logger.log( PDERROR, errMsg ) ;
            continue ;
         }
         ip = strTrim( removeBreak(ip) ) ;
         if ( this.arrayContainElement( uniqueIPArr, ip ) ) {
            continue ;
         } else {
            uniqueIPArr.push( ip ) ;
            // append ip info
            this._hostInfoArr[i].ip = ip ;
            uniqueHostInfoArr.push( this._hostInfoArr[i] ) ;
         }
      } catch( e ) {
         errMsg = "failed to get ip of host[" + address + "]" ;
         errMsgArr.push( new SdbError( e, errMsg ).toString() ) ;
         if ( exp == null ) exp = new SdbError( SDB_INVALIDARG ) ;
         logger.log( PDERROR, errMsg ) ;
      }
   }
   if ( exp != null ) {
      throw new SdbError( SDB_SYS, errMsgArr.toString() ) ;
   } else {
      try {
         this._hostInfoArr = uniqueHostInfoArr.splice( 0 ) ;
      } catch( e ) {
         exp = new SdbError( e, 
            "failed to copy unique host info array to local" ) ;
         logger.log( PDERROR, exp ) ;
         throw exp ;
      }
   }
} ;


ConfigMgr.prototype._appendHostName = function ConfigMgr__appendHostName() {
   var errMsg    = null ;
   var errMsgArr = [] ;
   var exp       = null ;
   for ( var i = 0; i < this._hostInfoArr.length; i++ ) {
      var address = this._hostInfoArr[i].address ;
      var ssh = this._hostInfoArr[i].rootSshObj ;
      try {
         var hostName = ssh.exec("hostname") ;
         if ( !isString(hostName) ) {
            errMsg = sprintf( "the return hostname of host[?] is " + 
               "not a string", address ) ;
            errMsgArr.push( errMsg ) ;
            if ( exp == null ) exp = new SdbError( SDB_INVALIDARG ) ;
            logger.log( PDERROR, errMsg ) ;
            continue ;
         }
         hostName = strTrim( removeBreak( hostName ) ) ;
         // append host name and ip info
         this._hostInfoArr[i].hostName = address ;
      } catch( e ) {
         errMsg = sprintf( "failed to get hostname of host[?]", address ) ;
         errMsgArr.push( new SdbError( e, errMsg ).toString() ) ;
         if ( exp == null ) exp = new SdbError( SDB_INVALIDARG ) ;
         logger.log( PDERROR, errMsg ) ;
      }
   }
   if ( exp != null ) {
      throw new SdbError( SDB_SYS, errMsgArr.toString() ) ;
   }
} ;

ConfigMgr.prototype._appendHostNameToModuleInfo =
   function ConfigMgr__appendHostNameToModuleInfo() {
      var arr = [] ;
      var len = this._hostInfoArr.length ;
      for ( var i = 0; i < len; i++ ) {
         var hostName  = this._hostInfoArr[i].hostName ;
         var obj = new Object() ;
         obj[HostName] = hostName ;
         arr.push( obj ) ;
      }
      this._moduleInfo[Location] = arr ;
   } ;

ConfigMgr.prototype._appendRemoteInstalledInfo = 
   function ConfigMgr__appendRemoteInstalledInfo() {
   var errMsg    = null ;
   var errMsgArr = [] ;
   var exp       = null ;
   
   for ( var i = 0; i < this._hostInfoArr.length; i++ ) {
      var infoObj        = new Object() ;
      var ssh            = this._hostInfoArr[i].rootSshObj ;
      var address        = this._hostInfoArr[i].address ;
      var localFileName  = address + "_" + ts ;
      var remoteFileName = OMA_FILE_INSTALL_INFO ;
      if ( !ssh.isPathExist(remoteFileName) ) {
         errMsg = "file[" + remoteFileName + 
            "] does not exist in host[" + address + "]" ;
         errMsgArr.push( errMsg ) ;
         if ( exp == null ) exp = new SdbError( SDB_INVALIDARG ) ;
         logger.log( PDERROR, errMsg ) ;
         continue ;
      }
      try {
         ssh.pull( remoteFileName, localFileName ) ;
      } catch( e ) {
         errMsg = "failed to pull file[" + remoteFileName + 
            "] from host[" + address + "] to local" ;
         errMsgArr.push( new SdbError( e, errMsg ).toString() ) ;
         if ( exp == null ) exp = new SdbError( SDB_INVALIDARG ) ;
         logger.log( PDERROR, errMsg ) ;
         continue ;
      }
      try {
         infoObj = eval( '(' + Oma.getOmaConfigs(localFileName) + ')' ) ;
      } catch( e ) {
         errMsg = "failed to extract installed info from file[" + localFileName + 
            "] which is get back from host[" + address + "]" ;
         errMsgArr.push( new SdbError( e, errMsg ).toString() ) ;
         if ( exp == null ) exp = new SdbError( SDB_INVALIDARG ) ;
         logger.log( PDERROR, errMsg ) ;
         continue ;
      } finally {
         try { File.remove(localFileName) ; } catch(e) {}
      }
      this._hostInfoArr[i].installedInfo = infoObj ;
   }
   if ( exp != null ) {
      throw new SdbError( SDB_SYS, errMsgArr.toString() ) ;
   }
} ;

ConfigMgr.prototype._secondlyCheck = 
   function ConfigMgr__secondlyCheck() {
   var errMsg     = null ;
   var errMsgArr  = [] ;
   var exp        = null ;
   var address    = null ;
   var programArr = 
      [ "sdb", "sdbcmd", "sdbcm", "sequoiadb", "sdbcmart", "sdbcmtop" ] ;
   logger.log( PDEVENT, "begin to secondly check" ) ;
   // 1. check the specified admin accounts are the same with 
   // the one in remote or not
   for ( var i = 0; i < this._hostInfoArr.length; i++ ) {
      var host        = this._hostInfoArr[i] ;
      // check the sdb admin user is the same with the one in remote or not
      if ( host.installedInfo[SDBADMIN_USER] != host.sdbUserName ) {
         errMsg = "the giving sdb admin account is different from " + 
            "the one in host[" + host.address + "]" ;
         errMsgArr.push( errMsg ) ;
         if ( exp == null ) exp = new SdbError( SDB_INVALIDARG ) ;
         logger.log( PDERROR, errMsg ) ;
         continue ;
      }
   }
   if ( exp != null ) {
      throw new SdbError( SDB_INVALIDARG, errMsgArr.toString() ) ;
   }
   
   // 2. check the specified install paths are the same with
   // the one in remote or not
   for ( var i = 0; i < this._hostInfoArr.length; i++ ) {
      var host = this._hostInfoArr[i] ;
      var path = host.installedInfo[INSTALL_DIR] ;
      var ssh  = host.rootSshObj ;
      // install path exist or not
      if ( !ssh.isPathExist(path) ) {
         errMsg = "path [" + path + "] does not exist in host[" + 
            host.address + "]" ;
         errMsgArr.push( errMsg ) ;
         if ( exp == null ) exp = new SdbError( SDB_INVALIDARG ) ;
         logger.log( PDERROR, errMsg ) ;
         continue ;
      }
      // check whether pivotal executable program exist or not
      var tmpArr = [] ;
      var exp2   = null ;
      for ( var j = 0; j < programArr.length; j++ ) {
         var program = adaptPath( path ) + "bin/" + programArr[j] ;
         if ( !ssh.isPathExist( program) ) {
            tmpArr.push( programArr[j] ) ;
            if ( exp2 == null ) exp2 = new SdbError( SDB_INVALIDARG ) ;
            continue ;
         }
      }
      if ( exp2 != null ) {
         errMsg = sprintf( "program? does not exist in host[?]", 
            tmpArr, host.address ) ;
         errMsgArr.push( errMsg ) ;
         logger.log( PDERROR, errMsg ) ;
         exp = exp2 ;
      }
   }
   if ( exp != null ) {
      throw new SdbError( SDB_INVALIDARG, errMsgArr.toString() ) ;
   }
   logger.log( PDEVENT, "finishing secondly check" ) ;
} ;

ConfigMgr.prototype._appendInstalledPath = 
   function ConfigMgr__appendInstalledPath() {
   var errMsg    = null ;
   var errMsgArr = [] ;
   var exp       = null ;
   for ( var i = 0; i < this._hostInfoArr.length; i++ ) {
      var host          = this._hostInfoArr[i] ;
      var address       = host.address ;
      var installedInfo = host.installedInfo ;
      try {
         // append installPath
         this._hostInfoArr[i].installPath = 
            adaptPath( installedInfo[INSTALL_DIR] ) ;
      } catch( e ) {
         errMsg = sprintf( "failed to append installed path of host[?] " + 
               "to local", address ) ;
         errMsgArr.push( new SdbError( e, errMsg ).toString() ) ;
         if ( exp == null ) exp = new SdbError( SDB_INVALIDARG ) ;
         logger.log( PDERROR, errMsg ) ;
      }
   }
   if ( exp != null ) {
      throw new SdbError( SDB_SYS, errMsgArr.toString() ) ;
   }
} ;

ConfigMgr.prototype._collectCoordInfoFromHost = 
   function ConfigMgr__collectCoordInfoFromHost() {
   var retArr = [] ;
   var exp    = null ;
   var cmd1   = " var arr = Sdbtool.listNodes({type:\"db\", role:\"coord\", mode:\"local\", expand:true}); " ;
   var cmd2   = " arr.size(); " ;
   var cmd3   = " arr.next(); " ;
   var qCmd   = " quit " ;
   
   for ( var i = 0; i < this._hostInfoArr.length; i++ ) {
      var host        = this._hostInfoArr[i] ;
      var ssh         = host.rootSshObj ;
      var installPath = host.installPath ;
      
      this.execCommand( ssh, installPath, cmd1, false ) ;
      var retStr = this.execCommand( ssh, installPath, cmd2, false ) ;
      var size   = 0 ;
      try {
         size = this.getMatchedSize( retStr ) ;
      } catch( e ) {
         exp = new SdbError( e, "failed to get the number of matched coords" ) ;
         logger.log( PDERROR, exp ) ;
         try { 
            this.execCommand( ssh, installPath, qCmd, true ) ; 
         } catch( e ) {}
         throw exp ;
      }
      logger.log( PDEVENT, 
         sprintf( "there has [?] coord(s) in host[?]", size, ssh.getPeerIP() ) ) ;
      for ( var j = 0; j < size; j++ ) {
         var obj = null ;
         retStr  = this.execCommand( ssh, installPath, cmd3, false ) ;
         try {
            obj  = this.changeToObj( retStr ) ;
         } catch( e ) {
            exp = new SdbError( e, 
               sprintf( "failed to change [?] which is returned by " + 
                  "executing command [?] in host[?] to an object", 
                  retStr, cmd3, ssh.getPeerIP() ) ) ;
            logger.log( PDERROR, exp ) ;
            try { 
               this.execCommand( ssh, installPath, qCmd, true ) ; 
            } catch( e ) {}
            throw exp ;
         }
         obj[FIELD_CONF_HOSTNAME] = host.hostName ;
         obj[FIELD_CONF_IP]       = host.ip ;
         retArr.push( obj ) ;
      }
   }
   logger.log( PDDEBUG, 
      sprintf( "collect [?] coord(s) from the giving host(s), " + 
         "they are as below: ", retArr.length ) ) ;
   for ( var i = 0; i < retArr.length; i++ ) {
       logger.log( PDDEBUG, JSON.stringify(retArr[i]) ) ;
   }
   this._coordInfoFromHost = retArr ;
} ;

ConfigMgr.prototype._getCatalogRGAddress = 
   function ConfigMgr__getCatalogRGAddress ( ip, port, user, password ) {
   var retArr = [] ;
   var db     = null ;

   try {
      db      = new Sdb( ip, port, user, password ) ;
      var cur = db.list( SDB_LIST_GROUPS, {"GroupName": "SYSCatalogGroup"} ) ;
      var record = cur.next().toObj() ;
      var catalogNodeArr = record[Group] ;
      if ( !isArray(catalogNodeArr) ) {
         throw new SdbError( SDB_SYS, 
            sprintf( "the return catalog node info[?] is " + 
               "not an array", catalogNodeArr ) ) ;
      }
      for ( var i = 0; i < catalogNodeArr.length; i++ ) {
         var node     = catalogNodeArr[i] ;
         var hostName = node[HostName] ;
         var svcName  = null ;
         var svcArr   = node[Service] ;
         for ( var j = 0; j < svcArr.length; j++ ) {
            if ( svcArr[j][Type] == 3 ) {
               svcName = svcArr[j][Name] ;
               break ;
            }
         }
         if ( svcName == null ) {
            throw new SdbError( SDB_SYS, "no port info" ) ;
         }
         retArr.push( hostName + ":" + svcName ) ;
      }
   } catch( e ) {
      var exp = new SdbError( e, sprintf( "failed to get catalog address " + 
         "of coord[?:?] from database", ip, port ) ) ;
      logger.log( PDWARNING, exp ) ;
      throw exp ;
   } finally {
      if ( db != null ) {
         try { db.close() } catch(e) {}
      }
   }
   return retArr ;
} ;

ConfigMgr.prototype._extractCoordInfo = 
   function ConfigMgr__extractCoordInfo() {
   var faultNodeNum = 0 ;
   var authUserName = this._getField( FIELD_DB_AUTH_USER ) ;
   var authPassword = this._getField( FIELD_DB_AUTH_PASSWD ) ;
   var coordNodeNum = this._coordInfoFromHost.length ;
   for ( var i = 0; i < coordNodeNum; i++ ) {
      var wapper          = new CoordInfoWrapper() ;
      var obj             = new CoordInfo() ;
      var coordInfoObj    = this._coordInfoFromHost[i] ;
      var catalogAddrArr  = [] ;
      obj.HostName        = coordInfoObj[FIELD_CONF_HOSTNAME] ;
      obj.dbpath          = coordInfoObj[FIELD_CONF_DBPATH] ;
      obj.Service[0].Name = coordInfoObj[FIELD_CONF_SVCNAME] ;
      obj.Service[1].Name = coordInfoObj[FIELD_CONF_REPLNAME] ;
      obj.Service[2].Name = coordInfoObj[FIELD_CONF_CATALOGNAME] ;
      wapper.infoObj      = obj ;
      wapper.hostName     = coordInfoObj[FIELD_CONF_HOSTNAME] ;
      wapper.svnName      = coordInfoObj[FIELD_CONF_SVCNAME] ;
      wapper.ip           = coordInfoObj[FIELD_CONF_IP] ;
      // get catalog address from db, but if we can not connect to db,
      // let's get it from config file
      try {
         catalogAddrArr = 
            this._getCatalogRGAddress( wapper.ip, wapper.svnName,
                                       authUserName, authPassword ) ;
         logger.log( PDEVENT, 
            sprintf( "catalog address of coord[?:?] get from database is: ?", 
               wapper.ip, wapper.svnName, JSON.stringify(catalogAddrArr) ) ) ;
      } catch( e ) {
         faultNodeNum++ ;
         var exp = new SdbError( e, sprintf( "failed to catalog address of " +
            "coord[?:?] from database, going to get it from config file",
            wapper.ip, wapper.svnName) ) ;
         logger.log( PDWARNING, exp ) ;
         catalogAddrArr = coordInfoObj[FIELD_CONF_CATALOGADDR].split( ',' ) ;
         logger.log( PDEVENT, 
            sprintf( "catalog address of coord[?:?] get from config file is: ?", 
               wapper.ip, wapper.svnName, JSON.stringify(catalogAddrArr) ) ) ;
      }
      if ( faultNodeNum == coordNodeNum ) {
         throw new SdbError( SDB_SYS, "no coord is available" ) ;
      }
      for ( var j = 0; j < catalogAddrArr.length; j++ ) {
         wapper.cataAddrArr.push( removeQuotes( catalogAddrArr[j] ) ) ; 
      }
      // keep coord info in local array
      this._coordInfoInFormat.push( wapper ) ;
   }
   logger.log( PDEVENT, 
      sprintf( "after extracting, [?] coord(s) info are as below: ", 
         this._coordInfoInFormat.length ) ) ;
   for ( var i = 0 ; i < this._coordInfoInFormat.length; i++ ) {
      logger.log( PDEVENT, JSON.stringify(this._coordInfoInFormat[i]) ) ;
   }
} ;

ConfigMgr.prototype._thirdlyCheck = function ConfigMgr__thirdlyCheck() {
   var exp         = null ;
   var catalogArr  = [] ;
   logger.log( PDEVENT, "begin to thirdly check" ) ;
   // check whether there is only one cluster in the giving hosts or not   
   for ( var i = 0; i < this._coordInfoInFormat.length; i++ ) {
      var hasMatchedElement = false ;
      var coordWrapper      = this._coordInfoInFormat[i] ;
      var cataAddrArr       = coordWrapper.cataAddrArr ;
      if ( catalogArr.length == 0 ) {
         for ( var j = 0; j < cataAddrArr.length; j++ ) {
            catalogArr.push( cataAddrArr[j] ) ;
         }
         continue ;
      }
      for ( var m = 0; m < cataAddrArr.length; m++ ) {
         try {
            if ( this.arrayContainElement( catalogArr, cataAddrArr[m] ) ) {
               catalogArr = this.mergeArray( catalogArr, cataAddrArr ) ;
               hasMatchedElement = true ;
               break ;
            }
         } catch ( e ) {
            exp = new SdbError( e, 
               sprintf( "failed to merge array[?] and array[?]", 
                  catalogArr, cataAddrArr ) ) ;
            logger.log( PDERROR, exp ) ;
            throw exp ;
         }
      }
      if ( !hasMatchedElement ) {
         exp = new SdbError( SDB_INVALIDARG, 
            sprintf( "there are more than one cluster in the giving " + 
               "host(s), we get one cluster which catalog is [?], " +
               "and another cluter which catalog is [?], " + 
               "and we do not know which to choice", 
               catalogArr, cataAddrArr ) ) ;
         logger.log( PDERROR, exp ) ;
         throw exp ;
      }
   }
   logger.log( PDEVENT, "finishing thirdly check" ) ;
} ;

ConfigMgr.prototype._appendDBInfo = function ConfigMgr__appendDBInfo() {
   var exp          = null ;
   var coordWrapper = null ;
   var hostName     = null ;
   var svcName      = null ;
   var authUserName = null ;
   var authPassword = null ;

   if ( this._coordInfoInFormat.length == 0 ) {
      exp = new SdbError( SDB_SYS, "no coord node in the giving host(s)" ) ;
      logger.log( PDERROR, exp ) ;
      throw exp ;
   }
   // get auth info
   authUserName = this._getField( FIELD_DB_AUTH_USER ) ;
   authPassword = this._getField( FIELD_DB_AUTH_PASSWD ) ;
   // try to get db info
   for ( var i = 0; i < this._coordInfoInFormat.length; i++ ) {
      exp          = null ;
      coordWrapper = this._coordInfoInFormat[i] ;
      hostName     = coordWrapper.hostName ;
      svcName      = coordWrapper.svnName ;
      logger.log( PDEVENT, 
         sprintf( "connect to coord[?:?]", hostName, svcName ) ) ;
      try {
         this._sdb = new Sdb( hostName, svcName, 
                              authUserName, authPassword ) ;
         break ;
      } catch( e ) {
         errMsg = "failed to connect to coord[" + 
            hostName + ":" + svcName + "]" ;
         exp = new SdbError( e, errMsg ) ;
         logger.log( PDWARNING, exp ) ;
      }
   }
   if ( exp != null ) {
      exp = new SdbError( SDB_SYS, "no coord is available for connecting" ) ;
      logger.log( PDERROR, exp ) ;
      throw exp ;
   }
} ;

ConfigMgr.prototype._appendRGInfo = function ConfigMgr__appendRGInfo() {
   var exp    = null ;
   var cur    = null ;
   var record = null ;
   try {
      cur = this._sdb.listReplicaGroups() ;
   } catch ( e ) {
      var exp = new SdbError( e, "failed to get replica group's info" ) ;
      logger.log( PDERROR, exp ) ;
      throw exp ;
   }
   this._rgInfoArr = [] ;
   while ( (record = cur.next()) != undefined ) {
      var obj = record.toObj() ;
      this._rgInfoArr.push( obj ) ;
   }
} ;

ConfigMgr.prototype._getHostNameFromRGInfo = 
   function ConfigMgr__getHostNameFromRGInfo() {
   var errMsg       = null ;
   var exp          = null ;
   var retArr       = [] ;

   for ( var i = 0; i < this._rgInfoArr.length; i++ ) {
      var obj      = this._rgInfoArr[i] ;
      var arr      = obj[Group] ;
      for ( var j = 0; j < arr.length; j++ ) {
         var subObj   = arr[j] ;
         var hostName = subObj[HostName] ;
         retArr.push( hostName ) ;
      }
   }
   return retArr ;
} ;

ConfigMgr.prototype._fouthlyCheck = 
   function ConfigMgr__fouthlyCheck() {
   var errMsg  = null ;
   var hostArr = [] ;
   var exp     = null ;

   logger.log( PDEVENT, "begin to fouthly check" ) ;
   // get the host from the return rg info
   var rgHostArr = this._getHostNameFromRGInfo() ;
   for ( var i = 0; i < rgHostArr.length; i++ ) {
      var hostName = rgHostArr[i] ;
      var isMatch  = false ;
      for ( var j = 0; j < this._hostInfoArr.length; j++ ) {
         if ( hostName == this._hostInfoArr[j].hostName || 
              hostName == this._hostInfoArr[j].ip ) {
            isMatch = true ;
            break ;
         }
      }
      if ( !isMatch ) {
         var hasContained = false ;      
         for ( var m = 0; m < hostArr.length; m++ ) {
            if ( hostName == hostArr[m] ) {
               hasContained = true ;
               break ;
            }
         }
         if ( !hasContained ) {
            hostArr.push( hostName ) ;
         }
         if ( exp == null ) exp = new SdbError( SDB_INVALIDARG ) ;
         continue ;
      }
   }
   if ( exp != null ) {
      errMsg = "should offer information about the follow hosts[" + 
         hostArr.toString() + "], for some nodes exist in those hosts" ;
      exp = new SdbError( SDB_INVALIDARG, errMsg ) ;
      logger.log( PDERROR, exp ) ;
      throw exp ;
   }
   logger.log( PDEVENT, "finishing fouthly check" ) ;
} ;

ConfigMgr.prototype._doit = function ConfigMgr__doit() {
   this._init() ;
   if ( this._getAction() == ACTION_REMOVE_OM ) {
      return ;
   }
   // appending info about build om
   this._appendDBInstallPacket() ;
   if ( this._getAction() == ACTION_BUILD_OM ) {
      return ;
   }
   // appending info
   this._appendAddress() ;
   this._appendRootInfo() ;
   this._appendSshPort() ;
   this._appendSdbAdminInfo() ;
   this._appendOMAdminInfo() ;
   this._appendClusterInfo() ;
   this._appendModuleInfo() ;
   this._appendDBConfigOption() ;
   // checking
   this._firstlyCheck() ;
   // appending info
   this._appendSshObj() ;
   this._appendIP() ;
   this._appendHostName() ;
   this._appendHostNameToModuleInfo() ;
   this._appendRemoteInstalledInfo() ;
   // checking
   this._secondlyCheck() ; 
   // appending info
   this._appendInstalledPath() ;
   this._collectCoordInfoFromHost() ;
   this._extractCoordInfo() ;
   // checking
   this._thirdlyCheck() ;
   // appending info
   this._appendDBInfo() ;
   this._appendRGInfo() ;
   // checking
   this._fouthlyCheck() ;
} ;

var RGInfoMgr = function( rgInfoArr ) {
   this._rgInfoArr = rgInfoArr ;
   this._init() ;
} ;

RGInfoMgr.prototype._init = function RGInfoMgr__init() {
   if ( !isArray(this._rgInfoArr) || this._rgInfoArr.length == 0 ) {
      var exp = new SdbError( SDB_SYS, 
         "no replica group's info in catalog, RGInfoMgr::_rgInfoArr is: " + 
         this._rgInfoArr ) ;
      logger.log( PDERROR, exp ) ;
      throw exp ;
   }
} ;

RGInfoMgr.prototype._getUsedNodeIDFromDB = 
   function RGInfoMgr__getUsedNodeIDFromDB() {
   var retArr  = [] ;
   var nodeArr = [] ;
   var rgArr   = [] ;
   var exp     = null ;
   var len     = this._rgInfoArr.length ;
   
   // get catalog and coord rg's info
   for ( var i = 0; i < len; i++ ) {
      var info = this._rgInfoArr[i] ;
      if ( info[GroupName] == "SYSCatalogGroup" || 
           info[GroupName] == "SYSCoord" ) {
         rgArr.push( info ) ;
      }
   }
   if ( rgArr.lenth == 0 ) {
      exp = new SdbError( SDB_SYS, "no catalog or coord group's info" ) ;
      logger.log( PDERROR, exp ) ;
      throw exp ;
   }
   // get catalog or coord node id
   for ( var i = 0; i < rgArr.length; i++ ) {
      var rgInfo = rgArr[i] ;
      nodeArr = rgInfo[Group] ;
      if ( nodeArr == null ) {
         if ( rgInfo[GroupName] == "SYSCatalogGroup" ) {
            exp = new SdbError( SDB_SYS, "no node's info in catalog group" ) ;
            logger.log( PDERROR, exp ) ;
            throw exp ;      
         } else {
            // coord group may have no node's info, let's skip
            continue ;
         }
      }
      for ( var j = 0; j < nodeArr.length; j++ ) {
         var nodeID = nodeArr[j][FIELD_COORD_INFO_NODEID] ;
         if ( !isNumber(nodeID) ) {
            exp = new SdbError( SDB_SYS, 
               "get invalid node id[" + nodeID + 
               "] from group " + rgInfo[GroupName] ) ;
            logger.log( PDERROR, exp ) ;
            throw exp ;
         }
         retArr.push( nodeID ) ;
      }
   }

   logger.log( PDEVENT, 
      "the used node id of coord or catalog group are as below: " + retArr ) ;
   return retArr ;
} ;

RGInfoMgr.prototype._genNodeID = function RGInfoMgr__genNodeID( num ) {
   var retArr        = [] ;
   var exp           = null ;
   var usedNodeIDArr = this._getUsedNodeIDFromDB() ;
   var id            = 1 ;
   var maxID         = 1000 ;

   for ( var i = 0, id = 1; i < num && id < maxID; i++ ) {
      var isMatch = false ;
      for ( var j = 0; j < usedNodeIDArr.length; j++ ) {
         if ( id == usedNodeIDArr[j] ) {
            isMatch = true ;
            break ;
         }
      }
      if ( !isMatch ) {
         retArr.push(id) ;
         usedNodeIDArr.push(id) ;
      } else {
         i-- ;
      }
      id++ ;
   }
   logger.log( PDEVENT, 
      sprintf( "expect ? node id(s), actually get ?, " + 
               "the return nodeID array is: ?", 
               num, retArr.length, retArr.toString() ) ) ;
   if ( retArr.length != num ) {
      exp = new SdbError( "SDB_SYS", 
         "no invalid node id for the coord nodes" ) ;
      logger.log( PDERROR, exp ) ;
   }
   return retArr ;
} ;

RGInfoMgr.prototype._getNodeAddrFromDB = 
   function RGInfoMgr__getNodeAddrFromDB( role, type ) {
   var retArr   = [] ;
   var nodeArr  = null ;
   var exp      = null ;
   var rgInfo   = null ;
   var rgName   = null ;
   var portType = null ;

   if ( type == "local" ) portType = 0 ;
   else if ( type == "catalog" ) portType = 3 ;
   
   if ( role == "catalog" ) {
      rgName   = "SYSCatalogGroup" ;
   } else if ( role == "coord" ) {
      rgName   = "SYSCoord" ;
   } else {
      throw new SdbError( PDERROR, "invalid role: " + role ) ;
   }
   
   // get rg's info
   for ( var i = 0; i < this._rgInfoArr.length; i++ ) {
      var info = this._rgInfoArr[i] ;
      if ( rgName == info[GroupName] ) {
         rgInfo = info ;
         break ;
      }
   }
   if ( rgInfo == null ) {
      logger.log( PDWARNING, 
         role + " group's info does not exist in catalog" ) ;
      return retArr ;
   }
   // get node's info
   nodeArr = rgInfo[Group] ;
   if ( nodeArr == null ) {
      logger.log( PDWARNING, 
         role + " group does not contain any nodes" ) ;
      return retArr ;
   }
   for ( var i = 0; i < nodeArr.length; i++ ) {
      var node     = nodeArr[i] ;
      var hostName = node[HostName] ;
      var svcArr   = node[Service] ;
      var nodeInfo = null ;
      for ( var j = 0; j < svcArr.length; j++ ) {
         var svc  = svcArr[j] ;
         var type = svc[Type] ;
         if ( portType == type ) {
            nodeInfo = hostName + ":" + svc[Name] ;
            break ;
         }
      }
      if ( nodeInfo != null ) {
         retArr.push( nodeInfo ) ;
      }
   }
   logger.log( PDEVENT, 
      sprintf( "group[?] has ? node(s) which type" +
         " is [?] as below: [?] ", role, retArr.length, portType, retArr ) ) ;
   return retArr ;  
} ;

RGInfoMgr.prototype._getCataAddrFromDB = 
   function RGInfoMgr__getCataAddrFromDB( type ) {
   var exp         = null ;
   var cataAddrArr = this._getNodeAddrFromDB( "catalog", type ) ;
   if ( cataAddrArr == null || cataAddrArr.length == 0 ) {
      exp = new SdbError( SDB_SYS, 
         "can not get any info about catalog group from database" ) ;
      logger.log( PDERROR, exp ) ;
      throw exp ;
   }
   logger.log( PDEVENT, 
      "the catalog address with the type[" + type + "] is: " + cataAddrArr ) ;
   return cataAddrArr ;
} ;

RGInfoMgr.prototype._getCoordAddrFromDB = 
   function RGInfoMgr__getCoordAddrFromDB( type ) {
   var retArr = this._getNodeAddrFromDB( "coord", type ) ;
   logger.log( PDEVENT, 
      "the coord address with the type[" + type + "] is: " + retArr ) ;
   return retArr ;
} ;

RGInfoMgr.prototype._getOriginalCoordRGInfo = 
   function RGInfoMgr__getOriginalCoordRGInfo() {
   var retObj = null ;
   // get coord rg
   for ( var i = 0; i < this._rgInfoArr.length; i++ ) {
      var info = this._rgInfoArr[i] ;
      if ( "SYSCoord" == info[GroupName] ) {
         retObj = info ;
         break ;
      }
   }
   if ( retObj == null ) {
      logger.log( PDWARNING, "database has no coord group" ) ;
   }
   return retObj ;
} ;

RGInfoMgr.prototype._getOriginalCoordNodes = 
   function RGInfoMgr__getOriginalCoordRGNodes() {
   var retArr     = [] ;
   var coordRGObj = this._getOriginalCoordRGInfo() ;
   
   if ( coordRGObj == null || coordRGObj[Group] == null ) {
      logger.log( PDWARNING, "there has no any coord nodes" ) ;
      return retArr ;
   }
   var nodeArr = coordRGObj[Group] ;
   for ( var i = 0; i < nodeArr.length; i++ ) {
      retArr.push( nodeArr[i] ) ;
   }
   return retArr ;
} ;

var UpdateCoord = function( configMgr ) { 
   this._configMgr               = configMgr ;
   // helper var
   this._hostInfoArr             = null ;
   this._rgInfoMgr               = null ;
   this._sdb                     = null ;
   this._originalCoordRGInfo     = null ;
   this._hostCoordInfoWrapperArr = null ;
   // local var
   this._newCoordInfoArr         = null ;
} ;

UpdateCoord.prototype._init = function UpdateCoord__init() {
   this._hostInfoArr             = this._configMgr._getHostInfoArr() ;
   this._sdb                     = this._configMgr._getSdbObj() ;
   this._rgInfoMgr               = 
      new RGInfoMgr( this._configMgr._getRGInfoArr( false ) ) ;
   this._hostCoordInfoWrapperArr = this._configMgr._getCoordInfoWrapperArr() ;
   this._originalCoordRGInfo     = this._rgInfoMgr._getOriginalCoordRGInfo() ;
   // should set the log level to event,
   // we want the original coord group info is recorded in log
   logger.log( PDEVENT, 
      "the original coord group info is: " + 
      JSON.stringify( this._originalCoordRGInfo ) ) ;
} ;

UpdateCoord.prototype._appendNodeID =
   function UpdateCoord__appendNodeID( matchedCoordInfoArr ) {
   var retArr    = [] ;
   var nodeIDArr = [] ;
   // get usable node id
   nodeIDArr = this._rgInfoMgr._genNodeID( matchedCoordInfoArr.length ) ;
   // generate inserted records
   for ( var i = 0; i < matchedCoordInfoArr.length; i++ ) {
      var obj                      = matchedCoordInfoArr[i] ;
      obj[FIELD_COORD_INFO_NODEID] = nodeIDArr[i] ;
      retArr.push( obj ) ;
   }
   logger.log( PDEVENT, 
      sprintf( "finish appending node id for [?] coord(s)", retArr.length ) ) ;
   if ( retArr.length > 0 ) {
      logger.log( PDDEBUG, "the coord with node id are as below: " ) ;
      for ( var i = 0; i < retArr.length; i++ ) {
         logger.log( PDDEBUG, JSON.stringify(retArr[i]) ) ;
      }
   }
   return retArr ;
} ;

UpdateCoord.prototype._closeConnections = 
   function UpdateCoord__closeConnections( connArr ) {
   for ( var i = 0; i < connArr.length; i++ ) {
      try { connArr[i].close() ; } catch( e ) {}
   }
} ;

UpdateCoord.prototype._buildUpdateRule = 
   function UpdateCoord__buildUpdateRule( type ) {
   var retObj = null ;
   try {
      var recordObj          = new Object() ;
      var subObj             = new Object() ;
      if ( type == "rebuild" ) {
         subObj[Group]       = this._addExistedCoordInfo() ; ;
         recordObj["$set"]   = subObj ;
         retObj              = recordObj ;
      } else if ( type == "rollback" ) {
         if ( this._originalCoordRGInfo != null ) {
            recordObj["$set"] = this._originalCoordRGInfo ;
            retObj            = recordObj ;
         } else {
            // in this case, that means the original coord rg does not exist
            // so we don't need to build update rule
            retObj            = null ;
         }
      }
   } catch( e ) {
      var exp = new SdbError( e, 
         sprintf( "failed to build update coord rule with the type \'?\'",
            type ) ) ;
      logger.log( PDERROR, exp ) ;
      throw exp ;
   }
   logger.log( PDEVENT, sprintf( "the updated rule for [?] is: ", 
      type ) + JSON.stringify( retObj ) ) ;
   return retObj ;
} ;

UpdateCoord.prototype._buildCond =
   function UpdateCoord__buildCond() {
   var retObj = null ;
   var exp    = null ;
   var obj    = new Object() ;
   var subObj = new Object() ;
   subObj["$et"]  = "SYSCoord" ;
   obj[GroupName] = subObj ;
   retObj = obj ;
   return retObj ;
} ;

UpdateCoord.prototype._buildUpdateCond =
   function UpdateCoord__buildUpdateCond() {
   var retObj = null ;
   try {
      retObj = this._buildCond() ;
   } catch( e ) {
      var exp = new SdbError( e,
         "failed to build update coord info's condition" ) ;
      logger.log( PDERROR, exp ) ;
      throw exp ;
   }
   logger.log( PDEVENT, "update coord info's condition is: " + 
      JSON.stringify( retObj ) ) ;
   return retObj ;
} ;

UpdateCoord.prototype._buildQueryCond =
   function UpdateCoord__buildQueryCond() {
   var retObj = null ;
   try {
      retObj = this._buildCond() ;
   } catch( e ) {
      var exp = new SdbError( e,
         "failed to build query coord info's condition" ) ;
      logger.log( PDERROR, exp ) ;
      throw exp ;
   }
   logger.log( PDEVENT, "query coord info's condition is: " + 
      JSON.stringify( retObj ) ) ;
   return retObj ;
} ;

UpdateCoord.prototype._addExistedCoordInfo = 
   function UpdateCoord__addExistedCoordInfo() {
   var retArr             = this._newCoordInfoArr ;
   var originalCoordNodes = this._rgInfoMgr._getOriginalCoordNodes() ;
   logger.log( PDDEBUG, 
      "the original coord info is: " + JSON.stringify(originalCoordNodes) ) ;
   logger.log( PDDEBUG, 
      "the new coord info is: " + JSON.stringify(this._newCoordInfoArr) ) ;
   for ( var i = 0; i < originalCoordNodes.length; i++ ) {
      retArr.push( originalCoordNodes[i] ) ;
   }
   logger.log( PDDEBUG, 
      "the total coord info is: " + JSON.stringify(retArr) ) ;
   return retArr ;
} ;

UpdateCoord.prototype._rollback = 
      function UpdateCoord__rollback( rollbackCLArr, rule, cond ) {
   var exp       = null ;
   var errMsg    = null ;
   var errMsgArr = [] ;

   for ( var i = 0; i < rollbackCLArr.length; i++ ) {
      var cl = null ;
      try {
         cl = rollbackCLArr[i] ;
         logger.log( PDEVENT, 
            sprintf( "begin to recover coord info to table[?]", cl ) ) ;
         if ( rule != null ) {
            cl.update( rule, cond ) ;
         } else {
            // in this case, "SYSCoord" does not exist in catalog group
            // so, let's remove the inserted coord info
            cl.remove( cond ) ; 
         }
         logger.log( PDEVENT, 
            sprintf( "succeed to recover coord info to table[?]", cl ) ) ;
      } catch( e ) {
         exp = new SdbError( e, 
            sprintf( "failed to recover the original coord info in [?] with " + 
               "the rule [?] and condition [?]", cl, 
               JSON.stringify( rule ), JSON.stringify( cond ) ) ) ;
         errMsg = exp.toString() ;
         errMsgArr.push( errMsg ) ;
         logger.log( PDERROR, errMsg ) ;
         println( errMsg ) ;
      }
   }
} ;

UpdateCoord.prototype._getNewCoordInfo = 
   function UpdateCoord__getNewCoordInfo() {
   var exp       = null ;
   var resultArr = [] ;

   // get existing coord's info from database
   var dbCoordAddrArr = this._rgInfoMgr._getCoordAddrFromDB( "local" ) ;
   // get catalog's info from database
   var dbCatalogAddrArr = this._rgInfoMgr._getCataAddrFromDB( "catalog" ) ;

   if ( !isArray(dbCatalogAddrArr) || dbCatalogAddrArr.length == 0 ) {
      exp = new SdbError( SDB_SYS, 
         "no catalog address for comparing, dbCatalogAddrArr is: " + 
         dbCatalogAddrArr ) ;
      logger.log( SDB_SYS, exp ) ;
      throw exp ;
   }
   // pick up the coord info which should be in current cluster but 
   // have not been included in
   for ( var i = 0; i < this._hostCoordInfoWrapperArr.length; i++ ) {
      var hostCoordInfo = this._hostCoordInfoWrapperArr[i].infoObj ;
      var hostName     = hostCoordInfo[FIELD_COORD_INFO_HOSTNAME] ;
      var svcName      = null ;
      var needRestart  = false ;
      for ( var j = 0; j < 2; j++ ) {
         var type = 
            hostCoordInfo[FIELD_COORD_INFO_SERVICE][j][FIELD_COORD_INFO_TYPE] ;
         if ( type == 0 ) {
            svcName = 
               hostCoordInfo[FIELD_COORD_INFO_SERVICE][j][FIELD_COORD_INFO_NAME] ;
            break ;
         }
      }
      var hostCoordAddr = hostName + ":" + svcName ;
      // filter the coord which info has been in catalog
      for ( var j = 0; j < dbCoordAddrArr.length; j++ ) {
         var dbCoordAddr = dbCoordAddrArr[j] ;
         if ( hostCoordAddr == dbCoordAddr ) {
            needRestart = true ;
            break ;
         }
      }
      if ( needRestart ) continue ;
      // if the coord does not contain in catalog, it's see whether 
      // whether the coord belong to current cluster or not
      var hostCataAddrArr = 
         this._hostCoordInfoWrapperArr[i].cataAddrArr ;
      if ( !isArray(hostCataAddrArr) || hostCataAddrArr.length == 0 ) {
         logger.log( PDWARNING, "no catalog info in coord node[" +
            hostCoordAddr + "], hostCataAddrArr is: " + hostCataAddrArr ) ;
         continue ;
      }
      for ( var m = 0; m < hostCataAddrArr.length; m++ ) {
         var hostCataAddr = hostCataAddrArr[m] ;
         var isMatch      = false ;
         for ( var n = 0; n < dbCatalogAddrArr.length; n++ ) {
            var dbCatalogAddr = dbCatalogAddrArr[n] ;
            if ( hostCataAddr == dbCatalogAddr ) {
               isMatch = true ;
               break ;
            }
         }
         if ( isMatch ) {
            resultArr.push( hostCoordInfo ) ;
            break ;
         }
      }
   }
   this._newCoordInfoArr = this._appendNodeID( resultArr ) ;
   logger.log( PDEVENT, sprintf( "there has another [?] existed coord(s) for " + 
         "updating into catalog", this._newCoordInfoArr.length ) ) ;
   if ( this._newCoordInfoArr.length > 0 ) {
   logger.log( PDDEBUG, "they are as below: " ) ;
      for ( var i = 0; i < this._newCoordInfoArr.length; i++ ) {
         logger.log( PDDEBUG, JSON.stringify(this._newCoordInfoArr[i]) ) ;
      }
   }
} ;

UpdateCoord.prototype._prepareCoordRG = 
   function UpdateCoord__prepareCoordRG() {
   var num = null ;
   var exp = null ;

   try {
      var cur = this._sdb.list( SDB_LIST_GROUPS, this._buildQueryCond() ) ;
      num     = cur.size() ;
   } catch( e ) {
      exp = new SdbError( e, 
         "failed to check whether coord group existed or not" ) ;
      logger.log( PDERROR, exp ) ;
      throw exp ;
   }
   if ( num == 0 ) {
      logger.log( PDEVENT, 
         "there is no coord group in current cluster, let's build one" ) ;
      try {
         this._sdb.createRG( "SYSCoord" ) ;
      } catch ( e ) {
         exp = new SdbError( e, "failed to build coord group" ) ;
         logger.log( PDERROR, exp ) ;
         throw exp ;
      }
      logger.log( PDEVENT, "finish building coord group" ) ;
   } else if ( num != 1 ) {
      exp = SdbError( SDB_SYS, "invalid amount of coord group: " + num ) ;
      logger.log( PDERROR, exp ) ;
      throw exp ;
   }
} ;

UpdateCoord.prototype._UpdateCoord = 
   function UpdateCoord__UpdateCoord() {
   var exp           = null ;
   var cdbArr        = [] ;
   var clArr         = [] ;
   var rollbackCLArr = [] ;

   var cataAddrArr = this._rgInfoMgr._getCataAddrFromDB( "local" ) ;
   var rule1       = this._buildUpdateRule( "rebuild" ) ;
   var rule2       = this._buildUpdateRule( "rollback" ) ;
   var cond        = this._buildUpdateCond() ;

   for ( var i = 0; i < cataAddrArr.length; i++ ) {
      try {
         var cdb = new Sdb( cataAddrArr[i] ) ;
         cdbArr.push( cdb ) ;
         var cl  = cdb.getCS( "SYSCAT" ).getCL( "SYSNODES" ) ;
         clArr.push( cl ) ;
      } catch( e ) {
         exp = new SdbError( e, 
            "failed to get system table 'SYSCAT.SYSNODES' from catalog[" + 
            cataAddrArr[i] + "]" ) ;
         logger.log( PDERROR, exp ) ;
         // disconnect
         this._closeConnections( cdbArr ) ;
         throw exp ;
      }
   }
   for ( var i = 0; i < clArr.length; i++ ) {   
      var cl = clArr[i] ;
      // update
      try {
         logger.log( PDEVENT, 
            sprintf( "begin to update coord info to catalog[?]", 
               cataAddrArr[i] ) ) ;
         cl.update( rule1, cond ) ;
         logger.log( PDEVENT, 
            sprintf( "succeed to update coord info to catalog[?]", 
               cataAddrArr[i] ) ) ;
      } catch( e ) {
         exp = new SdbError( e, 
            "failed to update coord info to table 'SYSCAT.SYSNODES' " + 
            "in catalog[" + cataAddrArr[i] + "]" ) ;
         logger.log( PDERROR, exp ) ;
         rollbackCLArr.push( cl ) ;
         // rollback
         this._rollback( rollbackCLArr, rule2, cond ) ;
         // disconnect
         this._closeConnections( cdbArr ) ;
         throw exp ;
      }
      rollbackCLArr.push( cl ) ;
   }
   // disconnect
   this._closeConnections( cdbArr ) ;
   
} ;

UpdateCoord.prototype._doit = function UpdateCoord__doit() {
   // init
   this._init() ;
   this._getNewCoordInfo() ;
   if ( this._newCoordInfoArr.length == 0 ) {
      logger.log( PDEVENT, "no new coord info for updating" ) ;
      return ;
   } else {
      logger.log( PDEVENT, "the new coord info for updating is: " + 
         JSON.stringify( this._newCoordInfoArr ) ) ;
   }
   this._prepareCoordRG() ;
   this._UpdateCoord() ;
   logger.log( PDEVENT, "succeed to update all the coord's info into catalog" ) ;
} ;

var CheckHost = function( configMgr, nodeDataPathArr ) {
   this._configMgr       = configMgr ;
   this._nodeDataPathArr = nodeDataPathArr ;
   this._hostInfoArr     = null ;
   this._resultArr       = [] ;
} ;

CheckHost.prototype = new Helper() ;

CheckHost.prototype._getCheckHostResult = 
   function CheckHost__getCheckHostResult() {
   return this._resultArr ;
} ;

CheckHost.prototype._init = function CheckHost__init() {
   this._hostInfoArr = this._configMgr._getHostInfoArr() ;
   var clusterInfo = this._configMgr._getClusterInfo() ;
   var clusterName = clusterInfo.ClusterName ;
   
   for ( var i = 0; i < this._hostInfoArr.length; i++ ) {
      var obj          = new CheckHostInfo() ;
      var host         = this._hostInfoArr[i] ;
      obj[ClusterName]    = clusterName ;
      obj[IP]             = host.ip ;
      obj[HostName]       = host.hostName ;
      obj[User]           = host.rootUserName ;
      obj[Passwd]         = host.rootPassword ;
      obj[InstallPath]    = host.installPath ;
      // should return a string for SshPort
      obj[SshPort]        = host.sshPort + "" ;
      this._resultArr.push( obj ) ;
   }
} ;

CheckHost.prototype._collOMAInfo = 
   function CheckHost__collOMAInfo( ssh, installPath, sdbUser ) {
   var exp         = null ;
   var retObj      = new OMAInfo() ;
   retObj[SdbUser] = sdbUser ;
   retObj[Path]    = adaptPath(installPath) ;

   var vStr = adaptPath(installPath) + OMA_PROG_BIN_SDBCM + " --version " ;
   var cmd1 = 
      " var arr = Sdbtool.listNodes({role:\"cm\", mode:\"local\"}, {type:\"sdbcm\"}); " ;
   var cmd2 = " arr.size(); " ;
   var cmd3 = " arr.next(); " ;
   var cmd4 = " var cmd = new Cmd(); cmd.run(\" " + vStr + " \"); " ;
   var qCmd = " quit " ;

   // test is there any omagent in target host or not
   this.execCommand( ssh, installPath, cmd1, false ) ;
   var retStr = this.execCommand( ssh, installPath, cmd2, false ) ;
   var size   = 0 ;
   try {
      size = parseInt( retStr ) ;
   } catch( e ) {
      exp = new SdbError( e, 
         sprintf( "the result of executing command[?] in host[?] is [?], " +
            "but failed to parse it to a number", 
            cmd2, ssh.getPeerIP(), retStr ) ) ;
      logger.log( PDERROR, exp ) ;
      try { 
         this.execCommand( ssh, installPath, qCmd, true ) ;
      } catch(e) {}
      throw exp ;
   }
   if ( !isNumber(size) ) {
      exp = new SdbError( SDB_SYS, 
         sprintf( "the amount of sdbcm in host[?] is [?], " +
            "and it's not a number", ssh.getPeerIP(), size ) ) ;
      logger.log( PDERROR, exp ) ;
      try { 
         this.execCommand( ssh, installPath, qCmd, true ) ; 
      } catch(e) {}
      throw exp ;
   }
   logger.log( PDEVENT, 
      sprintf( "thers has [?] sdbcm in host [?]", size, ssh.getPeerIP() ) ) ;
   if ( size == 0 ) {
      try { 
         this.execCommand( ssh, installPath, qCmd, true ) ; 
      } catch(e) {}
      return retObj() ;
   }
   retStr = this.execCommand( ssh, installPath, cmd3, false ) ;
   // get omagent service info
   try {
      var omaInfoObj  = this.changeToObj( retStr ) ;
      retObj[Service] = omaInfoObj[SvcName3] ;
   } catch( e ) {
      exp = new SdbError( e, 
         sprintf( "failed to get sdbcm service info in host[?]", 
            ssh.getPeerIP() ) ) ;
      logger.log( PDERROR, exp ) ;
      try { 
         this.execCommand( ssh, installPath, qCmd, true ) ; 
      } catch(e) {}
      throw exp ;
   }
   // get the version and release info of omgent
   retStr = this.execCommand( ssh, installPath, cmd4, false ) ;
   try {
      var beg = retStr.indexOf( OMA_MISC_OM_VERSION ) ;
      var end = retStr.indexOf( '\n' ) ;
      var len = OMA_MISC_OM_VERSION.length ;
      retObj[Version] = retStr.substring( beg + len, end ) ;
      beg = retStr.indexOf( OMA_MISC_OM_RELEASE ) ;
      len = OMA_MISC_OM_RELEASE.length ;
      subStr = retStr.substring( beg + len, retStr.length ) ;
      retObj[Release] = parseInt( subStr ) ;
   } catch( e ) {
      var exp = new SdbError( e, 
         sprintf( "failed to get sdbcm version and release info in host[?]", 
            ssh.getPeerIP() ) ) ;
      logger.log( PDERROR, exp ) ;
      throw exp ;
   } finally {
      try { 
         this.execCommand( ssh, installPath, qCmd, true ) ; 
      } catch(e) {}
   }

   return retObj ;
} ;

CheckHost.prototype._collOSInfo = 
   function CheckHost__collOSInfo( ssh, installPath ) {
   var retObj = new OSInfo() ;
   var str    = this.execCommand( ssh, installPath, SYSTEM_OS_INFO, true ) ;
   try {
      var obj = this.changeToObj( str ) ;
      retObj[Distributor] = obj[Distributor] ;
      retObj[Release]     = obj[Release] ;
      retObj[Description] = obj[Description] ;
      retObj[Bit]         = obj[Bit] ;
   } catch( e ) {
      var exp = new SdbError( e, 
         sprintf( "failed to get os info of host[?]", ssh.getPeerIP() ) ) ;
      logger.log( PDERROR, exp ) ;
      throw exp ;
   }
   
   return retObj ;
} ;

CheckHost.prototype._collCPUInfo = 
   function CheckHost__collCPUInfo( ssh, installPath ) {
   var infoObjArr = [] ;
   try {
      var str        = this.execCommand( ssh, installPath,
                                         SYSTEM_CPU_INFO, true ) ;
      var cpuInfo   = this.changeToObj( str ) ;
      var arr        = cpuInfo[Cpus] ;
      for ( var i = 0; i < arr.length; i++ )
      {
         var obj     = arr[i] ;
         var info    = new CPUInfo() ;
         // not offer ID and Model
         info[ID]    = "" ;
         info[Model] = obj[Info] ;
         info[Core]  = obj[Core] ;
         info[Freq]  = obj[Freq] ;
         infoObjArr.push( info ) ;
      }
   } catch( e ) {
      var exp = new SdbError( e, 
         sprintf( "failed to get cpu info of host[?]", ssh.getPeerIP() ) ) ;
      logger.log( PDERROR, exp ) ;
      return [] ;
   }
   return infoObjArr ;
} ;

CheckHost.prototype._collMemInfo = 
   function CheckHost__collMemInfo( ssh, installPath ) {
   var retObj = new MemoryInfo() ;
   var str    = this.execCommand( ssh, installPath, SYSTEM_MEM_INFO, true ) ;

   try {
      var obj = this.changeToObj( str ) ;
      // model is not offer
      retObj[Model] = "" ;
      retObj[Size]  = obj[Size] ;
      retObj[Free]  = obj[Free] ;
   } catch( e ) {
      exp = new SdbError( e, 
         sprintf( "failed to get memery info of host[?]", ssh.getPeerIP() ) ) ;
      logger.log( PDERROR, exp ) ;
      throw exp ;
   }
   
   return retObj ;
} ;

CheckHost.prototype._isMatchedDisk = 
   function CheckHost__isMatchedDisk( hostName, mountPath ) {
   var retValue     = false ;

   for ( var i = 0; i < this._nodeDataPathArr.length; i++ ) {
      if ( this._nodeDataPathArr[i][HostName] != hostName ) {
         continue ;
      }
      var pathArr = this._nodeDataPathArr[i][Path] ;
      for( var j = 0; j < pathArr.length; j++ ) {
         var match = adaptPath( pathArr[j] ).match( adaptPath( mountPath ) ) ;
         if ( match != null ) {
            retValue = true ;
            break ;
         }
      }
      break ;
   }
   return retValue ;
} ;

CheckHost.prototype._collDiskInfo = 
   function CheckHost__collDiskInfo( ssh, installPath, hostName ) {
   var infoObjArr = [] ;
   var str        = this.execCommand( ssh, installPath, 
                                       SYSTEM_DISK_INFO, true ) ;
   try {
      var diskInfo   = this.changeToObj( str ) ;
      var arr        = diskInfo[Disks] ;
      for ( var i = 0; i < arr.length; i++ )
      {
         var obj       = arr[i] ;
         var info      = new DiskInfo() ;
         info[Name]    = obj[Filesystem] ;
         info[Mount]   = obj[Mount] ;
         info[Size]    = obj[Size] ;
         info[Free]    = obj[Size] - obj[Used] ;
         info[IsLocal] = obj[IsLocal] ;
         if ( this._isMatchedDisk( hostName, info[Mount] ) ) {
            // we just save disk which were used by user
            infoObjArr.push( info ) ;
         }
      }
   } catch( e ) {
      var exp = new SdbError( e, 
         sprintf( "failed to get disk info of host[?]", ssh.getPeerIP() ) ) ;
      logger.log( PDERROR, exp ) ;
      throw exp ;
   }

   return infoObjArr ;
} ;

CheckHost.prototype._collNetCardInfo = 
   function CheckHost__collNetCardInfo( ssh, installPath ) {
   var infoObjArr = [] ;
   var str        = this.execCommand( ssh, installPath, 
                                       SYSTEM_NET_INFO, true ) ;
   try {
      var netCardInfo   = this.changeToObj( str ) ;
      var arr           = netCardInfo[Netcards] ;
      for ( var i = 0; i < arr.length; i++ )
      {
         var obj         = arr[i] ;
         var info        = new NetInfo() ;
         info[Name]      = obj[Name] ;
         // not offer Model and bandwidth 
         info[Model]     = "" ;
         info[Bandwidth] = "" ;
         info[IP]        = obj[Ip] ;
         infoObjArr.push( info ) ;
      }
   } catch( e ) {
      var exp = new SdbError( e, 
         sprintf( "failed to get net card info of host[?]", ssh.getPeerIP() ) ) ;
      logger.log( PDERROR, exp ) ;
      throw exp ;
   }

   return infoObjArr ;
} ;

CheckHost.prototype._collPortInfo = 
   function CheckHost__collPortInfo( ssh, installPath ) {
   var retObj = new PortInfo() ;
   return retObj ;
} ;

CheckHost.prototype._collServiceInfo = 
   function CheckHost__collServiceInfo() {
   var retObj = new ServiceInfo() ;
   return retObj ;} ;

CheckHost.prototype._collSafetyInfo = 
   function CheckHost__collSafetyInfo( ssh, installPath ) {
   var retObj = new SafetyInfo() ;
   return retObj ;
} ;

CheckHost.prototype._doit = function CheckHost__doit() {
   this._init() ;
   
   for ( var i = 0; i < this._hostInfoArr.length; i++ ) {
      var ssh         = this._hostInfoArr[i].rootSshObj ;
      var installPath = this._hostInfoArr[i].installPath ;

      this._resultArr[i][OMA]     = 
         this._collOMAInfo( ssh, installPath, 
                            this._hostInfoArr[i].sdbUserName ) ;
      this._resultArr[i][OS]      = this._collOSInfo( ssh, installPath ) ;
      this._resultArr[i][CPU]     = this._collCPUInfo( ssh, installPath ) ;
      this._resultArr[i][Memory]  = this._collMemInfo( ssh, installPath ) ;
      this._resultArr[i][Disk]    = 
         this._collDiskInfo( ssh, installPath, this._hostInfoArr[i].hostName ) ;
      this._resultArr[i][Net]     = this._collNetCardInfo( ssh, installPath ) ;
      this._resultArr[i][Port]    = [] ;
      this._resultArr[i][Service] = [] ;
      this._resultArr[i][Safety]  = this._collSafetyInfo( ssh, installPath ) ;
      // add agent service
      this._resultArr[i][AgentService] = this._resultArr[i][OMA][Service] ;
   }

   logger.log( PDDEBUG, 
      sprintf( "there has [?] record(s) as the result for checking host as below: ", 
         this._resultArr.length ) ) ;
   for ( var i = 0; i < this._resultArr.length; i++ ) {
      logger.log( PDDEBUG, JSON.stringify( this._resultArr[i] ) ) ;
   }
   logger.log( PDEVENT, "succeed to check all the host's info" ) ;
} ;

var CollectNodeInfo = function( configMgr ) {
   this._configMgr     = configMgr ;
   this._hostInfoArr   = null ;
   this._nodeArr       = [] ;
   this._resultArr     = [] ;
} ;

CollectNodeInfo.prototype = new Helper() ;

CollectNodeInfo.prototype._init = function CollectNodeInfo__init() {
   this._hostInfoArr  = this._configMgr._getHostInfoArr() ;
} ;

CollectNodeInfo.prototype._getNodeConfResult = 
   function CollectNodeInfo__getNodeConfResult() {
   //return this._resultArr ;
   var retArr = [] ;
   for( var i = 0; i < this._resultArr.length; i++ ) {
      var confInfoObj = new Object() ;
      var wrapper     = this._resultArr[i] ;
      confInfoObj[BusinessName] = wrapper[BusinessName] ;
      confInfoObj[HostName]     = wrapper[HostName] ;
      confInfoObj[ClusterName]  = wrapper[ClusterName] ;
      confInfoObj[BusinessType] = "sequoiadb" ;
      confInfoObj[DeployMod]    = "distribution" ;
      var adapterArr            = wrapper[Adapter] ;
      var configArr             = [] ;
      for ( var j = 0; j < adapterArr.length; j++ ) {
         configArr.push( adapterArr[j][Config] ) ;
      }
      confInfoObj[Config]       = configArr ;
      retArr.push( confInfoObj ) ;
   }
   logger.log( PDDEBUG, sprintf( "there has [?] record(s) as the result for " + 
      "collecting the config info of nodes, " + 
      "they are as below:", retArr.length ) ) ;
   for ( var i = 0; i < retArr.length; i++ ) {
      logger.log( PDDEBUG, JSON.stringify(retArr[i]) ) ;
   }
   
   return retArr ;
} ;

CollectNodeInfo.prototype._getNodeDataPaths = 
   function CollectNodeInfo__getNodeDataPaths() {
   var retArr = [] ;
   for( var i = 0; i < this._resultArr.length; i++ ) {
      var nodeInfo            = new NodeInfo() ;
      var nodeConfInfoWrapper = this._resultArr[i] ;
      nodeInfo[HostName] = nodeConfInfoWrapper.HostName ;
      var adapterArr = nodeConfInfoWrapper[Adapter] ;
      for ( var j = 0; j < adapterArr.length; j++ ) {
         var adapter = adapterArr[j] ;
         nodeInfo[Path].push( adapter[Config][FIELD_CONF_DBPATH] ) ;
      }
      retArr.push( nodeInfo ) ;
   }
   logger.log( PDDEBUG, 
      "the db path of current cluster are as below: " + 
      JSON.stringify(retArr) ) ;
   return retArr ;
} ;

CollectNodeInfo.prototype._getNodeConfPaths = 
   function CollectNodeInfo__getNodeConfPaths() {
   var retArr = [] ;
   for( var i = 0; i < this._resultArr.length; i++ ) {
      var nodeInfo            = new NodeInfo() ;
      var nodeConfInfoWrapper = this._resultArr[i] ;
      nodeInfo[HostName] = nodeConfInfoWrapper.HostName ;
      var adapterArr = nodeConfInfoWrapper[Adapter] ;
      for ( var j = 0; j < adapterArr.length; j++ ) {
         var adapter = adapterArr[j] ;
         nodeInfo[Path].push( adapter[FIELD_CONF_CONFPATH] ) ;
      }
      retArr.push( nodeInfo ) ;
   }
   logger.log( PDDEBUG, 
      "the node's config path of current cluster are as below: " +
      JSON.stringify( retArr ) ) ;
   return retArr ;
} ;

CollectNodeInfo.prototype._getField = 
   function CollectNodeInfo__getField( nodeInfo, field ) {
   var exp      = null ;
   var retValue = null ;
   try {
      retValue = nodeInfo[field] ;
      if ( retValue == null ) {
         throw new SdbError( SDB_SYS, 
            sprintf( "the value of field[?] is null", field ) ) ;
      }
   } catch( e ) {
      exp = new SdbError( e, 
         sprintf( "failed to get the value of field[?] in node's " +
                  "config object", field ) ) ;
      logger.log( PDERROR, "node's config info object is: " + 
                  JSON.stringify(nodeInfo) ) ;
      logger.log( PDERROR, exp ) ;
      throw exp ;
   }
   // whatever it is, return a string
   return retValue + "" ;
} ;

CollectNodeInfo.prototype._extractInfo = 
   function CollectNodeInfo__extractInfo( nodeInfoObj ) {
   var retObj  = new NodeConfInfoAdapter() ; 
   var confObj = new NodeConfInfo() ;
   var rgName = this._getField( nodeInfoObj, FIELD_CONF_GROUPNAME ) ;
   if ( rgName == OMA_SYS_CATALOG_RG || rgName == OMA_SYS_COORD_RG ) {
      rgName = "" ;
   } ;
   retObj[Config]                        = confObj ;
   retObj[FIELD_CONF_CONFPATH]           =
      this._getField( nodeInfoObj, FIELD_CONF_CONFPATH ) ;
   confObj[FIELD_CONF_DATAGROUPNAME]     = rgName ;
   confObj[FIELD_CONF_DBPATH]            = 
      this._getField( nodeInfoObj, FIELD_CONF_DBPATH ) ;
   confObj[FIELD_CONF_SVCNAME]           = 
      this._getField( nodeInfoObj, FIELD_CONF_SVCNAME ) ;
   confObj[FIELD_CONF_ROLE]              = 
      this._getField( nodeInfoObj, FIELD_CONF_ROLE ) ;
   confObj[FIELD_CONF_ARCHIVECOMPRESSON] = 
      this._getField( nodeInfoObj, FIELD_CONF_ARCHIVECOMPRESSON ).toLowerCase() ;
   confObj[FIELD_CONF_ARCHIVEEXPIRED] = 
      this._getField( nodeInfoObj, FIELD_CONF_ARCHIVEEXPIRED ) ;
   confObj[FIELD_CONF_ARCHIVEON] = 
      this._getField( nodeInfoObj, FIELD_CONF_ARCHIVEON ).toLowerCase() ;
   confObj[FIELD_CONF_ARCHIVEPATH] = 
      this._getField( nodeInfoObj, FIELD_CONF_ARCHIVEPATH ) ;
   confObj[FIELD_CONF_ARCHIVEQUOTA] = 
      this._getField( nodeInfoObj, FIELD_CONF_ARCHIVEQUOTA ) ;
   confObj[FIELD_CONF_ARCHIVETIMEOUT] = 
      this._getField( nodeInfoObj, FIELD_CONF_ARCHIVETIMEOUT ) ;
   confObj[FIELD_CONF_BKUPPATH] = 
      this._getField( nodeInfoObj, FIELD_CONF_BKUPPATH ) ;
   confObj[FIELD_CONF_DIAGLEVEL]         = 
      this._getField( nodeInfoObj, FIELD_CONF_DIAGLEVEL ) ;
   confObj[FIELD_CONF_HJBUF]             = 
      this._getField( nodeInfoObj, FIELD_CONF_HJBUF ) ;
   confObj[FIELD_CONF_INDEXPATH] = 
      this._getField( nodeInfoObj, FIELD_CONF_INDEXPATH ) ;
   confObj[FIELD_CONF_LOBMETAPATH] = 
      this._getField( nodeInfoObj, FIELD_CONF_LOBMETAPATH ) ;
   confObj[FIELD_CONF_LOBPATH] = 
      this._getField( nodeInfoObj, FIELD_CONF_LOBPATH ) ;
   confObj[FIELD_CONF_LOGBUFFSIZE]       = 
      this._getField( nodeInfoObj, FIELD_CONF_LOGBUFFSIZE ) ;
   confObj[FIELD_CONF_LOGFILENUM]        = 
      this._getField( nodeInfoObj, FIELD_CONF_LOGFILENUM ) ;
   confObj[FIELD_CONF_LOGFILESZ]         = 
      this._getField( nodeInfoObj, FIELD_CONF_LOGFILESZ ) ;
   confObj[FIELD_CONF_MAXPREFPOOL]       = 
      this._getField( nodeInfoObj, FIELD_CONF_MAXPREFPOOL ) ;
   confObj[FIELD_CONF_NUMPRELOAD]        = 
      this._getField( nodeInfoObj, FIELD_CONF_NUMPRELOAD ) ;
   confObj[FIELD_CONF_MAXSYNCJOB]        = 
      this._getField( nodeInfoObj, FIELD_CONF_MAXSYNCJOB ) ;
   confObj[FIELD_CONF_PREFEREDINSTANCE]  = 
      this._getField( nodeInfoObj, FIELD_CONF_PREFEREDINSTANCE ) ;
   confObj[FIELD_CONF_SORTBUF]           = 
      this._getField( nodeInfoObj, FIELD_CONF_SORTBUF ) ;
   confObj[FIELD_CONF_SYNCDEEP] = 
      this._getField( nodeInfoObj, FIELD_CONF_SYNCDEEP ).toLowerCase() ;
   confObj[FIELD_CONF_SYNCINTERVAL] = 
      this._getField( nodeInfoObj, FIELD_CONF_SYNCINTERVAL ) ;
   confObj[FIELD_CONF_SYNCRECORDNUM] = 
      this._getField( nodeInfoObj, FIELD_CONF_SYNCRECORDNUM ) ;
   confObj[FIELD_CONF_SYNCSTRATEGY]      = 
      this._getField( nodeInfoObj, FIELD_CONF_SYNCSTRATEGY ).toLowerCase() ;
   confObj[FIELD_CONF_TRANSACTION]       = 
      this._getField( nodeInfoObj, FIELD_CONF_TRANSACTION ).toLowerCase() ;
   confObj[FIELD_CONF_WEIGHT]            = 
      this._getField( nodeInfoObj, FIELD_CONF_WEIGHT ) ;

   return retObj ;
} ;

CollectNodeInfo.prototype._doit = function CollectNodeInfo__doit() {
   var exp  = null ;
   var cmd1 = " var arr = Sdbtool.listNodes( " + 
              " {type:\"all\", expand:true, mode:\"local\"}, " + 
              " {type: \"sequoiadb\"} ) ; " ;
   var cmd2 = " arr.size(); " ;
   var cmd3 = " arr.next(); " ;
   var qCmd = " quit " ;
   
   this._init() ;
   var moduleInfo = this._configMgr._getModuleInfo() ;
   for ( var i = 0; i < this._hostInfoArr.length; i++ ) {
      var ssh                           = this._hostInfoArr[i].rootSshObj ;
      var installPath                   = this._hostInfoArr[i].installPath ;
      var nodeConfInfoWrapper           = new NodeConfInfoWrapper() ;
      nodeConfInfoWrapper[ClusterName]  = moduleInfo[ClusterName] ;
      nodeConfInfoWrapper[BusinessName] = moduleInfo[BusinessName] ;
      nodeConfInfoWrapper[HostName]     = this._hostInfoArr[i].hostName ;
      // get node's config info
      this.execCommand( ssh, installPath, cmd1, false ) ;
      var retStr = this.execCommand( ssh, installPath, cmd2, false ) ;
      var size   = 0 ;
      try {
         size = parseInt( retStr ) ;
      } catch( e ) {
         exp = new SdbError( e, 
            sprintf( "the result of executing command[?] in host[?] is [?], " +
               "but failed to parse it to a number", 
               cmd2, ssh.getPeerIP(), retStr ) ) ;
         logger.log( PDERROR, exp ) ;
         try { 
            this.execCommand( ssh, installPath, qCmd, true ) ;
         } catch(e) {}
         throw exp ;
      }
      if ( !isNumber(size) ) {
         exp = new SdbError( SDB_SYS, 
            sprintf( "the amount of nodes in host[?] is [?], " +
               "and it's not a number", ssh.getPeerIP(), size ) ) ;
         logger.log( PDERROR, exp ) ;
         try { 
            this.execCommand( ssh, installPath, qCmd, true ) ; 
         } catch(e) {}
         throw exp ;
      }
      logger.log( PDEVENT, 
         sprintf( "thers has [?] nodes in host [?]", size, ssh.getPeerIP() ) ) ;
      if ( size == 0 ) {
         try { 
            this.execCommand( ssh, installPath, qCmd, true ) ; 
         } catch(e) {}
         continue ;
      }
      // extract node's config info
      for ( var j = 0; j < size; j++ ) {
         retStr = this.execCommand( ssh, installPath, cmd3, false ) ;
         var nodeInfoObj = null ;
         try {
            nodeInfoObj = this.changeToObj( retStr ) ;
         } catch( e ) {
            exp = new SdbError( e, 
               sprintf( "failed to change [?] which is returned by " + 
                  "executing command [?] in host[?] to an object", 
                  retStr, cmd3, ssh.getPeerIP() ) ) ;
            logger.log( PDERROR, exp ) ;
            try { 
               this.execCommand( ssh, installPath, qCmd, true ) ; 
            } catch( e ) {}
            throw exp ;
         }
         // extract the fileds we need
         var nodeConfInfoAdapter = null ;
         try {
             nodeConfInfoAdapter = this._extractInfo( nodeInfoObj ) ;
         } catch( e ) {
            exp = new SdbError( e, 
               sprintf( "failed to extract node's config info in host[?]", 
                  ssh.getPeerIP() ) ) ;
            logger.log( PDERROR, exp ) ;
            try { 
               this.execCommand( ssh, installPath, qCmd, true ) ;
            } catch(e) {}
             throw exp ;
         }
         nodeConfInfoWrapper[Adapter].push( nodeConfInfoAdapter ) ;
      }
      // keep the extracted info in local array
      this._resultArr.push( nodeConfInfoWrapper ) ;
      // close sdbbp in remote
      try { 
         this.execCommand( ssh, installPath, qCmd, true ) ;
      } catch(e) {
      }
   }
} ;

var FlushConfig = function( configMgr, nodeConfPathArr ) {
   this._configMgr        = configMgr ;
   this._nodeConfPathArr  = nodeConfPathArr ;
   this._configInfoObj    = null ;
   this._hostInfoArr      = null ;
   this._flushedConfigArr = [] ;
} ;

FlushConfig.prototype = new Helper() ;

FlushConfig.prototype._init = function FlushConfig__init() {
   this._hostInfoArr    = this._configMgr._getHostInfoArr() ;
   this._configInfoObj  = this._configMgr._getDBConfigInfo() ;
} ;

FlushConfig.prototype._getHostAdminSshObj = 
   function FlushConfig__getHostSshObj( host ) {
   var retObj = null ;
   var exp    = null ;
   for ( var i = 0; i < this._hostInfoArr.length; i++ ) {
      var hostName = this._hostInfoArr[i].hostName ;
      if ( host == hostName ) {
         retObj = this._hostInfoArr[i].adminSshObj ;
         break ;
      }
   }
   if ( retObj == null ) {
      exp = new SdbError( SDB_SYS, 
         sprintf( "no sdb admin ssh object of host[?]", host ) ) ;
      logger.log( exp ) ;
      throw exp ;
   }
   return retObj ;
} ;

FlushConfig.prototype._getHostInstallPath = 
   function FlushConfig__getHostInstallPath( host ) {
   var retPath = null ;
   var exp     = null ;
   for ( var i = 0; i < this._hostInfoArr.length; i++ ) {
      var hostName = this._hostInfoArr[i].hostName ;
      if ( host == hostName ) {
         retPath = this._hostInfoArr[i].installPath ;
         break ;
      }
   }
   if ( retPath == null ) {
      exp = new SdbError( SDB_SYS, 
         sprintf( "no install path of host[?]", host ) ) ;
      logger.log( exp ) ;
      throw exp ;
   }
   return retPath ;
} ;

FlushConfig.prototype._buildConfigOptionStr = 
   function FlushConfig__buildConfigOptionStr( originalObjStr, 
                                               newObjStr, omAddr ) {
   var exp         = null ;
   var retStr      = null ;
   var retObj      = null ;
   var originalStr = removeBreak( originalObjStr ) ;
   var newStr      = removeBreak( newObjStr ) ;
   var pos         = originalStr.lastIndexOf( '}' ) ;
   if ( pos == -1 ) {
      exp = SdbError( SDB_SYS, 
         sprintf( "invalid original node config info[?]", originalStr ) ) ;
      logger.log( PDERROR, exp ) ;
      throw exp ;
   }
   var subOriginalStr = originalStr.substring( 0, pos ) ;
   if ( strTrim(subOriginalStr) != '{') {
      subOriginalStr += ", " ;
   }
   pos = newStr.indexOf( "{" ) ;
   if ( pos == -1 ) {
      exp = SdbError( SDB_SYS, 
         sprintf( "invalid new node config info[?]", newStr ) ) ;
      logger.log( PDERROR, exp ) ;
      throw exp ;
   }
   var subNewStr    = newStr.substring( pos + 1 ) ;
   var configObjStr = subOriginalStr + " \"omaddr\" : \"" + 
                      omAddr + "\" , " + subNewStr ;
   try {
      retObj = this.changeToObj( configObjStr ) ;
      retStr = JSON.stringify( retObj ) ;
   } catch( e ) {
      exp = new SdbError( e, 
         sprintf( "failed to build complete config option info, " +
            "according to string[?]", configObjStr ) ) ;
      logger.log( PDERROR, exp ) ;
      throw exp ;
   }
   return retStr ;
} ;

FlushConfig.prototype._flush = 
   function FlushConfig__flush( ssh, installPath, 
                                confFile, configOptionStr ) {
   var cmd = " var option = " + configOptionStr + "; " ;
   this.execCommand( ssh, installPath, cmd, false ) ;
   cmd = " Oma.setOmaConfigs( option, \"" + confFile + "\"); " ;
   this.execCommand( ssh, installPath, cmd, true ) ;
} ;

FlushConfig.prototype._rollback = function FlushConfig__rollback() {
   var exp = null ;
   logger.log( PDEVENT, 
      "going to recover the config info which we had modified" ) ;
   for ( var i = 0; i < this._flushedConfigArr.length; i++ ) {
      var flushedConfig = null ;
      try {
         flushedConfig = this._flushedConfigArr[i] ;
         this._flush( flushedConfig.adminSshObj, 
                      flushedConfig.installPath,
                      flushedConfig.confFile,
                      flushedConfig.originalStr ) ;
      } catch( e ) {
         exp = new SdbError( e, 
            sprintf( "failed to recover the original config info[?] " + 
               "to config file[?] in host[?]",
               flushedConfig.originalStr,
               flushedConfig.confFile, flushedConfig.hostName ) ) ;
         logger.log( PDERROR, exp ) ;
         println( exp.toString() ) ;
      }
   }
   if ( exp == null ) {
      logger.log( PDEVENT, "succeed to recover all the modified config info" ) ;
   } else {
      logger.log( PDEVENT, "recovery is incomplete" ) ;
   }
} ;

FlushConfig.prototype._doit = function FlushConfig__doit() {
   var exp      = null ;
   var hostName = null ;
   var confFile = null ;
   var original = null ;
   var omAddr   = null ;
   
   this._init() ;
   
   // get om address
   if ( this.omExist() ) {
      try {
         omAddr = this.getOMAddr() ;
      } catch( e ) {
         exp = new SdbError( e, "failed to get local om's address" ) ;
         logger.log( PDERROR, exp ) ;
         throw exp ;
      }
   } else {
      exp = new SdbError( SDBCM_NODE_NOTEXISTED, 
                          "need to flush om's address to nodes' " + 
                          "config file, but om does not exist in local host" ) ;
      logger.log( PDERROR, exp ) ;
      throw exp ;
   }
   
   try {
      for ( var i = 0; i < this._nodeConfPathArr.length; i++ ) {
         hostName                 = null ;
         confFile                 = null ;
         original                 = null ;
         var hostNodeConfPathInfo = this._nodeConfPathArr[i] ;
         hostName                 = hostNodeConfPathInfo[HostName] ;
         var confPathArr          = hostNodeConfPathInfo[Path] ;
         var ssh                  = this._getHostAdminSshObj( hostName ) ;
         var installPath          = this._getHostInstallPath( hostName ) ;
         for ( var j = 0; j < confPathArr.length; j++ ) {
            var flushedConfigObj = new FlushedConfig() ;
            confFile             = adaptPath( confPathArr[j] ) + "sdb.conf" ;
            flushedConfigObj.hostName     = hostName ;
            flushedConfigObj.adminSshObj  = ssh ;
            flushedConfigObj.installPath  = installPath ;
            flushedConfigObj.confFile     = confFile ;
            var cmd = " Oma.getOmaConfigs(\"" + confFile + "\"); " ;   
            var originalStr = 
               this.execCommand( ssh, installPath, cmd, true ) ;
            original = removeBreak( originalStr ) ;
            flushedConfigObj.originalStr = original ;
            // build the complete config option
            var configOptionStr =
               this._buildConfigOptionStr( original, 
                  JSON.stringify(this._configInfoObj), omAddr ) ;
            // keep the original config info for rollback, if necessary
            this._flushedConfigArr.push( flushedConfigObj ) ;
            // going to flush config
            this._flush( ssh, installPath, confFile, configOptionStr ) ;
         }
      }
   } catch( e ) {
      exp = new SdbError( e, 
         sprintf( "failed to flush the config option into the " + 
            "node's config file[?] in host[?]", confFile, hostName ) ) ;
      logger.log( PDERROR, exp ) ;
      // record the original info to the log, should use level "PDEVENT"
      logger.log( PDEVENT, 
         sprintf( "the original info of config file[?] " + 
            "in host[?] is: [?]", confFile, hostName, original ) ) ;
      // rollback
      this._rollback() ;
      throw exp ;
   }
   logger.log( PDEVENT, "succeed to flush the config option info all " + 
      "the config file in current cluster" ) ;
} ;

var RestartNodes = function( configMgr ) {
   this._configMgr        = configMgr ;
   this._hostInfoArr      = null ;
} ;

RestartNodes.prototype = new Helper() ;

RestartNodes.prototype._init = function RestartNodes__init() {
   this._hostInfoArr    = this._configMgr._getHostInfoArr() ;
} ;

RestartNodes.prototype._getGroupInfo = 
   function RestartNodes__getGroupInfo() {
   var exp       = null ;
   var groupInfoArr = [] ;
   var rgInfoArr = this._configMgr._getRGInfoArr( true ) ;
   for ( var i = 0; i < rgInfoArr.length; i++ ) {
      var groupInfo = new GroupInfo() ;
      var rgInfoObj = rgInfoArr[i] ;
      var groupName = rgInfoObj[GroupName] ;
      var nodeArr   = rgInfoObj[Group] ;
      if ( !isArray(nodeArr) ) {
         exp = new SdbError( SDB_SYS, 
            sprintf( "the value of field [?] is not an array in group[?]",
               Group, groupName ) ) ;
         logger.log( PDERROR, exp ) ;
         throw exp ;
      }
      groupInfo.groupName = groupName ;
      for ( var j = 0; j < nodeArr.length; j++ ) {
         var isMatch        = false ;
         var nodeInfo       = new NodeInfo() ;
         var hostName       = nodeArr[j][HostName] ;
         var svcArr         = nodeArr[j][Service] ;
         nodeInfo[HostName] = hostName ;
         for ( var m = 0; m < svcArr.length; m++ ) {
            var type = svcArr[m][Type] ;
            if ( type == 0 ) {
               isMatch = true ;
               nodeInfo[Service] = svcArr[m][Name] ;
               break ;               
            }
         }
         if ( !isMatch ) {
            exp = new SdbError( SDB_SYS, 
               sprintf( "no service with type[?] in group[?] in " + 
                  "host[?]", 0, groupName, hostName ) ) ;
            logger.log( PDERROR, exp ) ;
            throw exp ;
         }
         // keep the node's info into array
         groupInfo.nodeArr.push( nodeInfo ) ;
      }
      // keep the group's info into array
      groupInfoArr.push( groupInfo ) ;
   }
   return groupInfoArr ;
} ;

RestartNodes.prototype._getInstalledPath = 
   function RestartNodes__getInstalledPath( hostName ) {
   var retStr      = null ;
   var hostInfoArr = this._configMgr._getHostInfoArr() ;
   for ( var i = 0; i < hostInfoArr.length; i++ ) {
      if ( hostName == hostInfoArr[i].hostName ||
           hostName == hostInfoArr[i].ip ) {
         retStr = hostInfoArr[i].installPath ;
         break ;
      }
   }
   return retStr ;
} ;

RestartNodes.prototype._getAdminSshObj = 
   function RestartNodes__getAdminSshObj( hostName ) {
   var retObj      = null ;
   var hostInfoArr = this._configMgr._getHostInfoArr() ;
   for ( var i = 0; i < hostInfoArr.length; i++ ) {
      if ( hostName == hostInfoArr[i].hostName ||
           hostName == hostInfoArr[i].ip ) {
         retObj = hostInfoArr[i].adminSshObj ;
         break ;
      }
   }
   return retObj ;
} ;

RestartNodes.prototype._restart = 
   function RestartNodes__restart( adminSshObj, installPath, svcName ) {
   var cmd1 = adaptPath( installPath ) + "bin/sdbstop -p " + svcName ;
   var cmd2 = adaptPath( installPath ) + "bin/sdbstart -p " + svcName ;
   try {
      adminSshObj.exec( cmd1 ) ;
      sleep( 3000 ) ;
      adminSshObj.exec( cmd2 ) ;
      sleep( 3000 ) ;
   } catch( e ) {
      var exp = new SdbError( e ) ;
      logger.log( PDERROR, exp ) ;
      throw exp ;
   }
}

RestartNodes.prototype._doit = 
   function RestartNodes__doit() {
   var exp       = null ;
   this._init() ;
   // begin to restart nodes
   var rgInfoArr = this._getGroupInfo() ;
   for ( var i = 0; i < rgInfoArr.length; i++ ) {
      var rgInfo  = rgInfoArr[i] ;
      var rgName  = rgInfo.groupName ;
      var nodeArr = rgInfo.nodeArr ;
      logger.log( PDEVENT, 
         sprintf( "begin to restart nodes in group[?]", rgName ) );
      for ( var j = 0; j < nodeArr.length; j++ ) {
         var hostName    = nodeArr[j][HostName] ;
         var svcName     = nodeArr[j][Service] ;
         var installPath = this._getInstalledPath( hostName ) ;
         if ( installPath == null ) {
            exp = new SdbError( SDB_SYS, 
               sprintf( "failed to get install path for restart nodes in " + 
                  "host[?]", hostName ) ) ;
            logger.log( PDERROR, exp ) ;
            throw exp ;
         }
         var adminSshObj = this._getAdminSshObj( hostName ) ;
         if ( adminSshObj == null ) {
            exp = new SdbError( SDB_SYS, 
               sprintf( "failed to admin ssh object for restart nodes in " + 
                  "host[?]", hostName ) ) ;
            logger.log( PDERROR, exp ) ;
            throw exp ;
         }
         logger.log( PDEVENT, 
            sprintf( "[?]restarting node[?:?]", 
               rgName, hostName, svcName ) ) ;
         try {
            this._restart( adminSshObj, installPath, svcName ) ;
         } catch( e ) {
            exp = new SdbError( e, 
               sprintf( "failed to restart node[?:?] in host[?]", 
                  hostName, svcName, hostName ) ) ;
            logger.log( PDERROR, exp ) ;
            throw exp ;
         }
         logger.log( PDEVENT, 
            sprintf( "[?]succeed to restart node[?:?]", 
               rgName, hostName, svcName ) ) ;
      }
      logger.log( PDEVENT, 
         sprintf( "succeed to restart all the nodes in group[?]", rgName ) );
   }
}

var AddBusiness = function( configMgr, hostInfoArr, configInfoArr ) {
   this._configMgr     = configMgr ;
   this._hostInfoArr   = hostInfoArr ;
   this._configInfoArr = configInfoArr ;
   this._clusterExist  = false ;
   this._omAddr        = null ;
   this._omObj         = null ;
} ;

AddBusiness.prototype = new Helper() ;

AddBusiness.prototype._init = function AddBusiness__init() {
   var exp = null ;
   // om exist or not
   if ( !this.omExist() ) {
      exp = new SdbError( SDBCM_NODE_NOTEXISTED, "om does not exist in local host" ) ;
      logger.log( PDERROR, exp ) ;
      throw exp ;
   }
   // get om object
   var omAuthInfo = this._configMgr._getOMAuthInfo() ;
   var omSvc = this.getOMSvc() ;
   try {
      this._omObj = new Sdb( "127.0.0.1", omSvc, 
                             omAuthInfo[AuthUser], omAuthInfo[AuthPasswd] ) ;
   } catch( e ) {
      exp =  new SdbError( e, "failed to connect to om in local host" ) ;
      logger.log( PDERROR, exp ) ;
      throw exp ;
   }
   // get om address
   this._omAddr = this.getOMAddr() ;
} ;

AddBusiness.prototype._checkClusterInfo = function AddBusiness__checkClusterInfo() {
   var exp = null ;
   var csName = "SYSDEPLOY" ;
   var clName = "SYSCLUSTER" ;
   
   // cluster info from config file
   var clusterInfoObj = this._configMgr._getClusterInfo() ;
   var clusterName = clusterInfoObj[ClusterName] ;
   // cluster info from om
   try {
      var cur = this._omObj.getCS( csName ).getCL( clName ).find() ;
      var record = null ;
      var recordObj = null ;
      while( ( record = cur.next() ) != null ) {
         recordObj = record.toObj() ;
         if ( clusterName == recordObj[ClusterName] ) {
            if ( clusterInfoObj[SdbUser] != recordObj[SdbUser] ||
                 clusterInfoObj[SdbPasswd] != recordObj[SdbPasswd] ||
                 clusterInfoObj[SdbUserGroup] != recordObj[SdbUserGroup] ||
                 adaptPath(clusterInfoObj[InstallPath]) != 
                    adaptPath(recordObj[InstallPath]) ) {
               exp = new SdbError( SDB_INVALIDARG, 
                  sprintf( "cluster[?] has existed in om, it's info is[?], " + 
                     "but the cluster info which got from config file is[?], " + 
                     "it's conflicted with the existed cluster info", clusterName, 
                     clusterInfoObj.toString(), record.toJson() ) ) ;
               logger.log( PDERROR, exp ) ;
               throw exp ;               
            } else {
               this._clusterExist = true ;
            }
            break ;
         }
      }
   } catch( e ) {
      exp =  new SdbError( e, "failed to check the info of cluster" ) ;
      logger.log( PDERROR, exp ) ;
      throw exp ;
   }
   
} ;

AddBusiness.prototype._checkModuleInfo = function AddBusiness__checkModuleInfo() {
   var exp = null ;
   var csName = "SYSDEPLOY" ;
   var clName = "SYSBUSINESS" ;
   var cur = null ;
   
   var moduleInfoObj = this._configMgr._getModuleInfo() ;
   var clusterName = moduleInfoObj[ClusterName] ;
   var businessName = moduleInfoObj[BusinessName] ;

   try {
      cur = this._omObj.getCS( csName ).getCL( clName ).find() ;
      var record = null ;
      while( (record = cur.next()) != null ) {
         recordObj = record.toObj() ;
         if ( moduleInfoObj[BusinessName] == recordObj[BusinessName] ) {
            exp = new SdbError( SDB_INVALIDARG, 
               sprintf( "business name[?] has been used in cluster[?]", 
                  recordObj[BusinessName], recordObj[ClusterName] ) ) ;
            logger.log( PDERROR, exp ) ;
            throw exp ;
         }
      }
   } catch( e ) {
      exp = new SdbError( e, "failed to check the info of business" ) ;
      logger.log( PDERROR, exp ) ;
      throw exp ;
   }
} ;

AddBusiness.prototype._checkHostInfo = function AddBusiness__checkHostInfo() {
   var exp = null ;
   var csName = "SYSDEPLOY" ;
   var clName = "SYSHOST" ;
   var cur = null ;
   var errMsgArr = [] ;
   
   var hostInfoArr = this._configMgr._getHostInfoArr() ;
   var clusterInfoObj = this._configMgr._getClusterInfo() ;
   var currentClusterName = clusterInfoObj[ClusterName] ;
   
   try {
      cur = this._omObj.getCS( csName ).getCL( clName ).find() ;
      var record = null ;
      while( (record = cur.next()) != null ) {
         recordObj = record.toObj() ;
         var hostName = recordObj[HostName] ;
         var clusterName = recordObj[ClusterName] ;
         for( var i = 0; i < hostInfoArr.length; i++ ) {
            var host = hostInfoArr[i] ;
            var name = host.hostName ;
            if ( hostName == name && currentClusterName != clusterName ) {
               errMsgArr.push( sprintf( "host[?] has already been using in " +
                                  "cluster[?], it can't be add to cluster[?]", 
                                  hostName, clusterName, currentClusterName ) ) ;
            }
         }
      }
   } catch( e ) {
      exp = new SdbError( e, 
         "failed to check whether hosts are conflicting in each cluster or not" ) ;
      logger.log( PDERROR, exp ) ;
      throw exp ;
   }
   if ( errMsgArr.length != 0 ) {
      exp = new SdbError( SDB_INVALIDARG, errMsgArr.toString() ) ;
      logger.log( PDERROR, exp ) ;
      throw exp ;
   }
} ;

AddBusiness.prototype._check = function AddBusiness__check() {
   
   // cluster exist or not
   // and if so, whether the info is the same the current or not
   this._checkClusterInfo() ;
   // business exist or not
   this._checkModuleInfo() ;
   // host exist or not
   this._checkHostInfo() ;
} ;

AddBusiness.prototype._insertClusterInfo = 
   function AddBusiness__insertClusterInfo() {
   var csName = "SYSDEPLOY" ;
   var clName = "SYSCLUSTER" ;
   if ( this._clusterExist ) {
      return ;
   }
   try {
      var clusterInfoObj = this._configMgr._getClusterInfo() ;
      var cs = this._omObj.getCS( csName ) ;
      var cl = cs.getCL( clName ) ;
      cl.insert( clusterInfoObj ) ;
   } catch( e ) {
      var exp = new SdbError( e, 
         sprintf( "failed to insert record[?] into collection[?]",
            JSON.stringify(clusterInfoObj), cl ) ) ;
      logger.log( PDERROR, exp ) ;
      throw exp ;
   }
} ;

AddBusiness.prototype._insertModuleInfo = 
   function AddBusiness__insertModuleInfo() {
   var csName = "SYSDEPLOY" ;
   var clName = "SYSBUSINESS" ;
   try {
      var moduleInfoObj = this._configMgr._getModuleInfo() ;
      var cs = this._omObj.getCS( csName ) ;
      var cl = cs.getCL( clName ) ;
      cl.insert( moduleInfoObj ) ;
   } catch( e ) {
      var exp = new SdbError( e, 
         sprintf( "failed to insert record[?] into collection[?]",
            JSON.stringify(moduleInfoObj), cl ) ) ;
      logger.log( PDERROR, exp ) ;
      throw exp ;
   }   
} ;

AddBusiness.prototype._insertHostInfo = 
   function AddBusiness__insertHostInfo() {
   var csName = "SYSDEPLOY" ;
   var clName = "SYSHOST" ;
   try {
      var cs = this._omObj.getCS( csName ) ;
      var cl = cs.getCL( clName ) ;
      cl.insert( this._hostInfoArr ) ;
   } catch( e ) {
      var exp = new SdbError( e, 
         sprintf( "failed to insert record[?] into collection[?]",
            JSON.stringify(ths._hostInfoArr), cl ) ) ;
      logger.log( PDERROR, exp ) ;
      throw exp ;
   } 
} ;

AddBusiness.prototype._insertConfigInfo = 
   function AddBusiness__insertConfigInfo() {
   var csName = "SYSDEPLOY" ;
   var clName = "SYSCONFIGURE" ;
   
   // insert info
   try {
      var cs = this._omObj.getCS( csName ) ;
      var cl = cs.getCL( clName ) ;
      cl.insert( this._configInfoArr ) ;
   } catch( e ) {
      var exp = new SdbError( e, 
         sprintf( "failed to insert record[?] into collection[?]",
            JSON.stringify( this._configInfoArr ), cl ) ) ;
      logger.log( PDERROR, exp ) ;
      throw exp ;
   } 
} ;

AddBusiness.prototype._doit = function AddBusiness__doit() {
   var exp = null ;
   this._init() ;
   this._check() ;
   try {
      this._insertClusterInfo() ;
      this._insertModuleInfo() ;
      this._insertHostInfo() ;
      this._insertConfigInfo() ;
   } catch( e ) {
      exp = new SdbError( e, "failed to insert info into om" ) ;
      logger.log( PDERROR, exp ) ;
      throw exp ;
   }
   logger.log( PDEVENT, "succeed to add business" ) ; 
} ;

var InstallOM = function( configMgr ) {
   this._configMgr     = configMgr ;
} ;

InstallOM.prototype = new Helper() ;

InstallOM.prototype._init = function InstallOM__init() {
} ;

InstallOM.prototype._copyInstallPacket = 
   function InstallOM__copyInstallPacket() {
   var source = null ;
   var target = null ;
   try {
      source          = this._configMgr._getDBInstallPacket() ;
      var installInfo = getInstallInfoObj() ;
      var packetName  = getPacketName( source ) ; 
      target          = adaptPath( installInfo[INSTALL_DIR] ) + 
                        "packet/" + packetName ;
      File.copy( source, target ) ;
   } catch( e ) {
      var exp = new SdbError( e, 
         sprintf( "failed to copy SequoiaDB install packet[?] to [?]", 
            source, target ) ) ;
      logger.log( PDERROR, exp ) ;
      throw exp ;
   }
} ;

InstallOM.prototype._installOM = 
   function InstallOM__installOM() {
   var exp       = null ;
   var agentPort = null ;
   // get om agent's service
   try {
      agentPort = this.getAgentPort() ;
   } catch( e ) {
      exp = new SdbError( e, "failed to get local sdbcm's service" ) ;
      logger.log( PDERROR, exp ) ;
      throw exp ;      
   }
   // connect to om agent
   var oma = null ;
   try {
      oma = new Oma( "127.0.0.1", agentPort ) ;
   } catch( e ) {
      exp = new SdbError( e, 
         sprintf( "failed to connect to local sdbcm[?;?]", 
            "127.0.0.1", agentPort ) ) ;
      logger.log( PDERROR, exp ) ;
      throw exp ;
   }
   // create om svc
   var om     = null ;
   var dbPath = null ;
   try {
      var dbPath = adaptPath( System.getEWD() ) + "../database/sms/11780" ;
      oma.createOM( 11780, dbPath, {httpname: 8000} ) ;
   } catch( e ) {
      exp = new SdbError( e, "failed to install om" ) ;
      logger.log( PDERROR, exp ) ;
      if ( exp.getErrCode() != SDBCM_NODE_EXISTED ) {
         try { oma.removeOM( 11780 ) ; } catch( e ) {}
      }
      throw exp ;
   }
   // start om
   try {
      oma.startNode( 11780 ) ;
   } catch( e ) {
      exp = new SdbError( e, "failed to start om" ) ;
      logger.log( PDERROR, exp ) ;
      try { oma.removeOM( 11780 ) ; } catch( e ) {}
      try { oma.close() ; } catch( e ) {}
      throw exp ;
   }
} ;

InstallOM.prototype._doit = function InstallOM__doit() {
   this._init() ;
   if ( !this.omExist() ) {
	   this._installOM() ;
      this._copyInstallPacket() ;
   } else {
       var exp = new SdbError( SDBCM_NODE_EXISTED, "om has already existed in localhost" ) ;
	   logger.log( PDERROR, exp ) ;
	   throw exp ;
   }
} ;

var RemoveOM = function() {
} ;

RemoveOM.prototype = new Helper() ;

RemoveOM.prototype._init = function RemoveOM__init() {
} ;

RemoveOM.prototype._doit = function RemoveOM__doit() {
   try {
      var agentPort = this.getAgentPort() ;
      var omPort    = this.getOMSvc() ;
      var oma       = new Oma( "127.0.0.1", agentPort ) ;
      oma.removeOM( omPort ) ;
      logger.log( PDEVENT, "succeed to remove om in local host" ) ;
   } catch( e ) {
      var exp = new SdbError( e, "failed to remove om in local host" ) ;
      logger.log( PDERROR, exp ) ;
      throw exp ;
   }
} ;

function main() {
   var errMsg    = null ;
   var exp       = null ;

   // 1. extract config info from config file and 
   // check whether the offered info is ok
   var configMgr = new ConfigMgr( OM_CONF_FILE, ACTION ) ;
   configMgr._doit() ;

   // 2. try to build/remove om
   if ( configMgr._getAction() == ACTION_BUILD_OM ) {
	   var installOMTask = new InstallOM( configMgr ) ;
           logger.log( PDEVENT, "excuting task: install om" ) ;
	   installOMTask._doit() ;
	   return 0;
   }
   if ( configMgr._getAction() == ACTION_REMOVE_OM ) {
      var removeOMTask = new RemoveOM() ;
      logger.log( PDEVENT, "excuting task: remove om" ) ;
      removeOMTask._doit() ;
      return 0 ;
   }
   
   try {
      // 3. try to update coord info
      if ( configMgr._getAction() == ACTION_ADD_BUSINESS || 
           configMgr._getAction() == ACTION_UPDATE_COORD ||
           configMgr._getAction() == ACTION_FLUSH_CONFIG ) {
         var updateCoordTask = new UpdateCoord( configMgr ) ;
         logger.log( PDEVENT, "excuting task: update coord" ) ;
         updateCoordTask._doit() ;
      }
      // if we just want to update coord info, let's return
      if ( configMgr._getAction() == ACTION_UPDATE_COORD ) {
         // stop running
         return 0 ;
      }
      
      // 4. collect the config info of the nodes in current cluster
      // those info will be saved in collection "SYSDEPLOY.SYSCONFIGURE" in OM
      var collectNodeInfoTask = new CollectNodeInfo( configMgr ) ;
      logger.log( PDEVENT, "excuting task: collect node's info" ) ;
      collectNodeInfoTask._doit() ;

      // 5. get the info of host in current cluster
      // those info will be saved in collection "SYSDEPLOY.SYSHOST" in OM
      var nodeDataPathArr = collectNodeInfoTask._getNodeDataPaths() ;
      var checkHostTask   = new CheckHost( configMgr, nodeDataPathArr ) ;
      logger.log( PDEVENT, "excuting task: collect host's info" ) ;
      checkHostTask._doit() ;

      // 6. add business
      if ( configMgr._getAction() == ACTION_ADD_BUSINESS ) {
         try {
            var checkHostInfoArr = checkHostTask._getCheckHostResult() ;
            var nodeConfInfoArr  = collectNodeInfoTask._getNodeConfResult() ;
            var addBusinessTask  = new AddBusiness( configMgr, 
                                                    checkHostInfoArr, 
                                                    nodeConfInfoArr ) ;
            logger.log( PDEVENT, "excuting task: add business" ) ;
            addBusinessTask._doit() ;
         } catch( e ) {
            errMsg = "failed to add business to local om" ;
            exp = new SdbError( e, errMsg ) ;
            logger.log( PDERROR, exp ) ;
            if ( exp.getErrCode() == SDBCM_NODE_NOTEXISTED ) {
               errMsg = sprintf( "Error: failed add business to local om, " + 
                  "for om does not exist in local host. " + 
                  "Please use command[?] to install om, and then try " + 
                  "to add business again.", 
                  "./ombuild.sh --action buildom --conf ombuild.conf"  ) ;
               println( errMsg ) ;
               // stop running
               return -1 ;
            } else {
               throw exp ;
            }
         }
      }
   } finally {
      configMgr._final() ;
   }
   return 0 ;
}

// execute
try {
   var exp     = null ;
   var errCode = null ;
   _init() ;
   var rc = main() ;
   if ( SDB_OK == rc ) {
      println( "succeed to run all the tasks" ) ;
   }
} catch( e ) {
   exp     = new SdbError( e, "failed to run tasks" ) ; 
   errCode = exp.getErrCode() ;
   logger.log( PDERROR, exp ) ;
   throw errCode ;
} finally {
   if ( exp == null ) {
      _final() ;
   }
}

