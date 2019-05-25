// File open option define
const SDB_FILE_CREATEONLY =    0x00000001 ;
const SDB_FILE_REPLACE =       0x00000002 ;
const SDB_FILE_CREATE =        SDB_FILE_CREATEONLY | SDB_FILE_REPLACE ;
const SDB_FILE_SHAREREAD =     0x00000010 ;
const SDB_FILE_SHAREWROTE =    SDB_FILE_SHAREREAD | 0x00000020 ;
const SDB_FILE_READONLY =      0x00000004 | SDB_FILE_SHAREREAD ;
const SDB_FILE_WRITEONLY =     0x00000008 ;
const SDB_FILE_READWRITE =     0x00000004 | SDB_FILE_WRITEONLY ;

// BSONObj
BSONObj.prototype.toObj = function() {
   return JSON.parse( this.toJson() ) ;
}

BSONObj.prototype.toString = function() {
   try
   {
      var obj = this.toObj() ;
      var str = JSON.stringify( obj, undefined, 2 ) ;
      return str ;
   }
   catch( e )
   {
      return this.toJson() ;
   }
}
// end BSONObj

// BSONArray
BSONArray.prototype.toArray = function() {
   if ( this._arr )
      return this._arr;

   var a = [];
   while ( true ) {
      var bs = this.next();
      if ( ! bs ) break ;
      var json = bs.toJson () ;
      try
      {
         var stf = JSON.stringify(JSON.parse(json), undefined, 2) ;
         a.push ( stf ) ;
      }
      catch ( e )
      {
         a.push ( json ) ;
      }
   }
   this._arr = a ;
   return this._arr ;
}

BSONArray.prototype.arrayAccess = function( idx ) {
   return this.toArray()[idx] ;
}

BSONArray.prototype.toString = function() {
   //return this.toArray().join('\n') ;
   var array = this ;
   var record = undefined ;
   var returnRecordNum = 0 ;
   while ( ( record = array.next() ) != undefined )
   {
      returnRecordNum++ ;
      try
      {
         println ( record ) ;
      }
      catch ( e )
      {
         var json = record.toJson () ;
         println ( json ) ;
      }
   }
   println("Return "+returnRecordNum+" row(s).") ;
   return "" ;
}

BSONArray.prototype._formatStr = function() {

   var bsonObj = this.toArray() ;
   var objArr = new Array() ;
   var eleArr = new Array() ;
   var maxSizeArr = new Array() ;
   var outStr = "" ;
   var colNameArr = new Array() ;
   var objNum ;

   for ( var i in bsonObj )
   {
      objArr.push( JSON.parse( bsonObj[i] ) ) ;
   }

   var objNum = objArr.length ;

   if ( objNum > 0 )
   {
      // get row name and init maxSizeArr
      for( var index in objArr )
      {
         for ( var eleKey in objArr[ index ] )
         {
            if ( -1 == colNameArr.indexOf( eleKey ) )
            {
               colNameArr.push( eleKey ) ;
               maxSizeArr[ eleKey ] = eleKey.length ;
            }
         }
      }

      for ( var index in objArr )
      {
         var localArr = new Array() ;
         for( var ele in objArr[ index ] )
         {
            var localEle = objArr[ index ][ ele ].toString() ;
            localArr[ ele ] = localEle ;
            if ( maxSizeArr[ ele ] < localEle.length )
            {
               maxSizeArr[ ele ] = localEle.length ;
            }
         }
         eleArr.push( localArr ) ;
      }

      for( var index in maxSizeArr )
      {
         maxSizeArr[ index ] = maxSizeArr[ index ] + 1 ;
      }
      for ( var index in colNameArr )
      {
         var localRowName = colNameArr[ index ] ;
         outStr += " " + localRowName ;
         for ( var k = 0; k < maxSizeArr[ localRowName ] - localRowName.length ;
               k++ )
         {
            outStr += " " ;
         }
      }
      outStr += "\n" ;

      for ( var index in eleArr )
      {
         var arr = eleArr[ index ] ;
         for ( var ele in colNameArr )
         {
            var localRowName = colNameArr[ ele ] ;
            var localEle = arr[ localRowName ] ;
            if ( undefined == localEle )
            {
               localEle = "" ;
            }
            outStr += " " + localEle ;
            for ( var k = 0; k < maxSizeArr[ localRowName ] - localEle.length;
                  k++ )
            {
               outStr += " " ;
            }
         }
         outStr += "\n" ;
      }
   }
   return outStr ;
}
// end BSONArray

// Oma member function
Oma.prototype.getOmaInstallFile = function() {
   return this._runCommand( "oma get oma install file" ).toObj().installFile ;
}

Oma.prototype.getOmaInstallInfo = function() {
   return this._runCommand( "oma get oma install info" ) ;
}

Oma.prototype.getOmaConfigFile = function() {
   return this._runCommand( "oma get oma config file" ).toObj().confFile ;
}

Oma.prototype.getOmaConfigs = function( confFile ) {
   var recvObj ;

   // run command
   if ( undefined != confFile )
   {
      recvObj = this._runCommand( "oma get oma configs", {},
                                  { "confFile": confFile } ) ;
   }
   else
   {
      recvObj = this._runCommand( "oma get oma configs" ) ;
   }
   return recvObj ;
}

Oma.prototype.setOmaConfigs = function( configsObj, confFile, isReload ) {

   // check argument
   if ( undefined == configsObj )
   {
      setLastErrMsg( "obj must be config" ) ;
      throw SDB_OUT_OF_BOUND ;
   }

   // run command
   if ( undefined != confFile && "" != confFile )
   {
      this._runCommand( "oma set oma configs", {}, { "confFile": confFile },
                        { "configsObj": configsObj } ) ;
   }
   else
   {
      this._runCommand( "oma set oma configs", {}, {},
                        { "configsObj": configsObj } ) ;
   }

   if ( true == isReload )
   {
      this.reloadConfigs() ;
   }
}

Oma.prototype.getAOmaSvcName = function( hostname, confFile ) {
   // check argument
   if ( undefined == hostname )
   {
      setLastErrMsg( "hostname must be config" ) ;
      throw SDB_OUT_OF_BOUND ;
   }
   var recvObj ;

   // run command
   if ( undefined != confFile )
   {
      recvObj = this._runCommand( "oma get a oma svc name", {},
                                  { "confFile": confFile,
                                    "hostname": hostname } ) ;
   }
   else
   {
      recvObj = this._runCommand( "oma get a oma svc name", {},
                                  { "hostname": hostname } ) ;
   }
   return recvObj.toObj().svcName ;
}

Oma.prototype.addAOmaSvcName = function( hostname, svcname,
                                         isReplace, confFile ) {
   // check argument
   if ( undefined == hostname  )
   {
      setLastErrMsg( "hostname must be config" ) ;
      throw SDB_OUT_OF_BOUND ;
   }
   else if ( undefined == svcname )
   {
      setLastErrMsg( "svcname must be config" ) ;
      throw SDB_OUT_OF_BOUND ;
   }

   if ( undefined == isReplace )
   {
      isReplace = true ;
   }

   // run command
   if ( undefined != confFile )
   {
      this._runCommand( "oma add a oma svc name", { "isReplace": isReplace },
                        { "confFile": confFile },
                        { "hostname": hostname, "svcname": svcname } ) ;
   }
   else
   {
      this._runCommand( "oma add a oma svc name", { "isReplace": isReplace },
                        {}, { "hostname": hostname, "svcname": svcname } ) ;
   }
}

Oma.prototype.delAOmaSvcName = function( hostname, confFile ) {

   // check argument
   if ( undefined == hostname  )
   {
      setLastErrMsg( "hostname must be config" ) ;
      throw SDB_OUT_OF_BOUND ;
   }

   // run command
   if ( undefined != confFile )
   {
      this._runCommand( "oma del a oma svc name", {},
                        { "hostname": hostname, "confFile": confFile } ) ;
   }
   else
   {
      this._runCommand( "oma del a oma svc name", {},
                        { "hostname": hostname } ) ;
   }
}

Oma.prototype.listNodes = function( optionObj, filterObj ) {
   var displayMode ;

   // check argument
   if ( undefined != optionObj )
   {
      if ( "object" != typeof( optionObj ) )
      {
         setLastErrMsg( "optionObj must be object" ) ;
         throw SDB_INVALIDARG ;
      }

      if ( undefined != optionObj.displayMode )
      {
         displayMode = optionObj.displayMode ;
         delete optionObj.displayMode ;
      }
   }
   else
   {
      optionObj = {} ;
   }

   // run command
   var recvObj = this._runCommand( "oma list nodes", optionObj ) ;
   var retArray ;

   // filter
   if ( undefined != filterObj )
   {
      var filter = new _Filter( filterObj ) ;
      retArray = filter.match( recvObj.toObj() ) ;
   }
   else
   {
      var filter = new _Filter( {} ) ;
      retArray = filter.match( recvObj.toObj() ) ;
   }

   // set display format
   if ( "text" == displayMode )
   {
      retArray = retArray._formatStr() ;
   }
   return retArray ;
}

Oma.prototype.getNodeConfigs = function( svcname ) {
   // check svcname
   if ( undefined == svcname )
   {
      setLastErrMsg( "svcname must be config" ) ;
      throw SDB_OUT_OF_BOUND ;
   }

   // run command
   return this._runCommand( "oma get node configs",
                            {}, { "svcname": svcname } ) ;
}

Oma.prototype.setNodeConfigs = function( svcname, configsObj ) {
   // check argument
   if ( undefined == svcname )
   {
      setLastErrMsg( "svcname must be config" ) ;
      throw SDB_OUT_OF_BOUND ;
   }
   if ( undefined == configsObj )
   {
      setLastErrMsg( "configsObj must be config" ) ;
      throw SDB_OUT_OF_BOUND ;
   }

   // run command
   this._runCommand( "oma set node configs", {},
                     { "svcname": svcname }, { "configsObj": configsObj } ) ;
}

Oma.prototype.updateNodeConfigs = function( svcname, configsObj ) {
   // check argument
   if ( undefined == svcname )
   {
      setLastErrMsg( "svcname must be config" ) ;
      throw SDB_OUT_OF_BOUND ;
   }
   if ( undefined == configsObj )
   {
      setLastErrMsg( "configsObj must be config" ) ;
      throw SDB_OUT_OF_BOUND ;
   }

   // run command
   this._runCommand( "oma update node configs", {},
                     { "svcname": svcname }, { "configsObj": configsObj } ) ;
}

Oma.prototype.startAllNodes = function( businessName ) {
   var localNodes ;
   if ( 'string' == typeof( businessName ) )
   {
      localNodes = this.listNodes( { 'type': 'db',
                                     'expand': true,
                                     'mode': 'local' },
                                   { 'businessname': businessName } ).toArray() ;
   }
   else
   {
      localNodes = this.listNodes( { 'type': 'db', 'expand': true,
                                     'mode': 'local' } ).toArray() ;
   }

   var arrObj = [] ;
   var obj ;
   var isSuccess = true ;
   var total = localNodes.length ;
   var success = 0 ;


   for( var index in localNodes )
   {
      obj = JSON.parse( localNodes[ index ] ) ;
      try
      {
         print( "Start sequoiadb(" + obj.svcname + "): " ) ;
         this.startNode( obj.svcname ) ;
         println( "Success" ) ;
         success++ ;
      }
      catch( e )
      {
         println( "Failed, errno: " + e + ", description: " + getErr( e )
                  + ", detail: " + getLastErrMsg() ) ;
         if ( SDB_NETWORK == e || SDB_NETWORK_CLOSE == e )
         {
            println( "Total: " + total +
                     "; Success: " + success +
                     "; Failed: " + ( total - success ) ) ;
            throw e ;
         }
         isSuccess = false ;
      }
   }
   println( "Total: " + total + "; Success: " + success + "; Failed: " + ( total - success ) ) ;
   if ( false == isSuccess )
   {
      setLastErrMsg( "Failed to start all nodes" ) ;
      throw SDB_SYS ;
   }
}

Oma.prototype.stopAllNodes = function( businessName ) {
   var localNodes ;
   if ( 'string' == typeof( businessName ) )
   {
      localNodes = this.listNodes( { 'type': 'db',
                                     'expand': true,
                                     'mode': "run" },
                                   { 'businessname': businessName } ).toArray() ;
   }
   else
   {
      localNodes = this.listNodes( { 'type': 'db', 'expand': true,
                                     'mode': 'run' } ).toArray() ;
   }

   var arrObj = [] ;
   var obj ;
   var isSuccess = true ;
   var total = localNodes.length ;
   var success = 0 ;

   for( var index in localNodes )
   {
      obj = JSON.parse( localNodes[ index ] ) ;
      try
      {
         print( "Stop sequoiadb(" + obj.svcname + "): " ) ;
         this.stopNode( obj.svcname ) ;
         println( "Success" ) ;
         success++ ;
      }
      catch( e )
      {
         println( "Failed, errno: " + e + ", description: " + getErr( e )
                  + ", detail: " + getLastErrMsg() ) ;
         if ( SDB_NETWORK == e || SDB_NETWORK_CLOSE == e )
         {
            println( "Total: " + total +
                     "; Success: " + success +
                     "; Failed: " + ( total - success ) ) ;
            throw e ;
         }
         isSuccess = false ;
      }
   }

   println( "Total: " + total + "; Success: " + success + "; Failed: " + ( total - success ) ) ;
   if ( false == isSuccess )
   {
      setLastErrMsg( "Failed to start all nodes" ) ;
      throw SDB_SYS ;
   }
}

Oma.prototype.reloadConfigs = function()
{
   this._runCommand( "reload config" ) ;
}
// end Oma

// Remote member function
Remote.prototype.getSystem = function() {
   var system = System.getObj() ;
   system._remote = this ;
   return system ;
}

Remote.prototype.getFile = function( filename, permission, openMode ) {
   var file = File._getFileObj() ;
   var option = {} ;
   file._remote = this ;


   if ( 1 <= arguments.length )
   {
      if ( "string" != typeof( filename ) )
      {
         setLastErrMsg( "filename must be string" ) ;
         throw SDB_INVALIDARG ;
      }

      if( undefined != permission )
      {
         option.permission = permission ;
         if( undefined != openMode )
         {
            option.mode = openMode ;
         }
      }

      var retObj = this._runCommand( "file open", option, {},
                                     { "Filename": filename } ) ;
      file._FID = retObj.toObj().FID ;
      file._filename = filename ;
   }
   return file ;
}

Remote.prototype.getCmd = function() {
   var cmd = new Cmd() ;
   cmd._remote = this ;
   cmd._retCode = SDB_OK ;
   cmd._strOut = '' ;
   cmd._command = '' ;
   return cmd ;
}

Remote.prototype._runCommand = function( command, optionObj,
                                         matchObj, valueObj ) {
   var bsonObj ;
   var retObj ;
   var option = {} ;
   var match = {} ;
   var value = {} ;
   if ( 4 < arguments.length )
   {
      setLastErrMsg( "Too much arguments" ) ;
      throw SDB_INVALIDARG ;
   }
   if( undefined != optionObj )
   {
      option = optionObj ;
   }
   if( undefined != matchObj )
   {
      match = matchObj ;
   }
   if( undefined != valueObj )
   {
      value = valueObj ;
   }

   try
   {
      bsonObj = this.__runCommand( command, option, match, value ) ;
   }
   catch( e )
   {
      if( SDB_INVALIDARG == e || SDB_OUT_OF_BOUND == e )
      {
         var errMsg = getLastErrMsg() ;
         var errObj = getLastErrObj().toObj() ;
         var extraErrMsg = ": the cause of this error may be that the server version "
                           + "is not consistent with the client version" ;
         errObj.detail = errObj.detail + extraErrMsg ;
         setLastErrMsg( errMsg + extraErrMsg  ) ;
         setLastErrObj( errObj ) ;
      }
      throw e ;
   }

   return bsonObj ;
}
// end Remote

// _Filter member function
_Filter.prototype.match = function( BSONArrObj ) {

   if ( BSONArrObj instanceof Object )
   {
      return this._match( BSONArrObj ) ;
   }
   else
   {
      setLastErrMsg( "Argument must be objArray" ) ;
      throw SDB_INVALIDARG ;
   }
}
// end _Filter

// System static function
System.listProcess = function( optionObj, filterObj )
{
   var result ;
   var recvObj ;
   var displayMode = "obj" ;

   // check argument
   if ( undefined != optionObj )
   {
      if ( undefined != optionObj.displayMode )
      {
         displayMode = optionObj.displayMode ;
         delete optionObj.displayMode ;
      }
      recvObj = System._listProcess( optionObj ) ;
   }
   else
   {
      recvObj = System._listProcess() ;
   }

   // filter result
   if ( undefined != filterObj )
   {
      var filter = new _Filter( filterObj ) ;
      result = filter.match( recvObj.toObj() ) ;
   }
   else
   {
      var filter = new _Filter( {} ) ;
      result = filter.match( recvObj.toObj() ) ;
   }

   // set format
   if ( "text" == displayMode )
   {
      return result._formatStr() ;
   }
   else
   {
      return result ;
   }
}

System.isProcExist = function( optionObj ) {
   var retArray ;
   var isExist ;

   // check argument
   if ( undefined != optionObj )
   {
      if ( undefined == optionObj.value )
      {
         setLastErrMsg( "value must be config" ) ;
         throw SDB_OUT_OF_BOUND ;
      }
      else
      {
         // get specific process
         if ( optionObj.type == "name" )
         {
             retArray = System.listProcess( { "detail": true },
                                            { "cmd": optionObj.value } ) ;
         }
         else
         {
            retArray = System.listProcess( { "detail": true },
                                           { "pid": optionObj.value } ) ;
         }
      }
   }
   else
   {
      setLastErrMsg( "optionObj must be config" ) ;
      throw SDB_OUT_OF_BOUND ;
   }

   if ( 0 != retArray.size() )
   {
      isExist = true ;
   }
   else
   {
      isExist = false ;
   }
   return isExist ;
}

System.listLoginUsers = function( optionObj, filterObj ) {
   var objArray ;
   var retArray ;
   var displayMode = "obj" ;

   // check argument
   if ( undefined != optionObj )
   {
      if ( undefined != optionObj.displayMode )
      {
         displayMode = optionObj.displayMode ;
         delete optionObj.displayMode ;
      }
      objArray = System._listLoginUsers( optionObj ) ;
   }
   else
   {
      objArray = System._listLoginUsers() ;
   }

   // filter
   if ( undefined != filterObj )
   {
      var filter = new _Filter( filterObj ) ;
      retArray = filter.match( objArray.toObj() ) ;
   }
   else
   {
      var filter = new _Filter( {} ) ;
      retArray = filter.match( objArray.toObj() ) ;
   }

   // set display format
   if ( "text" == displayMode )
   {
      retArray = retArray._formatStr() ;
   }
   return retArray ;
}

System.listAllUsers = function( optionObj, filterObj ) {
   var objArray ;
   var retArray ;
   var displayMode = "obj" ;

   // check argument
   if ( undefined != optionObj )
   {
      if ( undefined != optionObj.displayMode )
      {
         displayMode = optionObj.displayMode ;
         delete optionObj.displayMode ;
      }
      objArray = System._listAllUsers( optionObj ) ;
   }
   else
   {
      objArray = System._listAllUsers() ;
   }

   // filter
   if ( undefined != filterObj )
   {
      var filter = new _Filter( filterObj ) ;
      retArray = filter.match( objArray.toObj() ) ;
   }
   else
   {
      var filter = new _Filter( {} ) ;
      retArray = filter.match( objArray.toObj() ) ;
   }

   // set display format
   if ( "text" == displayMode )
   {
      retArray = retArray._formatStr() ;
   }

   return retArray ;
}

System.listGroups = function( optionObj, filterObj ) {
   var objArray ;
   var retArray ;
   var displayMode = "obj" ;

   // check argument
   if ( undefined != optionObj )
   {
      if ( undefined != optionObj.displayMode )
      {
         displayMode = optionObj.displayMode ;
         delete optionObj.displayMode ;
      }
      objArray = System._listGroups( optionObj ) ;
   }
   else
   {
      objArray = System._listGroups() ;
   }

   // filter
   if ( undefined != filterObj )
   {
      var filter = new _Filter( filterObj ) ;
      retArray = filter.match( objArray.toObj() ) ;
   }
   else
   {
      var filter = new _Filter( {} ) ;
      retArray = filter.match( objArray.toObj() ) ;
   }

   // set display format
   if ( "text" == displayMode )
   {
      retArray = retArray._formatStr() ;
   }
   return retArray ;
}

System.isUserExist = function( userName ) {
   var isExist = false ;
   var retArray ;

   // check argument
   if ( undefined == userName )
   {
      setLastErrMsg( "userName must be config" ) ;
      throw SDB_OUT_OF_BOUND ;
   }

   // get specific result
   retArray = System.listAllUsers( { "detail": true }, { "user": userName } ) ;

   if ( 0 != retArray.size() )
   {
      isExist = true ;
   }
   return isExist ;
}

System.isGroupExist = function( groupName ) {
   var isExist = false ;
   var retArray ;

   // check argument
   if ( undefined == groupName )
   {
      setLastErrMsg( "userName must be config" ) ;
      throw SDB_OUT_OF_BOUND ;
   }

   // get specific result
   retArray = System.listGroups( { "detail": true }, { "name": groupName } ) ;

   if ( 0 != retArray.size() )
   {
      isExist = true ;
   }
   return isExist ;
}

// System member function
System.prototype.getInfo = function()
{
   var result ;
   var infoObj ;
   if ( undefined != this._remote )
   {
      var _remoteInfo = this._remote.getInfo() ;
      infoObj = _remoteInfo.toObj() ;
      infoObj.isRemote = true ;
   }
   else
   {
      infoObj = new Object() ;
      infoObj.isRemote = false ;
   }
   result = this._getInfo( infoObj ) ;
   return result ;
}

System.prototype.ping = function( hostname ) {
   var retObj ;

   if ( undefined != this._remote )
   {
      var retObj = this._remote._runCommand( "system ping", {},
                                             { "hostname" : hostname } ) ;
   }
   else
   {
      retObj = System.ping( hostname ) ;
   }
   return retObj ;

}

System.prototype.type = function() {
   var retStr ;

   // check argument
   if ( 0 < arguments.length )
   {
      setLastErrMsg( getErr( SDB_INVALIDARG ) ) ;
      throw SDB_INVALIDARG ;
   }

   if ( undefined != this._remote )
   {
      var retObj = this._remote._runCommand( "system type" ) ;
      retStr = retObj.toObj().type ;
   }
   else
   {
      retStr = System.type() ;
   }
   return retStr ;
}

System.prototype.getReleaseInfo = function() {
   var retObj ;

   // check argument
   if ( 0 < arguments.length )
   {
      setLastErrMsg( getErr( SDB_INVALIDARG ) ) ;
      throw SDB_INVALIDARG ;
   }

   if ( undefined != this._remote )
   {
      retObj = this._remote._runCommand( "system get release info" ) ;
   }
   else
   {
      retObj = System.getReleaseInfo() ;
   }
   return retObj ;
}

System.prototype.getHostsMap = function() {
   var retObj ;

   if ( 0 < arguments.length )
   {
      setLastErrMsg( "getHostsMap() should have non arguments" ) ;
      throw SDB_INVALIDARG ;
   }

   if ( undefined != this._remote )
   {
      retObj = this._remote._runCommand( "system get hosts map" ) ;
   }
   else
   {
      retObj = System.getHostsMap() ;
   }
   return retObj ;
}

System.prototype.getAHostMap = function( hostname ) {
   var retStr ;

   if ( undefined != this._remote )
   {
      var retObj = this._remote._runCommand( "system get a host map", {},
                                             { "hostname": hostname } ) ;
      retStr = retObj.toObj().ip ;
   }
   else
   {
      retStr = System.getAHostMap() ;
   }
   return retStr ;
}

System.prototype.addAHostMap = function( hostname, ip, isReplace ) {
   if ( undefined == isReplace )
   {
      isReplace = true ;
   }
   if ( undefined != this._remote )
   {
      this._remote._runCommand( "system add a host map", {}, {},
                                { "hostname": hostname,
                                  "ip": ip,
                                  "isReplace": isReplace } ) ;
   }
   else
   {
      System.addAHostMap( hostname, ip, isReplace ) ;
   }
}

System.prototype.delAHostMap = function( hostname ) {
   if ( undefined != this._remote )
   {
      this._remote._runCommand( "system delete a host map", {},
                                { "hostname": hostname } ) ;
   }
   else
   {
      System.delAHostMap( hostname ) ;
   }
}

System.prototype.getCpuInfo = function() {
   var retObj ;
   if ( undefined != this._remote )
   {
      var retObj = this._remote._runCommand( "system get cpu info" ) ;
   }
   else
   {
      retObj = System.getCpuInfo() ;
   }
   return retObj ;
}

System.prototype.snapshotCpuInfo = function() {
   var retObj ;
   if ( undefined != this._remote )
   {
      var retObj = this._remote._runCommand( "system snapshot cpu info" ) ;
   }
   else
   {
      retObj = System.snapshotCpuInfo() ;
   }
   return retObj ;
}

System.prototype.getMemInfo = function() {
   var retObj ;
   if ( undefined != this._remote )
   {
      var retObj = this._remote._runCommand( "system get mem info" ) ;
   }
   else
   {
      retObj = System.getMemInfo() ;
   }
   return retObj ;
}

System.prototype.snapshotMemInfo = function() {
   var retObj ;
   if ( undefined != this._remote )
   {
      var retObj = this._remote._runCommand( "system get mem info" ) ;
   }
   else
   {
      retObj = System.snapshotMemInfo() ;
   }
   return retObj ;
}

System.prototype.getDiskInfo = function() {
   var retObj ;
   if ( undefined != this._remote )
   {
      var retObj = this._remote._runCommand( "system get disk info" ) ;
   }
   else
   {
      retObj = System.getDiskInfo() ;
   }
   return retObj ;
}

System.prototype.snapshotDiskInfo = function() {
   var retObj ;
   if ( undefined != this._remote )
   {
      var retObj = this._remote._runCommand( "system get disk info" ) ;
   }
   else
   {
      retObj = System.snapshotDiskInfo() ;
   }
   return retObj ;
}

System.prototype.getNetcardInfo = function() {
   var retObj ;
   if ( undefined != this._remote )
   {
      var retObj = this._remote._runCommand( "system get netcard info" ) ;
   }
   else
   {
      retObj = System.getNetcardInfo() ;
   }
   return retObj ;
}

System.prototype.snapshotNetcardInfo = function() {
   var retObj ;
   if ( undefined != this._remote )
   {
      var retObj = this._remote._runCommand( "system snapshot netcard info" ) ;
   }
   else
   {
      retObj = System.snapshotNetcardInfo() ;
   }
   return retObj ;
}

System.prototype.getIpTablesInfo = function() {
   var retObj ;
   if ( undefined != this._remote )
   {
      var retObj = this._remote._runCommand( "system get ip tables info" ) ;
   }
   else
   {
      retObj = System.getIpTablesInfo() ;
   }
   return retObj ;
}

System.prototype.getHostName = function() {
   var result ;

   // check argument
   if ( 0 < arguments.length )
   {
      setLastErrMsg( getErr( SDB_INVALIDARG ) ) ;
      throw SDB_INVALIDARG ;
   }

   if ( undefined != this._remote )
   {
      var retObj = this._remote._runCommand( "system get hostname" ) ;
      result = retObj.toObj().hostname ;
   }
   else
   {
      result = System.getHostName() ;
   }
   return result ;
}

System.prototype.sniffPort = function( port ) {
   var retObj ;

   // check argument
   if ( undefined == port )
   {
      setLastErrMsg( "not specified the port to sniff" ) ;
      throw SDB_INVALIDARG ;
   }

   if ( undefined != this._remote )
   {
      var retObj = this._remote._runCommand( "system sniff port", {},
                                             { "port": port }) ;
   }
   else
   {
      retObj = System.sniffPort( port ) ;
   }
   return retObj ;
}

System.prototype.listProcess = function( optionObj, filterObj ) {
   var retObj ;
   var result ;

   if ( undefined != this._remote )
   {
      var displayMode = "obj" ;

      // check argument
      if ( undefined != optionObj )
      {
         if ( "object" != typeof( optionObj ) )
         {
            setLastErrMsg( "optionObj must be object" ) ;
            throw SDB_INVALIDARG ;
         }
         if ( undefined != optionObj.displayMode  )
         {
            displayMode = optionObj.displayMode ;
            delete optionObj.displayMode ;
         }
      }

      if ( undefined != optionObj )
      {
         result = this._remote._runCommand( "system list process", optionObj ) ;
      }
      else
      {
         result = this._remote._runCommand( "system list process" ) ;
      }

      if ( undefined != filterObj )
      {
         var filter = new _Filter( filterObj ) ;
         retObj = filter.match( result.toObj() ) ;
      }
      else
      {
         var filter = new _Filter( {} ) ;
         retObj = filter.match( result.toObj() ) ;
      }

      if ( "text" == displayMode )
      {
         return retObj._formatStr() ;
      }
      else
      {
         return retObj ;
      }
   }
   else
   {
      retObj = System.listProcess( optionObj, filterObj ) ;
      return retObj ;
   }
}

System.prototype.isProcExist = function( optionObj ) {

   var retArray ;
   var isExist = false ;

   // check argument
   if ( undefined != optionObj )
   {
      if ( undefined == optionObj.value )
      {
         setLastErrMsg( "value must be config" ) ;
         throw SDB_OUT_OF_BOUND ;
      }
      else
      {
         if ( "name" == optionObj.type )
         {
             retArray = this.listProcess( { "detail": true },
                                         { "cmd": optionObj.value } ) ;
           }
         else
         {
            retArray = this.listProcess( { "detail": true },
                                         { "pid": optionObj.value } ) ;
         }
      }

     }
   else
   {
      setLastErrMsg( "optionObj must be config" ) ;
      throw SDB_OUT_OF_BOUND ;
   }

   if ( 0 != retArray.size() )
   {
      isExist = true ;
   }
   return isExist ;
}

System.prototype.addUser = function( userObj ) {
   if ( undefined != this._remote )
   {
      this._remote._runCommand( "system add user", {}, {}, userObj ) ;
   }
   else
   {
      System.addUser( userObj ) ;
   }
}

System.prototype.setUserConfigs = function( optionObj ) {
   if ( undefined != this._remote )
   {
      this._remote._runCommand( "system set user configs", {}, {}, optionObj ) ;
   }
   else
   {
      System.setUserConfigs( optionObj ) ;
   }
}

System.prototype.delUser = function( optionObj ) {
   if ( undefined != this._remote )
   {
      this._remote._runCommand( "system del user", {}, optionObj ) ;
   }
   else
   {
      System.delUser( optionObj ) ;
   }
}

System.prototype.addGroup = function( optionObj ) {
   if ( undefined != this._remote )
   {
      this._remote._runCommand( "system add group", {}, {}, optionObj ) ;
   }
   else
   {
      System.addGroup( optionObj ) ;
   }
}

System.prototype.delGroup = function( name ) {
   if ( undefined != this._remote )
   {
      this._remote._runCommand( "system del group", {}, { "name" :name } ) ;
   }
   else
   {
      System.delGroup( name ) ;
   }
}

System.prototype.listLoginUsers = function( optionObj, filterObj ) {
   var retArray ;

   if ( undefined != this._remote )
   {
      var objArray ;
      var displayMode = "obj" ;

      // check argument
      if ( undefined != optionObj )
      {
         if ( undefined != optionObj.displayMode )
         {
            displayMode = optionObj.displayMode ;
            delete optionObj.displayMode ;
         }
         objArray = this._remote._runCommand( "system list login users",
                                              optionObj ) ;
      }
      else
      {
         objArray = this._remote._runCommand( "system list login users" ) ;
      }

      // filter
      if ( undefined != filterObj )
      {
         var filter = new _Filter( filterObj ) ;
         retArray = filter.match( objArray.toObj() ) ;
      }
      else
      {
         var filter = new _Filter( {} ) ;
         retArray = filter.match( objArray.toObj() ) ;
      }

      // set format
      if ( "text" == displayMode )
      {
         retArray = retArray._formatStr() ;
      }
   }
   else
   {
      retArray = System.listLoginUsers( optionObj, filterObj ) ;
   }
   return retArray ;
}

System.prototype.listAllUsers = function( optionObj, filterObj ) {
   var retArray ;

   if ( undefined != this._remote )
   {
      var objArray ;
      var displayMode = "obj" ;

      // check argument
      if ( undefined != optionObj )
      {
         if ( undefined != optionObj.displayMode )
         {
            displayMode = optionObj.displayMode ;
            delete optionObj.displayMode ;
         }
         objArray = this._remote._runCommand( "system list all users",
                                              optionObj ) ;
      }
      else
      {
         objArray = this._remote._runCommand( "system list all users" ) ;
      }

      // filter
      if ( undefined != filterObj )
      {
         var filter = new _Filter( filterObj ) ;
         retArray = filter.match( objArray.toObj() ) ;
      }
      else
      {
         var filter = new _Filter( {} ) ;
         retArray = filter.match( objArray.toObj() ) ;
      }

      // set display format
      if ( "text" == displayMode )
      {
         retArray = retArray._formatStr() ;
      }
   }
   else
   {
      retArray = System.listAllUsers( optionObj, filterObj ) ;
   }
   return retArray ;
}

System.prototype.listGroups = function( optionObj, filterObj ) {
   var retArray ;

   if ( undefined != this._remote )
   {
      var objArray ;
      var displayMode = "obj" ;

      // check argument
      if ( undefined != optionObj )
      {
         if ( undefined != optionObj.displayMode )
         {
            displayMode = optionObj.displayMode ;
            delete optionObj.displayMode ;
         }
         objArray = this._remote._runCommand( "system list all groups",
                                              optionObj ) ;
      }
      else
      {
         objArray = this._remote._runCommand( "system list all groups" ) ;
      }

      // filter
      if ( undefined != filterObj )
      {
         var filter = new _Filter( filterObj ) ;
         retArray = filter.match( objArray.toObj() ) ;
      }
      else
      {
         var filter = new _Filter( {} ) ;
         retArray = filter.match( objArray.toObj() ) ;
      }

      // set display format
      if ( "text" == displayMode )
      {
         retArray = retArray._formatStr() ;
      }
   }
   else
   {
      retArray = System.listGroups( optionObj, filterObj ) ;
   }

   return retArray ;
}

System.prototype.getCurrentUser = function() {
   var retObj ;

   if ( undefined != this._remote )
   {
      retObj = this._remote._runCommand( "system get current user" ) ;
   }
   else
   {
      retObj = System.getCurrentUser() ;
   }
   return retObj ;
}

System.prototype.isUserExist = function( userName ) {
   var isExist = false ;
   var retArray ;

   // check argument
   if ( undefined == userName )
   {
      setLastErrMsg( "userName must be config" ) ;
      throw SDB_OUT_OF_BOUND ;
   }

   if ( undefined != this._remote )
   {
      retArray = this.listAllUsers( { "detail": true }, { "user": userName } ) ;
   }
   else
   {
      retArray = System.listAllUsers( { "detail": true }, { "user": userName } ) ;
   }

   // check result
   if ( 0 != retArray.size() )
   {
      isExist = true ;
   }
   return isExist ;
}

System.prototype.isGroupExist = function( groupName ) {
   var isExist = false ;
   var retArray ;

   // check argument
   if ( undefined == groupName )
   {
      setLastErrMsg( "groupName must be config" ) ;
      throw SDB_OUT_OF_BOUND ;
   }

   if ( undefined != this._remote )
   {
      retArray = this.listGroups( { "detail": true }, { "name": groupName } ) ;
   }
   else
   {
      retArray = System.listGroups( { "detail": true }, { "name": groupName } ) ;
   }

   // check result
   if ( 0 != retArray.size() )
   {
      isExist = true ;
   }
   return isExist ;
}

System.prototype.killProcess = function( optionObj ) {

   // check argument
   if ( undefined == optionObj )
   {
      setLastErrMsg( "optionObj must be config" ) ;
      throw SDB_OUT_OF_BOUND ;
   }
   if ( "object" != typeof( optionObj ) )
   {
      setLastErrMsg( "optionObj must be BsonObj" ) ;
      throw SDB_INVALIDARG ;
   }
   if ( undefined == optionObj.sig )
   {
      optionObj.sig = "term"
   }

   if ( undefined != this._remote )
   {
      this._remote._runCommand( "system kill process",
                                { "sig" : optionObj.sig,
                                  "pid": optionObj.pid } ) ;
   }
   else
   {
      System.killProcess( optionObj ) ;
   }
}

System.prototype.getProcUlimitConfigs = function() {
   var retObj ;

   // check argument
   if ( 0 < arguments.length )
   {
      setLastErrMsg( "getUlimitConfigs() should have non arguments" ) ;
      throw SDB_INVALIDARG ;
   }

   if ( undefined != this._remote )
   {
      retObj = this._remote._runCommand( "system get proc ulimit configs" ) ;
   }
   else
   {
      retObj = System.getProcUlimitConfigs() ;
   }
   return retObj ;
}

System.prototype.setProcUlimitConfigs = function( configsObj ) {

   // check argument
   if ( undefined == configsObj )
   {
      setLastErrMsg( "configsObj must be configs" ) ;
      throw SDB_OUT_OF_BOUND ;
   }

   if ( undefined != this._remote )
   {
      this._remote._runCommand( "system set proc ulimit configs", {},{},
                                { "configs": configsObj } ) ;
   }
   else
   {
      System.setProcUlimitConfigs( configsObj ) ;
   }
}

System.prototype.getUserEnv = function() {
   var result ;
   if ( undefined != this._remote )
   {
      result = this._remote._runCommand( "system get user env" ) ;
   }
   else
   {
      result = System.getUserEnv() ;
   }
   return result ;
}

System.prototype.getSystemConfigs = function( type ) {
   var retObj ;

   if ( undefined != this._remote )
   {
      if ( undefined != type )
      {
         retObj = this._remote._runCommand( "system get system configs",
                                            { "type": type } ) ;
      }
      else
      {
         retObj = this._remote._runCommand( "system get system configs" ) ;
      }
   }
   else
   {
      if ( undefined != type )
      {
         retObj = System.getSystemConfigs( type ) ;
      }
      else
      {
         retObj = System.getSystemConfigs( ) ;
      }
   }
   return retObj ;
}

System.prototype.runService = function ( serviceName, command, options ) {
   var result ;

   // check argument
   if ( undefined == serviceName )
   {
      setLastErrMsg( "serviceName must be config" ) ;
      throw SDB_INVALIDARG ;
   }
   if ( 'string' != typeof( serviceName ) )
   {
      setLastErrMsg( "serviceName must be string" ) ;
      throw SDB_INVALIDARG ;
   }

   if ( undefined == command )
   {
      setLastErrMsg( "command must be config" ) ;
      throw SDB_INVALIDARG ;
   }
   if ( 'string' != typeof( command ) )
   {
      setLastErrMsg( "command must be string" ) ;
      throw SDB_INVALIDARG ;
   }

   if ( undefined != this._remote )
   {
      var retObj ;
      if ( undefined != options )
      {
         retObj = this._remote._runCommand( "system run service",
                                            { "command": command,
                                              "options": options },
                                            { "serviceName": serviceName } ) ;
      }
      else
      {
         retObj = this._remote._runCommand( "system run service",
                                            { "command": command },
                                            { "serviceName": serviceName } ) ;
      }
      result = retObj.toObj().outStr ;
   }
   else
   {
      if ( "undefined" != options )
      {
         result = System.runService( serviceName, command, options ) ;
      }
      else
      {
         result = System.runService( serviceName, command ) ;
      }
   }
   return result ;
}

System.prototype.buildTrusty = function() {

   if ( "LINUX" == System.type() )
   {
      if ( undefined != this._remote )
      {
         var homeDir = System._getHomePath() ;
         System._createSshKey() ;
         var pubKey = File._readFile( homeDir + "/.ssh/id_rsa.pub" ) ;
         this._remote._runCommand( "system build trusty", {}, {},
                                   { "key": pubKey } ) ;
      }
   }
}

System.prototype.removeTrusty = function() {

   if ( "LINUX" == System.type() )
   {
      if ( undefined != this._remote )
      {
         var homeDir = System._getHomePath() ;
         var matchStr = File._readFile( homeDir + "/.ssh/id_rsa.pub" ) ;
         this._remote._runCommand( "system remove trusty", {}, {},
                                   { "matchStr": matchStr } ) ;
      }
   }
}

System.prototype.getPID = function() {

   // check argument
   if ( 0 < arguments.length )
   {
      setLastErrMsg( "No need arguments" ) ;
      throw SDB_INVALIDARG ;
   }

   var pid ;
   if ( undefined != this._remote )
   {
      pid = this._remote._runCommand( "system get pid" ).toObj().PID ;
   }
   else
   {
      pid = System.getPID() ;
   }
   return pid ;
}

System.prototype.getTID = function() {

   // check argument
   if ( 0 < arguments.length )
   {
      setLastErrMsg( "No need arguments" ) ;
      throw SDB_INVALIDARG ;
   }

   var tid ;
   if ( undefined != this._remote )
   {
      tid = this._remote._runCommand( "system get tid" ).toObj().TID ;
   }
   else
   {
      tid = System.getTID() ;
   }
   return tid ;
}

System.prototype.getEWD = function() {

   // check argument
   if ( 0 < arguments.length )
   {
      setLastErrMsg( "No need arguments" ) ;
      throw SDB_INVALIDARG ;
   }

   var ewd ;
   if ( undefined != this._remote )
   {
      ewd = this._remote._runCommand( "system get ewd" ).toObj().EWD ;
   }
   else
   {
      ewd = System.getEWD() ;
   }
   return ewd ;
}
// end System

// Cmd member function
Cmd.prototype.getInfo = function() {

   var result ;
   var infoObj ;
   if ( undefined != this._remote )
   {
      var _remoteInfo = this._remote.getInfo() ;
      infoObj = _remoteInfo.toObj() ;
      infoObj.isRemote = true ;
   }
   else
   {
      infoObj = new Object() ;
      infoObj.isRemote = false ;
   }
   result = this._getInfo( infoObj ) ;
   return result ;
}

Cmd.prototype.getCommand = function() {

   if ( undefined != this._remote )
   {
      return this._command ;
   }
   else
   {
      return this._getCommand() ;
   }
}

Cmd.prototype.getLastOut = function() {

   if ( undefined != this._remote )
   {
      return this._strOut ;
   }
   else
   {
      return this._getLastOut() ;
   }
}

Cmd.prototype.getLastRet = function() {

   if ( undefined != this._remote )
   {
      return this._retCode ;
   }
   else
   {
      return this._getLastRet() ;
   }
}

Cmd.prototype.run = function( cmd, args, timeout, useShell ) {
   var retStr ;

   // check argument
   if ( 1 > arguments.length )
   {
      setLastErrMsg( "cmd must be config" ) ;
      throw SDB_INVALIDARG ;
   }

   if ( "string" != typeof( cmd ) )
   {
      setLastErrMsg( "cmd must be string" ) ;
      throw SDB_INVALIDARG ;
   }
   this._command = cmd ;

   if ( 2 > arguments.length )
   {
      args = "" ;
   }
   else
   {
      if ( "string" != typeof( args ) )
      {
         setLastErrMsg( "environment should be a string" ) ;
         throw SDB_INVALIDARG ;
      }
      this._command += " " + args ;
   }

   if ( 3 > arguments.length )
   {
      timeout = 0 ;
   }
   if ( 4 > arguments.length )
   {
      useShell = 1 ;
   }

   // remote call
   if ( undefined != this._remote )
   {
      var retObj ;

      try
      {
         retObj = this._remote._runCommand( "cmd run",
                                            { "timeout": timeout,
                                              "useShell": useShell }, {},
                                            { "command": cmd,
                                              "args": args } ).toObj() ;
         this._retCode = getLastError() ;
         this._strOut = retObj.strOut ;
      }
      catch( e )
      {
         if( 0 <= e )
         {
            this._retCode = e ;
            this._strOut = getLastErrMsg() ;
         }
         throw e ;
      }
      retStr = this._strOut ;
   }
   else
   {
      if ( undefined != useShell )
      {
         retStr = this._run( cmd, args, timeout, useShell ) ;
      }
   }
   return retStr ;
}

Cmd.prototype.start = function( cmd, args, useShell, timeout ) {
   var retPid ;

   // check argument
   if ( 1 > arguments.length )
   {
      setLastErrMsg( "cmd must be config" ) ;
      throw SDB_INVALIDARG ;
   }

   if ( "string" != typeof( cmd ) )
   {
      setLastErrMsg( "cmd must be string" ) ;
      throw SDB_INVALIDARG ;
   }
   this._command = cmd ;

   if ( 2 > arguments.length )
   {
      args = "" ;
   }
   else
   {
      if ( "string" != typeof( args ) )
      {
         setLastErrMsg( "environment should be a string" ) ;
         throw SDB_INVALIDARG ;
      }
      this._command += " " + args ;
   }

   if ( 3 > arguments.length )
   {
      useShell = 1 ;
   }
   if ( 4 > arguments.length )
   {
      timeout = 100 ;
   }

   if ( undefined != this._remote )
   {
      var recvObj ;
      try
      {
         recvObj = this._remote._runCommand( "cmd start",
                                             { "useShell": useShell,
                                               "timeout": timeout }, {},
                                             { "command": cmd,
                                               "args": args } ).toObj() ;
         this._retCode = getLastError() ;
         this._strOut = recvObj.strOut ;
      }
      catch( e )
      {
         if( 0 <= e )
         {
            this._retCode = e ;
            this._strOut = getLastErrMsg() ;
         }
         throw e ;
      }
      retPid = recvObj.pid ;
   }
   else
   {
      retPid = this._start( cmd, args, useShell, timeout ) ;
   }
   return retPid ;
}

Cmd.prototype.runJS = function( code ) {
   var code ;

   // check argument
   if ( undefined == code )
   {
      setLastErrMsg( "code must be config" ) ;
      throw SDB_OUT_OF_BOUND ;
   }
   if ( "string" != typeof( code ) )
   {
      setLastErrMsg( "code must be config" ) ;
      throw SDB_INVALIDARG ;
   }

   // check if is remote obj
   if ( undefined != this._remote )
   {
      var recvObj ;
      recvObj = this._remote._runCommand( "cmd run js", {}, {},
                                            { "code": code } ) ;
      return recvObj.toObj().strOut ;
   }
   else
   {
      setLastErrMsg( "runJS() should be called by remote cmd obj" ) ;
      throw SDB_SYS ;
   }
}
// end Cmd

// File static function
File.list = function( optionObj, filterObj ) {

   var retObj ;
   var objArr ;
   var result ;
   var displayMode = "obj" ;

   // check argument
   if ( undefined != optionObj )
   {
      if ( undefined != optionObj.displayMode )
      {
         displayMode = optionObj.displayMode ;
         delete optionObj.displayMode ;

      }
      retObj = File._list( optionObj ) ;
   }
   else
   {
      retObj = File._list() ;
   }

   if ( undefined != filterObj )
   {
      var filter = new _Filter( filterObj ) ;
      objArr = filter.match( retObj.toObj() ) ;
   }
   else
   {
      var filter = new _Filter( {} ) ;
      objArr =  filter.match( retObj.toObj() ) ;
   }

   if ( "text" == displayMode )
   {
      result = objArr._formatStr() ;
   }
   else
   {
      result = objArr ;
   }

   return result ;
}

File.isFile = function( pathname ) {

   // check argument
   if ( undefined == pathname )
   {
      setLastErrMsg( "pathname must be config" ) ;
      throw SDB_OUT_OF_BOUND ;
   }

   // check return file type
   if ( "FIL" == File._getPathType( pathname ) )
   {
      return true ;
   }
   else
   {
      return false ;
   }
}

File.isDir = function( pathname ) {

   // check argument
   if ( undefined == pathname )
   {
      setLastErrMsg( "pathname must be config" ) ;
      throw SDB_OUT_OF_BOUND ;
   }

   // check return file type
   if ( "DIR" == File._getPathType( pathname ) )
   {
      return true ;
   }
   else
   {
      return false ;
   }
}

File.find = function( optionObj, filterObj ) {
   var recvObj ;
   var retArr ;
   var result ;
   var displayMode = "obj" ;

   // check argument
   if ( undefined == optionObj )
   {
      setLastErrMsg( "optionObj must be config" ) ;
      throw SDB_OUT_OF_BOUND ;
   }

   if ( undefined != optionObj.displayMode )
   {
      displayMode = optionObj.displayMode ;
      delete optionObj.displayMode ;
   }

   recvObj  = File._find( optionObj ) ;

   // filter
   if ( undefined != filterObj )
   {
      var filter = new _Filter( filterObj ) ;
      retArr = filter.match( recvObj.toObj() ) ;
   }
   else
   {
      var filter = new _Filter( {} ) ;
      retArr = filter.match( recvObj.toObj() ) ;
   }

   // set display format
   if ( "text" == displayMode )
   {
      result = retArr._formatStr() ;
   }
   else
   {
      result = retArr ;
   }
   return result ;
}

File.getUmask = function( base ) {

   var umaskStr = File._getUmask() ;
   var umask = parseInt( umaskStr, 8 ) ;
   if ( undefined != base )
   {
      if ( "string" == typeof( base ) )
      {
         base = parseInt( base ) ;
      }
      if ( 8 == base )
      {
         umask = ( "0000" + umask.toString( 8 ) ).substr( -4 ) ;
      }
      else if ( 10 == base )
      {
         umask = umask.toString( 10 ) ;
      }
      else if ( 16 == base )
      {
         umask = '0x' + umask.toString( 16 ) ;
      }
      else
      {
         setLastErrMsg( "base must be number( 8/10/16 )" ) ;
         throw SDB_INVALIDARG ;
      }
   }
   return umask ;
}

File.scp = function( src, dst, isReplace, mode ) {

   if( undefined == src )
   {
      setLastErrMsg( "src must be config" ) ;
      throw SDB_OUT_OF_BOUND ;
   }
   if( "string" != typeof( src ) )
   {
      setLastErrMsg( "src must be string" ) ;
      throw SDB_INVALIDARG ;
   }

   if( undefined == dst )
   {
      setLastErrMsg( "dst must be config" ) ;
      throw SDB_OUT_OF_BOUND ;
   }
   if( "string" != typeof( dst ) )
   {
      setLastErrMsg( "dst must be string" ) ;
      throw SDB_INVALIDARG ;
   }

   var srcFile ;
   var dstFile ;
   var COPY_UNIT = 4*1024*1024 ;
   var srcArr = src.split( "@" ) ;
   var dstFilename ;
   if( srcArr.length > 1 )
   {
      var hostPortSplit = srcArr[0].split( ":" ) ;
      var remote = new Remote( hostPortSplit[0], hostPortSplit[1] ) ;
      var fileMgr = remote.getFile() ;

      if( false == fileMgr.exist( srcArr[1] ) )
      {
         setLastErrMsg( "src not exist" ) ;
         throw SDB_FNE ;
      }
      if( undefined == mode )
      {
         mode = fileMgr._getPermission( srcArr[1] ) ;
      }
      srcFile = remote.getFile( srcArr[1], 0644, SDB_FILE_READONLY ) ;
   }
   else
   {
      if( false == File.exist( srcArr[0] ) )
      {
         setLastErrMsg( "src not exist" ) ;
         throw SDB_FNE ;
      }
      if( undefined == mode )
      {
         mode = File._getPermission( srcArr[0] ) ;
      }
      srcFile = new File( srcArr[0], 0644, SDB_FILE_READONLY ) ;
   }

   var dstArr = dst.split( "@" ) ;
   if( dstArr.length > 1 )
   {
      var hostPortSplit = dstArr[0].split( ":" ) ;
      var remote = new Remote( hostPortSplit[0], hostPortSplit[1] ) ;
      var fileMgr = remote.getFile() ;
      dstFilename = dstArr[1] ;
      if( true == fileMgr.exist( dstFilename ) )
      {
         if( false == isReplace )
         {
            setLastErrMsg( "dst exist" ) ;
            throw SDB_FE ;
         }
         else
         {
            dstFile = remote.getFile( dstFilename, mode,
                                      SDB_FILE_REPLACE| SDB_FILE_READWRITE ) ;
         }
      }
      else
      {
         dstFile = remote.getFile( dstFilename, mode,
                                   SDB_FILE_CREATEONLY | SDB_FILE_READWRITE ) ;
      }
   }
   else
   {
      dstFilename = dstArr[0] ;
      if( true == File.exist( dstFilename ) )
      {
         if( false == isReplace )
         {
            setLastErrMsg( "dst exist" ) ;
            throw SDB_FE ;
         }
         else
         {
            dstFile = new File( dstFilename, mode,
                                SDB_FILE_REPLACE | SDB_FILE_READWRITE ) ;
         }
      }
      else
      {
         dstFile = new File( dstFilename, mode,
                             SDB_FILE_CREATEONLY | SDB_FILE_READWRITE ) ;
      }
   }

   try
   {
      while( true )
      {
         var fileContent = srcFile.readContent( COPY_UNIT ) ;
         dstFile.writeContent( fileContent ) ;
         fileContent.clear() ;
      }
   }
   catch( e )
   {
      srcFile.close() ;
      dstFile.close() ;
      if( -9 == e )
      {
         println( "Success to copy file from " + src + " to " + dst ) ;
      }
      else
      {
         throw e ;
      }
   }
}

// File member function
File.prototype.getInfo = function() {
   var result ;
   var infoObj ;
   if ( undefined != this._remote )
   {
      var _remoteInfo = this._remote.getInfo() ;
      infoObj = _remoteInfo.toObj() ;
      infoObj.isRemote = true ;
      infoObj.filename = this._filename ;
   }
   else
   {
      infoObj = new Object() ;
      infoObj.isRemote = false ;
   }
   result = this._getInfo( infoObj ) ;
   return result ;
}

File.prototype.read = function( size ) {
   var str ;

   if ( undefined != this._remote )
   {
      var retObj ;
      if ( undefined != size )
      {
         retObj = this._remote._runCommand( "file read", {},
                                            { "FID": this._FID },
                                            { "Size": size } ) ;
      }
      else
      {
         retObj = this._remote._runCommand( "file read", {},
                                            { "FID": this._FID } ) ;
      }
      var recvObj = retObj.toObj() ;
      str = recvObj.Content ;
   }
   else
   {
      if ( undefined != size )
      {
         str = this._read( size ) ;
      }
      else
      {
         str = this._read() ;
      }
   }
   return str ;
}

File.prototype.readContent = function( size )
{
   var retObj ;
   if ( undefined != this._remote )
   {
      if( undefined != size )
      {
         retObj = this._readContent( this._remote, this._FID, size ) ;
      }
      else
      {
         retObj = this._readContent( this._remote, this._FID ) ;
      }
   }
   else
   {
      if( undefined != size )
      {
         retObj = this._readContent( size ) ;
      }
      else
      {
         retObj = this._readContent() ;
      }
   }
   return retObj ;
}

File.prototype.write = function( content ){

   if ( undefined != this._remote )
   {
      this._remote._runCommand( "file write", {}, { "FID": this._FID },
                                { "Content": content } ) ;
   }
   else
   {
      this._write( content ) ;
   }
}

File.prototype.writeContent = function( content )
{
   if( undefined != this._remote )
   {
      this._writeContent( this._remote, this._FID, content ) ;
   }
   else
   {
      this._writeContent( content ) ;
   }
}

File.prototype.seek = function( offset, where ) {

   // check argument
   if ( undefined == offset )
   {
      setLastErrMsg( "Offset must be config" ) ;
      throw SDB_OUT_OF_BOUND ;
   }

   var optionObj = {} ;
   if ( undefined != where )
   {
      optionObj.where = where ;
   }

   if ( undefined != this._remote )
   {
      this._remote._runCommand( "file seek", optionObj, { "FID": this._FID },
                                { "SeekSize": offset } ) ;
   }
   else
   {
      this._seek( offset, optionObj ) ;
   }
}

File.prototype.close = function() {
   if ( undefined == this._remote )
   {
      this._close() ;
   }
   else
   {
      if( this._FID != undefined )
      {
         this._remote._runCommand( "file close", {}, { "FID": this._FID } ) ;
      }
   }
}

File.prototype.remove = function( filepath ) {

   // check argument
   if ( undefined == filepath )
   {
      setLastErrMsg( "filepath must be config" ) ;
      throw SDB_OUT_OF_BOUND ;
   }

   if ( undefined != this._remote )
   {
      this._remote._runCommand( "file remove", {}, {},
                               { "Pathname" : filepath } ) ;
   }
   else
   {
      File.remove( filepath ) ;
   }
}

File.prototype.exist = function( filepath ) {
   var isExist ;

   // check argument
   if ( undefined == filepath )
   {
      setLastErrMsg( "filepath must be config" ) ;
      throw SDB_OUT_OF_BOUND ;
   }

   if ( undefined != this._remote )
   {
      var recvObj = this._remote._runCommand( "file is exist", {}, {},
                                              { "Pathname" : filepath } ) ;
      isExist = recvObj.toObj().IsExist ;
   }
   else
   {
      isExist = File.exist( filepath ) ;
   }
   return isExist ;
}

File.prototype.copy = function( src, dst, replace, mode ) {

   // check argument
   if ( undefined == src )
   {
      setLastErrMsg( "Src must be config" ) ;
      throw SDB_OUT_OF_BOUND ;
   }
   if ( undefined == dst )
   {
      setLastErrMsg( "Dst must be config" ) ;
      throw SDB_OUT_OF_BOUND ;
   }

   if ( undefined != this._remote )
   {
      var optionObj = {} ;
      if ( undefined != mode )
      {
         optionObj.mode = mode ;
      }
      if ( undefined != replace )
      {
         optionObj.isReplace = replace ;
      }
      this._remote._runCommand( "file copy", optionObj,
                                { "Src": src }, { "Dst": dst } ) ;
   }
   else
   {
      if ( undefined != mode )
      {
         File.copy( src, dst, replace, mode ) ;
      }
      else if ( undefined != replace )
      {
         File.copy( src, dst, replace ) ;
      }
      else
      {
         File.copy( src, dst ) ;
      }
   }
}

File.prototype.move = function( src, dst ) {

   // check argument
   if ( undefined == src )
   {
      setLastErrMsg( "Src must be config" ) ;
      throw SDB_OUT_OF_BOUND ;
   }
   if ( undefined == dst )
   {
      setLastErrMsg( "Dst must be config" ) ;
      throw SDB_OUT_OF_BOUND ;
   }

   if ( undefined != this._remote )
   {
      this._remote._runCommand( "file move", {}, { "Src": src },
                              { "Dst": dst } ) ;
   }
   else
   {
      File.move( src, dst ) ;
   }
}

File.prototype.mkdir = function( name, mode ) {

   // check argument
   if ( undefined == name )
   {
      setLastErrMsg( "Name must be config" ) ;
      throw SDB_OUT_OF_BOUND ;
   }

   if ( undefined != this._remote )
   {
      var optionObj = {} ;
      if( undefined != mode )
      {
         optionObj.mode = mode ;
      }
      this._remote._runCommand( "file mkdir", optionObj, {},
                               { "Dirname": name } ) ;
   }
   else
   {
      if( undefined != mode )
      {
         File.mkdir( name, mode ) ;
      }
      else
      {
         File.mkdir( name ) ;
      }
   }
}

File.prototype.find = function( optionObj, filterObj ) {

   // check argument
   if ( undefined == optionObj )
   {
      setLastErrMsg( "OptionObj must be config" ) ;
      throw SDB_OUT_OF_BOUND  ;
   }
   if ( false == optionObj instanceof Object )
   {
      setLastErrMsg( "OptionObj must be Object" ) ;
      throw SDB_INVALIDARG  ;
   }

   var result ;
   if ( undefined != this._remote )
   {
      var matchResult ;
      var displayMode = "obj" ;
      if ( undefined != optionObj.displayMode )
      {
         displayMode = optionObj.displayMode ;
         delete optionObj.displayMode ;
      }

      var recvObj = this._remote._runCommand( "file find", optionObj ).toObj() ;

      // filter
      if ( undefined != filterObj )
      {
         var filter = new _Filter( filterObj ) ;
         matchResult = filter.match( recvObj ) ;
      }
      else
      {
         var filter = new _Filter( {} ) ;
         matchResult = filter.match( recvObj ) ;
      }

      // set display format
      if ( "text" == displayMode )
      {
         result = matchResult._formatStr() ;
      }
      else
      {
         result = matchResult ;
      }
   }
   else
   {
      result = File.find( optionObj, filterObj ) ;
   }

   return result ;
}

File.prototype.chmod = function( filename, mode, recursive ) {

   if ( undefined != this._remote )
   {
      var optionObj = {} ;
      if ( undefined != recursive )
      {
         optionObj.isRecursive = recursive ;
      }
      this._remote._runCommand( "file chmod", optionObj, { "Pathname": filename },
                                { "Mode": mode } ) ;
   }
   else
   {
      if ( undefined != recursive )
      {
         File.chmod( filename, mode, recursive ) ;
      }
      else
      {
         File.chmod( filename, mode ) ;
      }
   }
}

File.prototype.toString = function()
{
   var result ;
   if ( undefined != this._remote )
   {
      if ( undefined != this._filename )
      {
         result = this._filename ;
      }
      else
      {
         result = "" ;
      }
   }
   else
   {
      result = this._toString() ;
   }
   return result ;
}

File.prototype.chown = function( filename, optionObj, recursive ) {
   if ( undefined != this._remote )
   {
      if ( undefined != recursive )
      {
         this._remote._runCommand( "file chown", { "isRecursive": recursive },
                                   { "Pathname": filename }, optionObj ) ;
      }
      else
      {
         this._remote._runCommand( "file chown", {}, { "Pathname": filename },
                                  optionObj ) ;
      }
   }
   else
   {
      if ( undefined != recursive )
      {
         File.chown( filename, optionObj, recursive ) ;
      }
      else
      {
         File.chown( filename, optionObj ) ;
      }
   }
}

File.prototype.chgrp = function( filename, groupname, recursive ) {
   if ( undefined != this._remote )
   {
      if ( undefined != recursive )
      {
         this._remote._runCommand( "file chgrp", { "isRecursive": recursive },
                                   { "Pathname": filename },
                                   { "Groupname": groupname } ) ;
      }
      else
      {
         this._remote._runCommand( "file chgrp", {}, { "Pathname": filename },
                                  { "Groupname": groupname } ) ;
      }
   }
   else
   {
      if ( undefined != recursive )
      {
         File.chgrp( filename, groupname, recursive ) ;
      }
      else
      {
         File.chgrp( filename, groupname ) ;
      }
   }
}

File.prototype.setUmask = function( mask ) {

   // check argument
   if ( undefined == mask )
   {
      setLastErrMsg( "Mask must be config" ) ;
      throw SDB_OUT_OF_BOUND ;
   }

   if ( undefined != this._remote )
   {
      this._remote._runCommand( "file set umask", {}, {}, { "Mask": mask } ) ;
   }
   else
   {
      File.setUmask( mask ) ;
   }
}

File.prototype.getUmask = function( base ) {
   var result ;

   if ( undefined != this._remote )
   {
      var recvObj = this._remote._runCommand( "file get umask" ) ;
      var umask = parseInt( recvObj.toObj().Mask, 8 ) ;
      if ( undefined != base )
      {
         if ( "string" == typeof( base ) )
         {
            base = parseInt( base ) ;
         }
         if ( 8 == base )
         {
            umask = ( "0000" + umask.toString( 8 ) ).substr( -4 ) ;
         }
         else if ( 10 == base )
         {
            umask = umask.toString( 10 ) ;
         }
         else if ( 16 == base )
         {
            umask = "0x" + umask.toString( 16 ) ;
         }
         else
         {
            setLastErrMsg( "base must be number( 8/10/16 )" ) ;
            throw SDB_INVALIDARG ;
         }
      }
      result = umask ;
   }
   else
   {
      result = File.getUmask( base ) ;
   }
   return result ;
}

File.prototype.list = function( optionObj, filterObj ) {
   var result ;
   if ( undefined == optionObj )
   {
      optionObj = {} ;
   }

   if ( undefined != this._remote )
   {
      var displayMode = "obj" ;
      if ( undefined != optionObj.displayMode )
      {
         displayMode = optionObj.displayMode ;
         delete optionObj.displayMode ;
      }

      var recvObj = this._remote._runCommand( "file list", optionObj ).toObj() ;
      var matchResult ;
      if ( undefined != filterObj )
      {
         var filter = new _Filter( filterObj ) ;
         matchResult = filter.match( recvObj ) ;
      }
      else
      {
         var filter = new _Filter( {} ) ;
         matchResult = filter.match( recvObj ) ;
      }

      if ( "text" == displayMode )
      {
         result = matchResult._formatStr() ;
      }
      else
      {
         result = matchResult ;
      }
   }
   else
   {
      result = File.list( optionObj, filterObj ) ;
   }
   return result ;
}

File.prototype.isFile = function( pathname ) {

   // check argument
   if ( undefined == pathname )
   {
      setLastErrMsg( "mask must be config" ) ;
      throw SDB_OUT_OF_BOUND ;
   }

   var result = false ;
   if ( undefined != this._remote )
   {
      var recvObj = this._remote._runCommand( "file get path type", {},
                                             { "Pathname":pathname } ) ;
      if ( "FIL" == recvObj.toObj().PathType )
      {
         result = true ;
      }
   }
   else
   {
      result = File.isFile( pathname ) ;
   }
   return result ;
}

File.prototype.isDir = function( pathname ) {

   // check argument
   if ( undefined == pathname )
   {
      setLastErrMsg( "mask must be config" ) ;
      throw SDB_OUT_OF_BOUND ;
   }

   var result = false ;
   if ( undefined != this._remote )
   {
      var recvObj = this._remote._runCommand( "file get path type", {},
                                             { "Pathname":pathname } ) ;
      if ( "DIR" == recvObj.toObj().PathType )
      {
         result = true ;
      }
   }
   else
   {
      result = File.isDir( pathname ) ;
   }
   return result ;
}

File.prototype.isEmptyDir = function( pathname ) {

   // check argument
   if ( undefined == pathname )
   {
      setLastErrMsg( "Pathname must be config" ) ;
      throw SDB_OUT_OF_BOUND ;
   }

   var result ;
   if ( undefined != this._remote )
   {
      var recvObj = this._remote._runCommand( "file is empty dir", {},
                                             { "Pathname":pathname } ) ;

      var isEmpty = recvObj.toObj().IsEmpty ;
      if ( isEmpty )
      {
         result = true ;
      }
      else
      {
         result = false ;
      }
   }
   else
   {
      result = File.isEmptyDir( pathname ) ;
   }
   return result ;
}

File.prototype.stat = function( filename ) {

   // check argument
   if ( undefined == filename )
   {
      setLastErrMsg( "Filename must be config" ) ;
      throw SDB_OUT_OF_BOUND ;
   }

   var result ;
   if ( undefined != this._remote )
   {
      result = this._remote._runCommand( "file stat", {},
                                        { "Filename": filename } ) ;
   }
   else
   {
      result = File.stat( filename ) ;
   }
   return result ;
}

File.prototype.md5 = function( filename ) {

   // check argument
   if ( undefined == filename )
   {
      setLastErrMsg( "Filename must be config" ) ;
      throw SDB_OUT_OF_BOUND ;
   }

   var result ;
   if ( undefined != this._remote )
   {
      var recvObj = this._remote._runCommand( "file md5", {},
                                              { "Filename": filename } ) ;
      result = recvObj.toObj().MD5 ;
   }
   else
   {
      result = File.md5( filename ) ;
   }
   return result ;

}

File.prototype._getPermission = function( pathname ) {

   if( undefined == pathname )
   {
      setLastErrMsg( "Pathname must be config" ) ;
      throw SDB_OUT_OF_BOUND ;
   }
   else if( "string" != typeof( pathname ) )
   {
      setLastErrMsg( "Pathname must be string" ) ;
      throw SDB_INVALIDARG ;
   }

   if( undefined != this._remote )
   {
      var retObj = this._remote._runCommand( "file get permission", {}, {},
                                             { "Pathname": pathname } ).toObj() ;
      return retObj.Permission ;
   }
   else
   {
      return File._getPermission( pathname ) ;
   }
}

File.prototype.getSize = function( filename ) {
   if( undefined != this._remote )
   {
      var retObj = this._remote._runCommand( "file get content size", {},
                                             { "Filename": filename } ).toObj() ;
      return retObj.Size ;
   }
   else
   {
      return File.getSize( filename ) ;
   }
}

File.prototype.readLine = function() {
   var retStr ;
   if( undefined != this._remote )
   {
      var retObj = this._remote._runCommand( "file read line", {},
                                             { "FID": this._FID } ) ;
      retStr = retObj.toObj().Content ;
   }
   else
   {
      retStr = this._readLine() ;
   }
   return retStr ;
}

// end File
