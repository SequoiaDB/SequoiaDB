/* ******************************************************************************
@Description: 集群操作脚本
@Modify list:
@   2016-01-19 Jianhui Xu     Init
@   2017-07-20 Jianhui Xu     引入参数动态生效，集群只读机制；引擎版本必须 2.8.2 及以上。
@   2017-11-15 Jiaming Wu     将串行的集群重启操作优化为并发操作。
@   2020-12-18 QinCheng Yang  去除对系统用户名、密码的依赖
****************************************************************************** */

/* SequoiaDB 安装目录定义，必须以 '/' 结尾 */
if ( typeof(SEQPATH) != "string" || SEQPATH.length == 0 ) { SEQPATH = "/opt/sequoiadb/" ; }
/* 机器登入用户名定义, 3.2 及以上版本无须填写 */
if ( typeof(USERNAME) != "string" ) { USERNAME = "sdbadmin" ; }
/* 机器登入密码定义，3.2 及以上版本无须填写 */
if ( typeof(PASSWD) != "string" ) { PASSWD = "sdbadmin" ; }
/* 数据库登入用户名定义 */
if ( typeof(SDBUSERNAME) != "string" ) { SDBUSERNAME = "" ; }
/* 数据库登入密码定义 */
if ( typeof(SDBPASSWD) != "string" ) { SDBPASSWD = "" ; }
/* 子网1机器定义，必须为字符串数组 */
if ( typeof(SUB1HOSTS) == "undefined" ) { SUB1HOSTS = [ "vmsvr2-suse-x64-1" ] ; }
/* 子网2机器定义，必须为字符串数组 */
if ( typeof(SUB2HOSTS) == "undefined" ) { SUB2HOSTS = [ "vmsvr2-cent-x64" ] ; }
/* 协调节点定义，如果协调节点已经在 Catalog的编目组信息中，则此处填写一个可用Coord即可 */
if ( typeof(COORDADDR) == "undefined" ) { COORDADDR = [ "vmsvr2-suse-x64-1:50000" ] }
/* 当前子网取值, 1表示子网1，2表示子网2，其它取值非法 */
if ( typeof(CURSUB) == "undefined" ) { CURSUB = 1 ; }
/* 当前操作，取值 "init", "split", "merge" */
if ( typeof(CUROPR) == "undefined" ) { CUROPR = "split" ; }
/* 是否激活该子网集群，取值 true/false */
if ( typeof(ACTIVE) == "undefined" ) { ACTIVE = true ; }
/* 剔除故障组节点后剩余的最小副本数, 若剔除后剩余副本数小于最小副本数，将不会执行剔除操作 */
if ( typeof(MINREPLICANUM) == "undefined" ) { MINREPLICANUM = 2 ; }
/* 执行init时是否重新选举 */
if ( typeof(NEEDREELECT) == "undefined" ) { NEEDREELECT = true }
/* 是否将init文件分发到集群的所有主机上 */
if ( typeof(NEEDBROADCASTINITINFO) == "undefined" ) { NEEDBROADCASTINITINFO = true }
/* 内部定义, 请勿修改 */
if ( SEQPATH.charAt( SEQPATH.length - 1 ) != '/' ) { SEQPATH += '/' ; }
var SDBSTART = SEQPATH + "bin/sdbstart" ;
var SDBSTOP  = SEQPATH + "bin/sdbstop" ;
var SDBLIST  = SEQPATH + "bin/sdblist" ;
var SDBSHELL = SEQPATH + "bin/sdb" ;
var CONFLOCAL= SEQPATH + "conf/local" ;
var INITFILE = SEQPATH + "datacenter_init.info" ;
var ALLHOSTS = [] ;
var CURHOSTS = [] ;
var CURCATAS = [] ;   // only catalog
var CURDATAS = [] ;   // only data
var CURCOORDS= [] ;   // only coord
var CATAADDRLINE = "" ;
var READSIZE = 655360 ;
var VERSION_DIVIDE = "3.2" ;
var NEW_VERSION = false ;
var NODE_CONF_CATALOGADDR = "catalogaddr" ;
var NODE_CONF_ROLE = "role" ;

/* *****************************************************************************
@discription: 从地址中分解出Hostname和SvcName
@nodeAddr : address( string ), ex: '192.168.10.106:30000'
@author: Jianhui Xu
@return: string array
         ex: [ 'hostname', 'svcname' ]
***************************************************************************** */
function splitHostAndSvcFromAddr( nodeAddr ) {
   var infoArray = new Array() ;
   var pos = nodeAddr.indexOf( ":" ) ;
   if ( -1 != pos ) {
      infoArray[0] = nodeAddr.substring( 0, pos ) ;
      infoArray[1] = nodeAddr.substring( pos + 1 ) ;
   } else {
      infoArray[0] = nodeAddr ;
      infoArray[1] = "11810" ;
   }
   //println( "HostName: " + infoArray[0] + ", SvcName: " + infoArray[1] ) ;
   return infoArray ;
}

/* *****************************************************************************
@discription: 从Group Object Array中提取出编目，协调和数据组的节点信息
@objarray : group obj array, ex: [ { group info obj}, { group info obj} ]
@keepHosts : 过滤的 hostname 数组, ex: [ "a", "b" ]，当为空时表示不过滤
@author: Jianhui Xu
@return: array[][], array[0] for catalog nodes array
                    array[1] for data nodes array
                    array[2] for coord nodes array
        ex: [
                [ "192.168.20.106:30000"],
                [ "192.168.20.106:20000", "192.168.20.106:40000" ],
                [ "192.168.20.106:50000" ]
            ]
   error with exception
***************************************************************************** */
function parseGroupNodes( objarray, keepHosts ) {
   var retarray = new Array() ;
   retarray.push( new Array() ) ;  //0 catalog
   retarray.push( new Array() ) ;  //1 data
   retarray.push( new Array() ) ;  //2 coord
   var index = 0 ;

   for ( var i = 0 ; i < objarray.length ; ++i ) {
      var tmpObj = objarray[i] ;
      if ( 1 == tmpObj["GroupID"] ) {
         index = 0 ;
      } else if ( 2 == tmpObj["GroupID"] ) {
         index = 2 ;
      } else {
         index = 1 ;
      }
      var tmpGroupArray = tmpObj["Group"] ;
      for ( var j = 0 ; j < tmpGroupArray.length ; ++j ) {
         var tmpNodeObj = tmpGroupArray[j] ;
         var nodename = tmpNodeObj["HostName"] ;
         /* Filter jduge */
         if ( keepHosts.length != 0 && -1 == keepHosts.indexOf( nodename ) ) {
            /* the host will be filtered */
            continue ;
         }
         /* Get the svcname */
         for ( var k = 0 ; k < tmpNodeObj.Service.length ; ++k ) {
            var tmpSvcObj = tmpNodeObj.Service[k] ;
            if ( tmpSvcObj["Type"] == 0 ) {
               nodename = nodename + ":" + tmpSvcObj["Name"] ;
               retarray[index].push( nodename ) ;
               break ;
            }
         } // end for service
      } // end for node
   } // end for group

   return retarray ;
}

/* *****************************************************************************
@discription: 合并数据并踢重
@left : array
@right: array
@return: array
***************************************************************************** */
function mergeArrayWithoutRepeat( left, right ) {
   var newArray = new Array() ;
   /* merge left */
   for ( var i = 0 ; i < left.length ; ++i ) {
      if ( -1 == newArray.indexOf( left[i] ) ) {
         newArray.push( left[i] ) ;
      }
   }
   /* merge right */
   for ( var j = 0 ; j < right.length ; ++j ) {
      if ( -1 == newArray.indexOf( right[j] ) ) {
         newArray.push( right[j] ) ;
      }
   }
   return newArray ;
}

/* *****************************************************************************
@discription: 根据keepHosts生成新的nodes array，如果keepHosts为空，则全部有效
@nodesarray : array, ex:[ "192.168.20.106:20000" ]
@keepHosts: array, ex:[ "192.168.20.106" ]
@return: array
***************************************************************************** */
function makeNodesArrayWithKeepHosts( nodesarray, keepHosts ) {
   var newArray = new Array() ;
   /* merge right */
   for ( var i = 0 ; i < nodesarray.length ; ++i ) {
      var nodesinfo = splitHostAndSvcFromAddr( nodesarray[i] ) ;
      if ( keepHosts.length != 0 && -1 == keepHosts.indexOf( nodesinfo[0] ) ) {
         continue ;
      }
      newArray.push( nodesarray[i] ) ;
   }
   return newArray ;
}

/* *****************************************************************************
@discription: 根据keepHosts生成新的addr line，如果keepHosts为空，则不改变
@addrLine : string, ex:"r730-90:11823,r730-91:11823,r730-92:11823"
@keepHosts: array, ex:[ "192.168.20.106" ]
@return: string
***************************************************************************** */
function makeAddrLineWithKeepHosts( addrLine, keepHosts ) {
   if ( keepHosts.length == 0 ) {
      return addrLine ;
   }
   var addrArray = addrLine.split( "," ) ;
   var nodeInfo ;
   var newArray = new Array() ;
   for ( var i = 0 ; i < addrArray.length ; ++i ) {
      nodeInfo = splitHostAndSvcFromAddr( addrArray[i] ) ;
      if ( -1 == keepHosts.indexOf( nodeInfo[0] ) ) {
         continue ;
      }
      newArray.push( addrArray[i] ) ;
   }
   return newArray.join( "," ) ;
}

/* *****************************************************************************
@discription: 获取本机IP地址列表
@exceptLo: 是否排除回环地址，即：127.0.0.1
@author: Jianhui Xu
@return: string array
     ex: [ "127.0.0.1", "192.168.20.106" ]
***************************************************************************** */
function getHostIPs( exceptLo ) {
   if ( typeof( exceptLo ) == "undefined" ) {
      exceptLo = true ;
   }
   var retArray = new Array() ;
   var tmpInfo = System.getNetcardInfo() ;
   var obj = eval( '(' + tmpInfo + ')' ) ;
   var cardArray = obj["Netcards"] ;
   for ( var i = 0 ; i < cardArray.length ; ++i ) {
      if ( exceptLo && cardArray[i].Ip == "127.0.0.1" ) {
         continue ;
      }
      retArray.push( cardArray[i].Ip ) ;
   }
   return retArray ;
}

/* *****************************************************************************
@discription: 校验参数正确性
@author: Jianhui Xu
@return: true/false
***************************************************************************** */
function checkArgs() {
   if ( CURSUB != 1 && CURSUB != 2 ) {
      println( "CURSUB must be 1 or 2" ) ;
      return false ;
   }
   if ( CUROPR != "init" && CUROPR != "split" && CUROPR != "merge"
        && CUROPR != "detachGroupNode" && CUROPR != "attachGroupNode" ) {
      println( "CUROPR must be 'init', 'split' or 'merge' " ) ;
      return false ;
   }

   if ( COORDADDR.length == 0 ) {
      println( "COORDADDR is empty, you should config it first" ) ;
      return false ;
   }
   /* Value args */
   if ( CURSUB == 1 ) {
      CURHOSTS = SUB1HOSTS ;
   } else {
      CURHOSTS = SUB2HOSTS ;
   }
   if ( CURHOSTS.length == 0 ) {
      println( "CURHOSTS is empty, should need to set SUB1HOSTS and SUB2HOSTS" ) ;
      return false ;
   }

   ALLHOSTS = mergeArrayWithoutRepeat( SUB1HOSTS, SUB2HOSTS ) ;

   /* Check current host is wether in CURHOSTS */
   var curHost = System.getHostName() ;
   if ( -1 == CURHOSTS.indexOf( curHost ) ) {
      // check ip is in the CURHOSTS
      var ipArray = getHostIPs( true ) ;
      var ipIn = false ;
      for ( var i = 0 ; i < ipArray.length ; ++i ) {
         if ( -1 != CURHOSTS.indexOf( ipArray[i] ) ) {
            ipIn = true ;
            break ;
         }
      }
      if ( false == ipIn ) {
         println( "Current host[" + curHost + "] is not in CURHOSTS: " + CURHOSTS + ". Make sure CURSUB value is right?" ) ;
      }
      return false ;
   }
   return true ;
}

/* *****************************************************************************
@discription: 检查 sequoiadb 版本是否高于 NEW_VERSION
@author: QinCheng Yang
@return: true/false
***************************************************************************** */
function checkSdbVersion()
{
   var cmd = new Cmd() ;
   try{
      version_obj = cmd.run(SDBSHELL, "--version") ;
      version_arr = version_obj.split("\n")[0].split(":")[1].trim().split(".");
      divide_arr = VERSION_DIVIDE.split(".") ;
      for ( i in divide_arr ){
         if ( divide_arr[i] > version_arr[i]){
            return false ;
         }else if ( divide_arr[i] < version_arr[i] ) {
            return true ;
         }else if ( divide_arr[i] == version_arr[i] ) {
            continue ;
         }
      }
   }catch( e ) {
       println( "Failed to check sequoiadb version, Info:" + e + "(" + getLastErrMsg() + ")" ) ;
       return false ;
   }
   return true ;
}

/* *****************************************************************************
@discription: 通过 Remote 去校验每台机器的环境配置，仅支持 NEW_VERSION 及以上版本
@hosts : 字符串数组（机器列表信息）
@author: QinCheng Yang
@return: true/false
***************************************************************************** */
function checkHostsEvnNew ( hosts ) {
   var checkFiles = [ SDBSTART, SDBSTOP, SDBLIST, SDBSHELL, CONFLOCAL ] ;
   for ( i in hosts ) {
      var remoteObj ;
      try {
         var sdbcmSvc = Oma.getAOmaSvcName( hosts[i] ) ;
         remoteObj = new Remote( hosts[i], sdbcmSvc ) ;
      } catch ( e ) {
         println( "Remote " + hostname + " failed, error info: " + e + "(" + getLastErrMsg() + ")" ) ;
         return false ;
      }
      try {
         var cmd = remoteObj.getCmd() ;
         for ( j in checkFiles ) {
            cmd.run( 'ls', checkFiles[j] ) ;
         }
      } catch ( e ) {
         println( "Check file[" + checkFiles[j] + "] in " + hosts[i] + " failed: " + e + "(" + getLastErrMsg() + ")" ) ;
         return false ;
      } finally {
        remoteObj.close() ;
      }
   }
   return true ;
}

/* *****************************************************************************
@discription: 校验每台机器的环境配置
@hosts : 字符串数组（机器列表信息）
@author: Jianhui Xu
@return: true/false
***************************************************************************** */
function checkHostsEvnOld( hosts ) {
   var checkFiles = [ SDBSTART, SDBSTOP, SDBLIST, SDBSHELL, CONFLOCAL ] ;

   for ( i in hosts ) {
      var ssh ;
      try {
         ssh = new Ssh( hosts[i], USERNAME,  PASSWD ) ;
      } catch( e ) {
         println( "SSH to " + hosts[i] + " failed: " + e + "(" + getLastErrMsg() + ")" ) ;
         return false ;
      }

      try {
         /* Check Files */
         for ( j in checkFiles ) {
           ssh.exec( 'ls ' + checkFiles[j] ) ;
         }
      } catch( e ) {
         println( "Check file[" + checkFiles[j] + "] in " + hosts[i] + " failed: " + e + "(" + getLastErrMsg() + ")" ) ;
         ssh.close() ;
         return false ;
      }

      /* Close the ssh connection */
      ssh.close() ;
   }
   return true ;
}

/* *****************************************************************************
@discription: 校验每台机器的环境配置
@hosts : 字符串数组（机器列表信息）
@author: QinCheng Yang
@return: true/false
***************************************************************************** */
function checkHostsEvn( hosts ) {
   if ( NEW_VERSION ) {
      return checkHostsEvnNew( hosts ) ;
   } else {
      return checkHostsEvnOld( hosts ) ;
   }
}

/* *****************************************************************************
@discription: 将Catalog所有Group信息保存到filename中
@coordAddr : coord address(string)
@filename : string
@author: Jianhui Xu
@return: true / false
***************************************************************************** */
function saveGroupsInfo( coordAddr, filename ) {
   var db ;
   var file ;
   var number = 0 ;
   try {
      var addrArray = splitHostAndSvcFromAddr( coordAddr ) ;
      db = new Sdb( addrArray[0], addrArray[1], SDBUSERNAME, SDBPASSWD ) ;
   } catch ( e ) {
      println( "Connect to " + coordAddr + " failed: " + e + "(" + getLastErrMsg() + ")" ) ;
      return false ;
   }

   try {
      file = new File( filename ) ;
   } catch( e ) {
      println( "Create or open file[" + filename + "] failed: " + e + "(" + getLastErrMsg() + ")" ) ;
      db.close() ;
      return false ;
   }

   try {
      var cursor = db.listReplicaGroups() ;
      while( cursor.next() ) {
         if ( 0 == number ) {
            file.write( "[\n" ) ;
         } else {
            file.write( ",\n" ) ;
         }
         file.write( cursor.current().toString() ) ;
         ++number ;
      }
      file.write( "\n]\n" ) ;
   } catch ( e ) {
      println( "Write info to  file[" + filename + "] failed: " + e + "(" + getLastErrMsg() + ")" ) ;
      db.close() ;
      file.close() ;
      return false ;
   }

   db.close() ;
   file.close() ;
   return true ;
}

/* *****************************************************************************
@discription: 从filename中读取保存的GroupsInfo，并转换成 [obj,obj]返回
@filename : string
@author: Jianhui Xu
@return: obj array
  error: throw exception
***************************************************************************** */
function readGroupsInfo( filename ) {
   var file ;
   var text ;
   var objarray = new Array() ;

   try {
      file = new File( filename ) ;
   } catch( e ) {
      println( "Open file[" + filename + "] failed: " + e + "(" + getLastErrMsg() + ")" ) ;
      throw e ;
   }

   try {
      text = file.read( READSIZE ) ;
      var pos = text.indexOf( "%%%%" ) ;
      if ( -1 != pos ) {
         text = text.substring( 0, pos ) ;
      }
      objarray = eval( '(' + text + ')' ) ;
   } catch ( e ) {
      println( "Read info from  file[" + filename + "] failed: " + e + "(" + getLastErrMsg() + ")" ) ;
      file.close() ;
      throw e ;
   }

   file.close() ;
   return objarray ;
}

/* *****************************************************************************
@discription: 根据active设置datacenter的readonly属性
              active:true  -> readonly:false
              active:false -> readonly:true
@cataAddr : catalog address(string)
@newAddrLine : string, ex : vmsvr2-suse-x64-1:30003,vmsvr2-suse-x64-1:30013
@active: true/false
@author: Jianhui Xu
@return: true / false
***************************************************************************** */
function updateDCInfoInCatalog( cataAddr, newAddrLine, active ) {
   var db ;
   var isReadOnly = false ;

   if ( !active ) { isReadOnly = true ; }
   try {
      var addrArray = splitHostAndSvcFromAddr( cataAddr ) ;
      db = new Sdb( addrArray[0], addrArray[1], SDBUSERNAME, SDBPASSWD ) ;
   } catch ( e ) {
      println( "Connect to " + cataAddr + " failed: " + e + "(" + getLastErrMsg() + ")" ) ;
      return false ;
   }

   try {
      db.SYSINFO.SYSDCBASE.update( {$set:{Readonly:isReadOnly, 'DataCenter.Address':newAddrLine} }, {Type:"GLOBAL"} ) ;
   } catch ( e ) {
      println( "Update readonly to " + isReadOnly + " in " + cataAddr + " failed: " + e + "(" + getLastErrMsg() + ")" ) ;
      db.close() ;
      return false ;
   }

   db.close() ;
   return true ;
}

/* *****************************************************************************
@discription: 将Catalog所有Group信息中非keepHosts的Host信息删除
@cataAddr : catalog address(string)
@keepHosts : hostname数组
@author: Jianhui Xu
@return: true / false
***************************************************************************** */
function updateGroupsInCatalog( cataAddr, keepHosts ) {
   var db ;

   try {
      var addrArray = splitHostAndSvcFromAddr( cataAddr ) ;
      db = new Sdb( addrArray[0], addrArray[1], SDBUSERNAME, SDBPASSWD ) ;
   } catch ( e ) {
      println( "Connect to " + cataAddr + " failed: " + e + "(" + getLastErrMsg() + ")" ) ;
      return false ;
   }

   var tmpGroupInfo ;
   try {
      tmpGroupInfo = db.SYSCAT.SYSNODES.find().toArray() ;
   } catch ( e ) {
      println( "Get groups info failed: " + e + "(" + getLastErrMsg() + ")" ) ;
      db.close() ;
      return false ;
   }

   /* filter group by hostname */
   for ( var i = 0 ; i < tmpGroupInfo.length ; ++i ) {
      var tmpGroupObj = eval( "(" + tmpGroupInfo[i] + ")" ) ;
      var tmpFieldGroup = tmpGroupObj["Group"] ;
      var newFieldGroup = new Array() ;
      var kickNum = 0 ;

      for ( var j = 0 ; j < tmpFieldGroup.length ; ++j ) {
         var tmpHostObj = tmpFieldGroup[ j ] ;
         /* Find in the keepHosts */
         if ( -1 != keepHosts.indexOf( tmpHostObj.HostName ) ) {
           newFieldGroup.push( tmpHostObj ) ;
         } else {
            ++kickNum ;
            println( "Kick out host[" + tmpHostObj.HostName + "] from group[" + tmpGroupObj.GroupName + "]" ) ;
         }
      }

      /* Update to db */
      if ( kickNum > 0 ) {
         try {
            db.SYSCAT.SYSNODES.update( { '$set':{ 'Group' : newFieldGroup } }, { '_id': tmpGroupObj['_id'] } ) ;
            println( "Update kicked group[" + tmpGroupObj.GroupName + "] to " + cataAddr + " succeed" ) ;
         } catch ( e ) {
            println( "Update kicked group info to " + cataAddr + " failed: " + e + "(" + getLastErrMsg() + ")" ) ;
            db.close() ;
            return false ;
         }
      } else {
         println( "Group[" + tmpGroupObj.GroupName + "] not change in " + cataAddr ) ;
      }
   }

   db.close() ;
   return true ;
}

/* *****************************************************************************
@discription: 恢复Catalog所有Group信息
@cataAddr : catalog address(string)
@groupsArray : obj array
@author: Jianhui Xu
@return: true / false
***************************************************************************** */
function restoreGroupsInCatalog( cataAddr, groupsArray ) {
   var db ;

   try {
      var addrArray = splitHostAndSvcFromAddr( cataAddr ) ;
      db = new Sdb( addrArray[0], addrArray[1], SDBUSERNAME, SDBPASSWD ) ;
   } catch ( e ) {
      println( "Connect to " + cataAddr + " failed: " + e + "(" + getLastErrMsg() + ")" ) ;
      return false ;
   }

   /* filter group by hostname */
   for ( var i = 0 ; i < groupsArray.length ; ++i ) {
      var tmpGroupObj = groupsArray[i] ;
      var tmpFieldGroup = tmpGroupObj["Group"] ;

      /* Update to db */
      try {
         db.SYSCAT.SYSNODES.update( { '$set':{ 'Group' : tmpFieldGroup } }, { 'GroupName': tmpGroupObj['GroupName'] } ) ;
         println( "Restore group[" + tmpGroupObj.GroupName + "] to " + cataAddr + " succeed" ) ;
      } catch ( e ) {
         println( "Restore group info to " + cataAddr + " failed: " + e + "(" + getLastErrMsg() + ")" ) ;
         db.close() ;
         return false ;
      }
   }

   db.close() ;
   return true ;
}

/* *****************************************************************************
@discription: 获取远端节点的配置，仅支持 NEW_VERSION 及以上版本
@address : node address(string), ex: "192.168.20.106:20000"
@author: Jianhui Xu
@return: cofig obj
@error: with exception
***************************************************************************** */
function getConfigObjNew( address ) {
   var addrArray = splitHostAndSvcFromAddr( address ) ;
   var conffile = CONFLOCAL + "/" + addrArray[1] + "/sdb.conf" ;
   var obj ;
   /* New version */
   var oma ;
   try {
      var sdbcmSvc = Oma.getAOmaSvcName( addrArray[0] ) ;
      oma = new Oma( addrArray[0], sdbcmSvc ) ;
   } catch ( e ) {
      println( "Failed to connect sdbcm in " + addrArray[0] + ", error info: " + e + "(" + getLastErrMsg() + ")" ) ;
      throw e ;
   }
   try {
      var tmpString = oma.getOmaConfigs( conffile ).toObj() ;
      obj = eval( tmpString ) ;
      oma.close() ;
   } catch ( e ) {
      println( "Get config object from " + address + " failed: " + e + "(" + getLastErrMsg() + ")" ) ;
      oma.close() ;
      throw e ;
   }
   return obj ;

}

/* *****************************************************************************
@discription: 获取远端节点的配置
@address : node address(string), ex: "192.168.20.106:20000"
@author: Jianhui Xu
@return: cofig obj
  error: with exception
***************************************************************************** */
function getConfigObj( address ) {
   var addrArray = splitHostAndSvcFromAddr( address ) ;
   var ssh ;
   var obj ;
   /* Ssh to remote host */
   try {
      ssh = new Ssh( addrArray[0], USERNAME, PASSWD ) ;
   } catch ( e ) {
      println( "Ssh to " + addrArray[0] + " failed: " + e + "(" + getLastErrMsg() + ")" ) ;
      throw e ;
   }
   /* Begin to get config object from remote host */
   try {
      var conffile = CONFLOCAL + "/" + addrArray[1] + "/sdb.conf" ;
      var tmpString = ssh.exec( SDBSHELL + ' -s "Oma.getOmaConfigs( ' + "'" + conffile + "'" + ' )" ' ) ;
      ssh.exec( SDBSHELL + ' -s quit ' ) ;
      ssh.close() ;
      obj = eval( '(' + tmpString + ')' ) ;
   } catch ( e ) {
      println( "Get config object from " + address + " failed: " + e + "(" + getLastErrMsg() + ")" ) ;
      ssh.close() ;
      throw e ;
   }

   return obj ;
}

/* *****************************************************************************
@discription: 保存配置到远端节点中，仅支持 NEW_VERSION 及以上版本
@address : node address(string), ex: "192.168.20.106:20000"
@obj : config object
@author: Jianhui Xu
@return: true/false
***************************************************************************** */
function saveConfigObjNew( address, obj ) {
   var addrArray = splitHostAndSvcFromAddr( address ) ;
   var oma ;
   try {
      var conffile = CONFLOCAL + "/" + addrArray[1] + "/sdb.conf" ;
      var omaSvc = Oma.getAOmaSvcName( addrArray[0] ) ;
      oma = new Oma( addrArray[0], omaSvc ) ;
      oma.setOmaConfigs( obj, conffile ) ;
      oma.reloadConfigs() ;
   } catch ( e ) {
      println( "Save config object to " + address + " failed: " + e + "(" + getLastErrMsg() + ")" ) ;
      return false ;
   } finally {
       oma.close() ;
   }
   return true ;
}

/* *****************************************************************************
@discription: 保存配置到远端节点中
@address : node address(string), ex: "192.168.20.106:20000"
@obj : config object
@author: Jianhui Xu
@return: true/false
***************************************************************************** */
function saveConfigObj( address, obj ) {
   var addrArray = splitHostAndSvcFromAddr( address ) ;
   var ssh ;
   /* Ssh to remote host */
   try {
      ssh = new Ssh( addrArray[0], USERNAME, PASSWD ) ;
   } catch ( e ) {
      println( "Ssh to " + addrArray[0] + " failed: " + e + "(" + getLastErrMsg() + ")" ) ;
      return false ;
   }
   /* Begin to save config object to remote host */
   try {
      var conffile = CONFLOCAL + "/" + addrArray[1] + "/sdb.conf" ;
      var objstring = JSON.stringify( obj ) ;
      objstring = objstring.replace( /\"/g, "\\\"" ) ;
      ssh.exec( SDBSHELL + ' -s " var obj = ' +  objstring + ' " ; ' ) ;
      ssh.exec( SDBSHELL + ' -s "Oma.setOmaConfigs( obj, ' + "'" + conffile + "'" + ' ) ; " ' ) ;
      ssh.exec( SDBSHELL + ' -s quit ' ) ;
      ssh.close() ;
   } catch ( e ) {
      println( "Save config object to " + address + " failed: " + e + "(" + getLastErrMsg() + ")" ) ;
      ssh.close() ;
      return false ;
   }

   return true ;
}

/* *****************************************************************************
@discription: 更新节点集的配置
@nodesArray : node address array
@key :
@value :
@author: Jianhui Xu
@return: true/false
***************************************************************************** */
function updateNodesConfig( nodesArray, key, value ) {
   for ( var i = 0 ; i < nodesArray.length ; ++i ) {
      var obj ;
      try {
         if ( NEW_VERSION ) {
            obj = getConfigObjNew( nodesArray[i] ) ;
         } else {
            obj = getConfigObj( nodesArray[i] ) ;
         }
      } catch ( e ) {
         println( "Get config obj from " + nodesArray[i] + " failed: " + e ) ;
         return false ;
      }
      if ( typeof( obj[ key ] ) == "undefined" || obj[key] != value ) {
         obj[key] = value ;
         var rc = true ;
         if ( NEW_VERSION ) {
            rc = saveConfigObjNew( nodesArray[i], obj ) ;
         } else {
            rc = saveConfigObj( nodesArray[i], obj ) ;
         }
         if ( !rc ) {
            println( "Save config obj to " + nodesArray[i] + " failed" ) ;
            return false ;
         }
      }
   }
   return true ;
}

/* *****************************************************************************
@discription: 动态生效节点配置
@nodesArray : node address array
@ignoreErrArray : ignore error code array
@author: Jianhui Xu
@return: true/false
***************************************************************************** */
function reloadNodesConf( nodesArray, ignoreErrArray ) {
   for ( var i = 0 ; i < nodesArray.length ; ++i ) {
      var addrArray = splitHostAndSvcFromAddr( nodesArray[i] ) ;
      try {
         var db = new Sdb( addrArray[0], addrArray[1], SDBUSERNAME, SDBPASSWD ) ;
         db.reloadConf( { Global : false } ) ;
         db.close() ;
      } catch ( e ) {
         /* When the error in ignoredErrArray, make sure is succeed */
         if ( -1 == ignoreErrArray.indexOf( e ) ) {
            println( "Reload config from " +  nodesArray[i] + " failed: " + e + "(" + getLastErrMsg() + ")" ) ;
            return false ;
         }
      }
   }

   return true ;
}

/* *****************************************************************************
@discription: 让所有组重选主
@coordAddr : coord address(string)
@author: Jianhui Xu
@return: true / false
***************************************************************************** */
function reelectAllGroups( coordAddrs ) {
   var db ;
   var get = false ;
   for ( var i = 0 ; i < coordAddrs.length ; ++i ) {
      try {
         var addrArray = splitHostAndSvcFromAddr( coordAddrs[i] ) ;
         db = new Sdb( addrArray[0], addrArray[1], SDBUSERNAME, SDBPASSWD ) ;
      } catch ( e ) {
         println( "Connect to " + coordAddrs[i] + " failed: " + e + "(" + getLastErrMsg() + ")" ) ;
         continue ;
      }

      /* Get replica groups */
      try {
         var cursor = db.listReplicaGroups() ;
         while ( cursor.next() ) {
            var obj = eval( '(' + cursor.current().toString() + ')' ) ;
            if ( 1 != obj["GroupID"] && obj["GroupID"] < 1000 ) {
               continue ;
            }
            rg = db.getRG( obj["GroupName"] ) ;
            try {
               rg.reelect( { Seconds : 20 } ) ;
            } catch ( e ) {
               /* Ignore all error */
            }
         }
         get = true ;
      } catch ( e ) {
         println( "Reelect all groups failed: " + e +  "(" + getLastErrMsg() + ")" ) ;
      }

      db.close() ;
      break ;
   }

   return get ;
}

/* *****************************************************************************
@discription: 将catalog address line保存到文件
@coordAddr : coord address(string)
@filename : string
@author: Jianhui Xu
@return: true / false
***************************************************************************** */
function saveCatalogAddrLine( coordAddr, filename ) {
   var addrstring = "" ;

   try {
      var tmpObj ;
      if ( NEW_VERSION ) {
         tmpObj = getConfigObjNew( coordAddr ) ;
      } else {
         tmpObj = getConfigObj( coordAddr ) ;
      }
      addrstring = tmpObj["catalogaddr"] ;
   } catch ( e ) {
      println( "Get catalog address line from " + coordAddr + " failed: " + e + "(" + getLastErrMsg() + ")" ) ;
      return false ;
   }

   try {
      file = new File( filename ) ;
      file.seek( 0, 'e' ) ;
   } catch( e ) {
      println( "Create or open file[" + filename + "] failed: " + e + "(" + getLastErrMsg() + ")" ) ;
      db.close() ;
      return false ;
   }

   try {
      file.write( "\n%%%%" + addrstring + "%%%%\n" ) ;
   } catch ( e ) {
      println( "Write catalog address line to  file[" + filename + "] failed: " + e + "(" + getLastErrMsg() + ")" ) ;
      file.close() ;
      return false ;
   }

   file.close() ;
   return true ;
}

/* *****************************************************************************
@discription: 从filename中读取保存的catalog address line，并转换成string返回
@filename : string
@author: Jianhui Xu
@return: string
  error: throw exception
***************************************************************************** */
function readCatalogAddressLine( filename ) {
   var file ;
   var text ;

   try {
      file = new File( filename ) ;
   } catch( e ) {
      println( "Open file[" + filename + "] failed: " + e + "(" + getLastErrMsg() + ")" ) ;
      throw e ;
   }

   try {
      text = file.read( READSIZE ) ;
      var pos = text.indexOf( "%%%%" ) ;
      if ( -1 != pos ) {
         text = text.substring( pos + 4 ) ;
         var posend = text.indexOf( "%%%%" ) ;
         if ( -1 != posend ) {
            text = text.substring(0, posend ) ;
         }
      }
   } catch ( e ) {
      println( "Read info from  file[" + filename + "] failed: " + e + "(" + getLastErrMsg() + ")" ) ;
      file.close() ;
      throw e ;
   }

   file.close() ;
   return text ;
}


/* *****************************************************************************
@discription: 重启指定节点，仅支持 NEW_VERSION 及以上版本
@hostname : string typ, ex: '192.168.10.106'
@svcnames : node svcname, (number|string|Array), ex: "11800,11810"
@author: QinCheng Yang
@return: true/false
***************************************************************************** */
function restartNodeWithOma( hostname, svcnames ){
   if ( !NEW_VERSION ){
       return false ;
   }
   var oma ;
   try {
      var sdbcmSvc = Oma.getAOmaSvcName( hostname ) ;
      oma = new Oma( hostname, sdbcmSvc ) ;
   } catch ( e ) {
      println( "Failed to connect sdbcm in " + hostname + ", error info: " + e + "(" + getLastErrMsg() + ")" ) ;
      throw e ;
   }

   try {
      var svcname_arr ;
      if ( typeof svcnames == "number" ) {
         svcname_arr = [ svcnames ] ;
      }else if ( typeof svcnames == "string") {
         svcname_arr = svcnames.split(",") ;
      }else if ( svcnames instanceof Array) {
         svcname_arr = svcnames ;
      }

      oma.stopNodes( svcname_arr ) ;
      println( "Stop " + svcnames + " succeed in " + hostname ) ;

      oma.startNodes( svcname_arr ) ;
      println( "Restart " + svcname_arr + " succeed in " + hostname ) ;
      oma.close() ;
   } catch ( e ) {
      oma.close() ;
      println( "Failed to start " + svcname_arr + " in " + hostname  + ", error info: " + e + "(" + getLastErrMsg() + ")" ) ;
      throw e ;
   }
   return true ;
}

/* *****************************************************************************
@discription: 使用 sdbstop/sdbstart 工具重启节点，并且可以指定具体的重启参数
@nodeAddr : node address( string ), ex: '192.168.10.106:30000'
@options: node configure, ex: "--role data"
@author: QinCheng Yang
@return: true/false
***************************************************************************** */
function restartNodeWithCmd( nodeAddr, options ) {
   var addrArray = splitHostAndSvcFromAddr( nodeAddr ) ;
   var remoteObj ;
   try {
      var sdbcmSvc = Oma.getAOmaSvcName( addrArray[0] ) ;
      remoteObj = new Remote( addrArray[0], sdbcmSvc ) ;
   } catch ( e ) {
      println( "Remote " + addrArray[0] + " failed, error info: " + e + "(" + getLastErrMsg() + ")" ) ;
      return false ;
   }
   var msg = "ReStart " + addrArray[1] ;
   try {
      var cmd = remoteObj.getCmd() ;
      cmd.run( SDBSTOP + " -p " + addrArray[1] ) ;
      println( "Stop " + addrArray[1] + " succeed in " + addrArray[0] ) ;
      if ( typeof options == "string" && options != "" ) {
         cmd.run( SDBSTART + " -p " + addrArray[1] + ' -o \"' + options + "\"" ) ;
         msg += " with " + options ;
      }else {
         cmd.run( SDBSTART + " -p " + addrArray[1] ) ;
      }
      println( msg + " succeed in " + addrArray[0] ) ;
   } catch ( e ) {
      println( msg + " failed: " + e + "(" + getLastErrMsg() + ")" ) ;
      return false ;
   } finally {
      remoteObj.close() ;
   }
   return true ;
}

/* *****************************************************************************
@discription: 将Catalog节点改为standalone模式启动，仅支持 NEW_VERSION 及以上版本
@cataAddr : catalog address( string ), ex: '192.168.10.106:30000'
@author: QinCheng Yang
@return: true/false
***************************************************************************** */
function change2StandaloneNew( cataAddr ) {
   var options = "--role standalone" ;
   return restartNodeWithCmd( cataAddr, options ) ;
}

/* *****************************************************************************
@discription: 将Catalog节点改为standalone模式启动
@cataAddr : catalog address( string ), ex: '192.168.10.106:30000'
@author: Jianhui Xu
@return: true/false
***************************************************************************** */
function change2StandaloneOld( cataAddr ) {
   var addrArray = splitHostAndSvcFromAddr( cataAddr ) ;

   var ssh ;
   /* Stop the node and start by standalone */
   try {
      ssh = new Ssh( addrArray[0], USERNAME, PASSWD ) ;
   } catch ( e ) {
      println( "Ssh to " + addrArray[0] + " failed: " + e + "(" + getLastErrMsg() + ")" ) ;
      return false ;
   }

   try {

      ssh.exec( SDBSTOP + " -p " + addrArray[1] ) ;
      println( "Stop " + addrArray[1] + " succeed in " + addrArray[0] ) ;
      var cmdline = SDBSTART + " -p " + addrArray[1] + ' -o "--role standalone" ' ;
      // println( "CommandLine: " + cmdline ) ;
      ssh.exec( cmdline ) ;
      println( "Start " + addrArray[1] + " by standalone succeed in " + addrArray[0] ) ;
   } catch ( e ) {
      println( "Change " + cataAddr + " to standalone failed: " + e + "(" + getLastErrMsg() + ")" ) ;
      ssh.close() ;
      return false ;
   }
   ssh.close() ;
   return true ;
}

/* *****************************************************************************
@discription: 将Catalog节点改为Catalog角色模式启动
@cataAddr : catalog address( string ), ex: '192.168.10.106:30000'
@author: QinCheng Yang
@return: true/false
***************************************************************************** */
function change2Standalone( cataAddr ) {
   if ( NEW_VERSION ) {
      /* In new version, there will write the parameters '--role standalone' to the 
      node configuration file. */
      return change2StandaloneNew( cataAddr ) ;
   } else {
      return change2StandaloneOld( cataAddr ) ;
   }
}

/* *****************************************************************************
@discription: 重启所有主机中的所有SequoiaDB节点
@hostnameArr : String Array[]
@author: Jiaming Wu
@return: true/false
***************************************************************************** */
function restartAllHostNode( hostnameArr ) {

   var hostNumber = hostnameArr.length ;
   var svcnameArr = new Array( hostNumber ) ;
   var nodeInfo = new Array( hostNumber ) ;

   /* Get svcname  */
   for ( var j = 0 ; j < hostNumber ; ++j ) {
      svcnameArr[ j ] = Oma.getAOmaSvcName( hostnameArr[ j ] ) ;
   }

   /* Restart all host */
   var restartJob = new Array( hostNumber ) ;
   for ( var j = 0 ; j < hostNumber ; ++j ) {
      try
      {
         var oma = new Oma( hostnameArr[ j ], svcnameArr[ j ] ) ;
         nodeInfo[ j ] = oma.listNodes() ;
      }
      catch( e )
      {
         println( "List " + hostnameArr[ j ] + " nodes failed: " + e + "(" + getLastErrMsg() + ")" ) ;
         return false ;
      }

      /* New version */
      if ( NEW_VERSION ) {
         var remoteObj = new Remote( hostnameArr[j], svcnameArr[j] ) ;
         try {
            var cmd = remoteObj.getCmd() ;
            var cmdStr = SDBSTOP + " -t all && " + SDBSTART  + " -t all" ;
            var retStr = cmd.start( cmdStr, "", 1, 0 ).toString() ;
            var pid = retStr.split( "\n" )[ 0 ] ;
            restartJob[ j ] = { "pid" : "" + pid } ;
         } catch ( e ) {
            println( "Restart " + hostnameArr[ j ] + " node failed: " + e + "(" + getLastErrMsg() + ")" ) ;
            return false ;
         } finally {
            remoteObj.close() ;
         }
      } else {
         try {
            ssh = new Ssh( hostnameArr[ j ], USERNAME, PASSWD ) ;
         } catch ( e ) {
            println( "Ssh to " + hostnameArr[ j ] + " failed: " + e + "(" + getLastErrMsg() + ")" ) ;
            return false ;
         }
         try
         {
            var retStr = ssh.exec( SDBSHELL + ' -s \'var cmd = new Cmd(); cmd.start( "'
                                   + SDBSTOP + ' -t all && ' + SDBSTART + ' -t all", "", 1, 0  ) ; \' ' ) ;
            var pid = retStr.split( "\n" )[ 0 ] ;
            restartJob[ j ] = { "pid" : "" + pid } ;
            ssh.close() ;
         }
         catch( e )
         {
            ssh.close() ;
            println( "Restart " + hostnameArr[ j ] + " node failed: " + e + "(" + getLastErrMsg() + ")" ) ;
            return false ;
         }
      }
   }

   /* Check whether all backgroup stop job have been completed  */
   var sysArr = new Array( hostNumber ) ;
   var finishFlagArr = new Array( hostNumber ) ;
   var finishNumber = 0 ;
   var remoteArr = new Array( hostNumber ) ;

   for( var j = 0; j < hostNumber; ++j ) {
      /* Get remote obj */
      try
      {
         remoteArr[ j ] = new Remote( hostnameArr[ j ], svcnameArr[ j ] ) ;
      }
      catch( e )
      {
         println( "Get " + hostnameArr[ j ] + " remote obj failed: " + e + "(" + getLastErrMsg() + ")" ) ;
         return false ;
      }
      finishFlagArr[ j ] = false ;
      sysArr[ j ] = remoteArr[ j ].getSystem() ;
   }
   while( finishNumber < hostNumber )
   {
      for( var j = 0; j < hostNumber; ++j )
      {
         if( finishFlagArr[ j ] != true )
         {
            var listProc = sysArr[ j ].listProcess( {}, restartJob[ j ] ) ;
            if( listProc.size() == 0 )
            {
               finishFlagArr[ j ] = true ;
               finishNumber++ ;
            }
            else
            {
               var cmdInfo = listProc[ 0 ].toObj()[ "cmd" ];
               // Ignore zombie process
               if ( undefined != cmdInfo && cmdInfo.indexOf( "<defunct>" ) != -1 ) {
                  finishFlagArr[ j ] = true ;
                  finishNumber++ ;
               }
            }
         }
      }
      sleep( 500 ) ;
   }
   /* Close remote obj */
   for ( i in remoteArr ) {
      try {
         remoteArr[i].close() ;
      } catch ( e ) {
         continue ;
      }
   }

   // Compare starttime to determine whether the node have been restart
   for ( var j = 0 ; j < hostNumber ; ++j ) {
      var currentNodeInfo ;
      var localNodeInfo ;
      try
      {
         var oma = new Oma( hostnameArr[ j ], svcnameArr[ j ] ) ;
         currentNodeInfo = oma.listNodes() ;
         localNodeInfo = oma.listNodes( { mode: "local" } ) ;
      }
      catch( e )
      {
         println( "Failed to get cluster node info" ) ;
         return false ;
      }
      if( currentNodeInfo.size() != localNodeInfo.size() )
      {
         println( "Start " + hostnameArr[ j ] + " node failed" ) ;
         return false ;
      }

      var nodeInfoArray = [] ;
      while ( true ) {
         var bs = nodeInfo[ j ].next();
         if ( ! bs ) break ;
         nodeInfoArray.push ( bs.toObj() ) ;
      }

      while( currentNodeInfo.more() ) {
         var node = currentNodeInfo.next().toObj() ;
         var filter = new _Filter( { "svcname": node.svcname } ) ;
         var result = filter.match( nodeInfoArray ) ;
         if( result.size() != 1 )
         {
            continue ;
         }
         if( node.starttime == result.next().toObj().starttime )
         {
            println( "Stop " + hostnameArr[ j ] + " node failed" ) ;
            return false ;
         }
      }
      println( "Restart all nodes succeed in " + hostnameArr[ j ] ) ;
   }
   return true ;
}

/* *****************************************************************************
@discription: 准备运行时的环境信息
@filename: string
@keepHosts : array
@author: Jianhui Xu
@return: true/false
***************************************************************************** */
function prepareEnv( filename, keepHosts ) {
   if ( !File.exist( filename ) ) {
      println( "File[" + filename + "] not exist, you should init with the file first" ) ;
      return false ;
   }

   var objarray ;
   try {
      objarray = readGroupsInfo( filename ) ;
   } catch ( e ) {
      println( "Read groups info from file[" + filename + "] failed: " + e ) ;
      return false ;
   }
   try {
      CATAADDRLINE = readCatalogAddressLine( filename ) ;
   } catch( e ) {
      println( "Read catalog address line failed: " + e ) ;
      return false ;
   }
   /* Parse nodes */
   var nodesarray ;
   try {
      nodesarray = parseGroupNodes( objarray, keepHosts ) ;
   } catch ( e ) {
      println( "Parse group nodes failed: " + e ) ;
      return false ;
   }
   CURCATAS = nodesarray[0] ; // catalog
   CURDATAS = nodesarray[1] ;  // data
   CURCOORDS = mergeArrayWithoutRepeat( nodesarray[2], makeNodesArrayWithKeepHosts( COORDADDR, keepHosts ) ) ;
   if ( CURCATAS.length == 0 ) {
      println( "Catalog nodes is empty" ) ;
      return false ;
   }
   return true ;
}

/* *****************************************************************************
@discription: 切分大集群，将非keepHosts的host从cataAddrs的保存组信息中踢除
@cataAddrs : string array, ex: [ '192.168.20.106:30000' ]
@keepHosts : string array, ex: [ 'test1', 'test2' ]
@active : true/false
@author: Jianhui Xu
@return: true/false
***************************************************************************** */
function splitCluster( cataAddrs, keepHosts, active ) {
   var newAddrLine = makeAddrLineWithKeepHosts( CATAADDRLINE, keepHosts ) ;

   /* 1. Update all node's addr--kick host */
   var allNodes = mergeArrayWithoutRepeat( CURCATAS, mergeArrayWithoutRepeat( CURDATAS, CURCOORDS ) ) ;
   if ( updateNodesConfig( allNodes, NODE_CONF_CATALOGADDR, newAddrLine ) ) {
      println( "Update all nodes' catalogaddr to " + newAddrLine + " succeed" ) ;
   } else {
      println( "Update all nodes' catalogaddr to " + newAddrLine + " failed" ) ;
      return false ;
   }

   for ( var i = 0 ; i < cataAddrs.length ; ++i  ) {
      /* 2. Change catalog to standalone */
      if ( change2Standalone( cataAddrs[ i ] ) ) {
         println( "Change " + cataAddrs[ i ] + " to standalone succeed"  ) ;
      } else {
         println( "Change " + cataAddrs[ i ] + " to standalone failed"  ) ;
         return false ;
      }
      /* 3. Update catalog groups info (kick the hosts) */
      if ( updateGroupsInCatalog( cataAddrs[i], keepHosts ) ) {
         println( "Update " + cataAddrs[ i ] + " catalog's info succeed"  ) ;
      } else {
         println( "Update " + cataAddrs[ i ] + " catalog's info failed"  ) ;
         return false ;
      }
      /* 4. Update catalog datacenter readonly property */
      if ( updateDCInfoInCatalog( cataAddrs[i], newAddrLine, active ) ) {
         println( "Update " + cataAddrs[i] + " catalog's readonly property succeed" ) ;
      } else {
         println( "Update " + cataAddrs[i] + " catalog's readonly property failed" ) ;
         return false ;
      }
   }

   /* 5. Restore catalog from standalone to normal*/
   if ( updateNodesConfig( cataAddrs, NODE_CONF_ROLE, "catalog" ) ) {
      println( "Restore [" + cataAddrs + "] to catalog succeed" ) ;
   } else {
      println( "Restore [" + cataAddrs + "] to catalog failed" ) ;
      return false ;
   }

   /* 6. Restart all keepHosts's nodes */
   if ( restartAllHostNode( keepHosts ) ) {
      println( "Restart all host nodes succeed"  ) ;
   } else {
      println( "Restart all host nodes failed"  ) ;
      return false ;
   }

   return true ;
}

/* *****************************************************************************
@discription: 获取集群所有主机地址
@coordAddrs : coord address( string[] ), ex: [ 'sdbserver:11810' ]
@author: Jiaming Wu
@return: true/false
***************************************************************************** */
function getClusterHostAddrs( coordAddrs )
{
   var db ;
   var hostAddrs = [] ;
   for ( var i = 0 ; i < coordAddrs.length ; ++i ) {
      try {
         var addrArray = splitHostAndSvcFromAddr( coordAddrs[i] ) ;
         db = new Sdb( addrArray[0], addrArray[1], SDBUSERNAME, SDBPASSWD ) ;
      } catch ( e ) {
         if( i >= coordAddrs.length ) {
            println( "There is no available coord" ) ;
            return false ;
         }
         println( "Connect to " + coordAddrs[i] + " failed: " + e + "(" + getLastErrMsg() + ")" ) ;
         continue ;
      }
      break ;
   }

   /* Get replica groups */
   try {
      var cursor = db.snapshot( SDB_SNAP_DATABASE, { RawData: true } ) ;
      while ( cursor.next() ) {
         var obj = eval( '(' + cursor.current().toString() + ')' ) ;
         if( hostAddrs.indexOf( obj.HostName ) == -1 ) {
            hostAddrs.push( obj.HostName ) ;
         }
      }
   } catch ( e ) {
      println( "Get all host failed: " + e +  "(" + getLastErrMsg() + ")" ) ;
      return false ;
   }
   db.close() ;
   return hostAddrs ;
}

/* *****************************************************************************
@discription: 初始化大集群，将大集群的组信息和Catalog Address line全部保存
@coordAddrs : string array, ex: [ '192.168.20.106:30000' ]
@filename : string
@active : true/false
@author: Jianhui Xu
@return: true/false
***************************************************************************** */
function initCluster( coordAddrs, filename, active ) {
   if ( File.exist( filename ) ) {
      println( "Already init. If you want to re-init, you should to remove the file: " + filename ) ;
      return false ;
   }
   var init = false ;
   for ( var i = 0 ; i < coordAddrs.length ; ++i  ) {
      try { File.remove( filename ) ; } catch( e ) {}
      if ( saveGroupsInfo( coordAddrs[i], filename ) ) {
         if ( saveCatalogAddrLine( coordAddrs[i], filename ) ) {
            init = true ;
            break ;
         }
      }
   }
   if ( false == init ) {
      println( "Init failed" ) ;
      try { File.remove( filename ) ; } catch( e ) {}
      return false ;
   }
   /* prepare env */
   if ( !prepareEnv( filename, CURHOSTS ) ) {
      println( "Prepare env failed" ) ;
      try { File.remove( filename ) ; } catch( e ) {}
      return false ;
   }
   if ( true == init && NEEDBROADCASTINITINFO ) {
      var copySuccess = true ;
      println( "Begin to copy init file to cluster hosts" ) ;
      var hostAddrs = getClusterHostAddrs( coordAddrs ) ;
      for( var i = 0; i < hostAddrs.length; ++i ) {
         if( hostAddrs[i] == System.getHostName() ) {
            continue ;
         }
         var dst_dir ;
         var omaSvc ;
         try {
            omaSvc = Oma.getAOmaSvcName( hostAddrs[i] ) ;
            var remote = new Remote( hostAddrs[i], omaSvc ) ;
            dst_dir = remote.getSystem().getEWD() ;
         } catch( e ) {
            copySuccess = false ;
            println( "Get " + hostAddrs[i] + " ewd failed: " + e +
                     "(" + getLastErrMsg() + ")" ) ;
            break ;
         }
         try {
            var dst_file = hostAddrs[i] + ":" + omaSvc  + "@" + dst_dir + "/../datacenter_init.info" ;
            File.scp( INITFILE, dst_file, true, 0700 ) ;
         } catch( e ) {
            copySuccess = false ;
            println( "Copy init file to " + hostAddrs[i] + " failed" + e +
                     "(" + getLastErrMsg() + ")" ) ;
            break ;
         }
         println( "Copy init file to " + hostAddrs[i] + " succeed" ) ;
      }
      if( false == copySuccess ) {
         println( "Failed" ) ;
         return false ;
      }
      println( "Done" ) ;
   }

   if( NEEDREELECT ) {
      /* and set weigth */
      var weigth = 10 ;
      if ( active ) {
         weigth = 100 ;
      }
      // update catalog and datanode
      print( "Begin to update catalog and data nodes' config..." ) ;
      if ( !updateNodesConfig( mergeArrayWithoutRepeat( CURCATAS, CURDATAS ), "weight", weigth ) ) {
         println( "Update catalog and data nodes' config failed" ) ;
         try { File.remove( filename ) ; } catch( e ) {}
         return false ;
      } else {
         println( "Done" ) ;
      }
      /* Reload all catalog and datanode */
      print( "Begin to reload catalog and data nodes' config..." ) ;
      if ( !reloadNodesConf( mergeArrayWithoutRepeat( CURCATAS, CURDATAS ), [ -15 ] ) ) {
         println( "Reload catalog and data nodes' config failed" ) ;
         try { File.remove( filename ) ; } catch( e ) {}
         return false ;
      } else {
         println( "Done" ) ;
      }

      /* Reelect all groups and ignore the error */
      print( "Begin to reelect all groups..." ) ;
      if ( !reelectAllGroups( coordAddrs ) ) {
         println( "WARNING: Reelect all groups failed" ) ;
      } else {
         println( "Done" ) ;
      }
   }

   return true ;
}

/* *****************************************************************************
@discription: 合并大集群，根据INIT保存的文件恢复Catalog所有组信息
@cataAddrs : string array, ex: [ '192.168.20.106:30000' ]
@keepHosts : string array, ex: [ 'test1', 'test2' ]
@filename : string
@active : true/false
@author: Jianhui Xu
@return: true/false
***************************************************************************** */
function mergeCluster( cataAddrs, keepHosts, filename, active ) {
   var groupsArray ;
   try {
      groupsArray = readGroupsInfo( filename ) ;
   } catch ( e ) {
      println( "Read groups info from file[" + filename + "] failed: " + e ) ;
      return false ;
   }

   for ( var i = 0 ; i < cataAddrs.length ; ++i  ) {
      /* 1. Change catalog node to standalone */
      if ( change2Standalone( cataAddrs[ i ] ) ) {
         println( "Change " + cataAddrs[ i ] + " to standalone succeed"  ) ;
      } else {
         println( "Change " + cataAddrs[ i ] + " to standalone failed"  ) ;
         return false ;
      }
      /* 2. Update catalog groups info (restore the hosts) */
      if ( restoreGroupsInCatalog( cataAddrs[i], groupsArray ) ) {
         println( "Restore " + cataAddrs[ i ] + " catalog's info succeed"  ) ;
      } else {
         println( "Restore " + cataAddrs[ i ] + " catalog's info failed"  ) ;
         return false ;
      }
      /* 3. Update catalog datacenter readonly property */
      if ( updateDCInfoInCatalog( cataAddrs[i], CATAADDRLINE, true ) ) {
         println( "Update " + cataAddrs[i] + " catalog's readonly property succeed" ) ;
      } else {
         println( "Update " + cataAddrs[i] + " catalog's readonly property failed" ) ;
         return false ;
      }
   }

   /* 4. Restore catalog from standalone to normal*/
   if ( updateNodesConfig( cataAddrs, NODE_CONF_ROLE, "catalog" ) ) {
      println( "Restore [" + cataAddrs + "] to catalog succeed" ) ;
   } else {
      println( "Restore [" + cataAddrs + "] to catalog failed" ) ;
      return false ;
   }

   /* 5. Update all nodes' addr */
   var allNodes = mergeArrayWithoutRepeat( CURCATAS, mergeArrayWithoutRepeat( CURDATAS, CURCOORDS ) ) ;
   if ( updateNodesConfig( allNodes, NODE_CONF_CATALOGADDR, CATAADDRLINE ) ) {
      println( "Update all nodes' catalogaddr to " + CATAADDRLINE + " succeed" ) ;
   } else {
      println( "Update all nodes' catalogaddr to " + CATAADDRLINE + " failed" ) ;
      return false ;
   }

   /* 6. Restart all keepHosts's nodes */
   if ( restartAllHostNode( keepHosts ) ) {
      println( "Restart all host nodes succeed"  ) ;
   } else {
      println( "Restart all host nodes failed"  ) ;
      return false ;
   }

   return true ;
}
/* *****************************************************************************
@discription: 重启节点
@author: Jiaming Wu
@return: true/false
***************************************************************************** */
function restartNode( nodeNameArray ) {
   var map = new Object() ;
   for( var i = 0; i < nodeNameArray.length; i++ ) {
      var addrArray = splitHostAndSvcFromAddr( nodeNameArray[i] ) ;
      var key = addrArray[0] ;
      var value = addrArray[1] ;
      if( undefined == map[key] ) {
         var array = new Array() ;
         map[key] = array ;
      }
      map[key].push( value ) ;
   }
   for( var tmpKey in map ) {
      var hostname = tmpKey ;
      var tmpArray = map[tmpKey] ;
      if( 0 != tmpArray.length ) {
         var portStr = tmpArray[0] ;
         for( var i = 1; i < tmpArray.length; i++ ) {
            portStr += 
"," + tmpArray[i] ;
         }

         /* New version */
         try {
            if ( NEW_VERSION ) {
               var result = restartNodeWithOma( hostname, portStr ) ;
               if ( result ) {
                  continue ;
               }
            }
         } catch ( e ) {
            println( "Restart all catalog nodes in " + hostname + " failed: " + e + "(" + getLastErrMsg() + ")" ) ;
            return false ;
         }

         /* Old version */
         var ssh ;
         /* Stop and start  */
         try {
            ssh = new Ssh( hostname, USERNAME, PASSWD ) ;
         } catch ( e ) {
            println( "Ssh to " + hostname+ " failed: " + e + "(" + getLastErrMsg() + ")" ) ;
            return false ;
         }

         try {
            ssh.exec( SDBSTOP + " -p " + portStr ) ;
            println( "Stop all catalog nodes succeed in " + hostname ) ;
            ssh.exec( SDBSTART + " -p " + portStr ) ;
            println( "Start all catalog nodes succeed in " + hostname ) ;
         } catch ( e ) {
            println( "Restart all catalog nodes in " + hostname + " failed: " + e + "(" + getLastErrMsg() + ")" ) ;
            ssh.close() ;
            return false ;
         }
         ssh.close() ;
      }
   }
   return true ;
}
/* *****************************************************************************
@discription: 更新节点配置并重启节点
@author: Jiaming Wu
@return: true/false
***************************************************************************** */
function updateNodeConfigAndRestart( nodeArr, key, value ) {
   if( !updateNodesConfig( nodeArr, key , value ) ) {
      println( "update nodes' config failed" ) ;
      return false ;
   }
   if( !restartNode( nodeArr ) ) {
      println( "Restart nodes failed" ) ;
      return false
   }
   return true ;
}
/* *****************************************************************************
@discription: 故障组剔除故障编目节点
@author: Jiaming Wu
@return: true/false
***************************************************************************** */
function detachCatalogNode( coordAddr, cataAddr ) {
   var db ;
   var isOK = true ;
   var needDisableAuth = false ;

   try {
      var addrArray = splitHostAndSvcFromAddr( coordAddr ) ;
      db = new Sdb( addrArray[0], addrArray[1], SDBUSERNAME, SDBPASSWD ) ;
   } catch ( e ) {
      if( SDB_CLS_NOT_PRIMARY == e ) {
         isOK = false ;
         needDisableAuth = true ;
      }else{
         println( "Connect to " + addrArray[0] + ":"
+ addrArray[1] + " failed: "
                  + e + " (" + getLastErrMsg() + ")" ) ;
         throw e ;
      }
   }
   if( isOK ) {
      var snapshotCursor ;
      /* Get specified group node snapshot */
      try {
         snapshotCursor = db.snapshot( SDB_SNAP_DATABASE, { GroupName: SDB_CATALOG_GROUP_NAME,
                                                            RawData: true } ) ;
      } catch( e ) {
         println( "Get cluster snapshot failed: " + e +
                  " (" + getLastErrMsg() + ")" ) ;
         return false ;
      }

      var exist = false ;
      var bs ;
      try {
         /* Check whether the primiary node exist and get error nodes info */
         while( ( bs = snapshotCursor.next() ) != undefined ) {
            var obj = bs.toObj() ;
            if( obj.ErrNodes != undefined ) {
               continue ;
            }
            if( obj.IsPrimary == true ) {
               exist = true ;
            }
         }
      } catch( e ) {
         println( "Traverse snapshot failed: " + e +
                  " (" + getLastErrMsg() + ")" ) ;
         return false ;
      }
      if( exist == true )
      {
         return true ;
      }
   }
   var existCatalogAddr = new Array() ;
   var existCatalogID = new Array() ;
   var map = new Object() ;
   for( var i = 0; i < cataAddr.length; i++ ) {
      var addrArray = splitHostAndSvcFromAddr( cataAddr[i] ) ;
      var key = addrArray[0] ;
      var value = addrArray[1] ;
      if( undefined == map[key] ) {
         var array = new Array() ;
         map[key] = array ;
      }
      map[key].push( value ) ;
   }
   for( var tmpKey in map ) {
      var hostname = tmpKey ;
      var tmpArray = map[tmpKey] ;
      if( 0 != tmpArray.length ) {
         var filterArr = [ { "svcname": tmpArray[0] } ];
         for( var i = 1; i < tmpArray.length; i++ ) {
            filterArr.push( { "svcname": tmpArray[i] } ) ;
         }
         try{
            var omaSvc = Oma.getAOmaSvcName( hostname ) ;
            var oma = new Oma( hostname, omaSvc ) ;
            result = oma.listNodes( { "role": "catalog" }, { "$or": filterArr } ) ;
            oma.close() ;
         } catch( e ) {
            if( SDB_NETWORK == e ) {
               continue ;
            }
            else {
               println( "Get exist catalog nodes failed: " + e + "(" + getLastErrMsg() + ")" ) ;
               return false ;
            }
         }
         if( result.size() == 0 ) {
            continue ;
         }
         var bs ;
         while( ( bs = result.next() ) != undefined ) {
            var obj = bs.toObj() ;
            existCatalogAddr.push( hostname + ":" + obj.svcname ) ;
            existCatalogID.push( obj.nodeid ) ;
         }
      }
   }

   if( needDisableAuth ) {

      /* Disable auth */
      if( !updateNodeConfigAndRestart( existCatalogAddr, "auth" , "FALSE" ) ) {
         println( "Disable catalog auth failed" ) ;
         return false ;
      }
      println( "Disable catalog auth succeed" ) ;
   }

   /* Get Nodes which will be detached */
   var detachNodeArray = new Array() ;
   var candidateAddr = new Array() ;
   for( var i = 0; i < cataAddr.length; i++ ) {
      var addrArray = splitHostAndSvcFromAddr( cataAddr[i] ) ;
      try{
         var tmpDb ;
         tmpDb = new Sdb( addrArray[0], addrArray[1], SDBUSERNAME, SDBPASSWD ) ;
         tmpDb.close() ;
         candidateAddr.push( addrArray ) ;
      } catch( e ){
         detachNodeArray.push( addrArray ) ;
      }
   }

   if( candidateAddr.length == 0 ) {
      println( "Failed: There are no available nodes in " + SDB_CATALOG_GROUP_NAME +
               " ReplicaGroup" ) ;
      if( needDisableAuth ) {
         /* Restore auth */
         if( !updateNodeConfigAndRestart( existCatalogAddr, "auth" , "TRUE" ) ) {
            println( "Restore catalog auth failed" ) ;
            return false ;
         }
         println( "Restore catalog auth succeed" ) ;
      }
      return false ;
   }

   if( candidateAddr.length < MINREPLICANUM ) {
       println( "Detach node from " + SDB_CATALOG_GROUP_NAME +
                 " failed: " + "After detachment,the number of exant node in "
                 + SDB_CATALOG_GROUP_NAME + " must greater than " + MINREPLICANUM ) ;
       if( needDisableAuth ) {
         /* Restore auth */
         if( !updateNodeConfigAndRestart( existCatalogAddr, "auth" , "TRUE" ) ) {
            println( "Restore catalog auth failed" ) ;
            return false ;
         }
         println( "Restore catalog auth succeed" ) ;
      }
       return false ;
   }

   /* To step up master appear */
   var success = false ;
   var primaryNodeID = 0 ;
   for( var i = 0; i < candidateAddr.length; i++ ) {
      var addrArray = candidateAddr[i] ;
      println( "Try to step up " + addrArray[0] + ":" + addrArray[1] + " election" ) ;
      var retryTime = 0 ;
      try
      {
         while( true ) {
            try {
               var cataDb = new Sdb( addrArray[0], addrArray[1], SDBUSERNAME, SDBPASSWD ) ;
               cataDb.forceStepUp( { Seconds: 300 } ) ;
               cataDb.close() ;
               break ;
            } catch( e ) {
               if( -251 == e && retryTime < 5 ) {
                  sleep( 1000 ) ;
                  retryTime++ ;
                  continue ;
               } else {
                  throw e ;
               }
            }
         }
      } catch( e ) {
         println( "Failed: " + e + " (" + getLastErrMsg() + ")" ) ;
         continue ;
      }
      var time = 0 ;
      var tmpCataDb ;
      try {
         tmpCataDb = new Sdb( addrArray[0], addrArray[1], SDBUSERNAME, SDBPASSWD ) ;
      } catch( e ) {
         println( "Connect to " + addrArray[0] + ":" + addrArray[1] + " failed: "
                  + e + " (" + getLastErrMsg() + ")" ) ;
         if( needDisableAuth ) {
            /* Restore auth */
            if( !updateNodeConfigAndRestart( existCatalogAddr, "auth" , "TRUE" ) ) {
               println( "Restore catalog auth failed" ) ;
               return false ;
            }
            println( "Restore catalog auth succeed" ) ;
         }
         return false ;
      }
      while( false == success && time < 10000 ) {
         var tmpCursor ;
         try {
            tmpCursor = tmpCataDb.snapshot( SDB_SNAP_DATABASE,
                                            { GroupName: SDB_CATALOG_GROUP_NAME,
                                              HostName: addrArray[0],
                                              ServiceName: addrArray[1],
                                              RawData: true } ) ;
         } catch( e ) {
            println( "Get " + addrArray[0] + ":" + addrArray[1] + " snapshot failed: "
                     + e + " (" + getLastErrMsg() + ")" ) ;
            if( needDisableAuth ) {
               /* Restore auth */
               if( !updateNodeConfigAndRestart( existCatalogAddr, "auth" , "TRUE" ) ) {
                  println( "Restore catalog auth failed" ) ;
                  return false ;
               }
               println( "Restore catalog auth succeed" ) ;
            }
            return false ;
         }
         var bs ;
         while( ( bs = tmpCursor.next() ) != undefined ) {
            var obj = bs.toObj() ;
            if( obj.HostName == addrArray[0] &&
                obj.ServiceName == addrArray[1] &&
                obj.IsPrimary == true ) {
               if( obj.NodeID instanceof Array && obj.NodeID.length == 2 ) {
                  primaryNodeID = obj.NodeID[1] ;
               }
               else {
                  println( "Get nodes id failed: " + e + " (" + getLastErrMsg() + ")" ) ;
                  if( needDisableAuth ) {
                     /* Restore auth */
                     if( !updateNodeConfigAndRestart( existCatalogAddr, "auth" , "TRUE" ) ) {
                        println( "Restore catalog auth failed" ) ;
                        return false ;
                     }
                     println( "Restore catalog auth succeed" ) ;
                  }
                  return false ;
               }
               success = true ;
               println( "Succeed" ) ;
               break ;
            }
         }
         if( !success ) {
            sleep( 500 ) ;
            time += 500 ;
         }
      }
      tmpCataDb.close() ;
      break ;
   }
   if( false == success ) {
      println( "Step up master election appear failed" ) ;
      if( needDisableAuth ) {
         /* Restore auth */
         if( !updateNodeConfigAndRestart( existCatalogAddr, "auth" , "TRUE" ) ) {
            println( "Restore catalog auth failed" ) ;
            return false ;
         }
         println( "Restore catalog auth succeed" ) ;
      }
      return false ;
   }

   /* update node info in catalog */
   for( var i = 0; i < candidateAddr.length; i++ ) {
      var addrArray = candidateAddr[i] ;
      try {
         var tmpCataDb = new Sdb( addrArray[0], addrArray[1], SDBUSERNAME, SDBPASSWD ) ;
         tmpCataDb.SYSCAT.SYSNODES.update( { $set: { "PrimaryNode": primaryNodeID } },
                                             { "GroupName": SDB_CATALOG_GROUP_NAME } ) ;
         tmpCataDb.close() ;
      } catch( e ) {
         println( "Update " + addrArray[0] + ":" + addrArray[1] + " node's info failed: "
                  + e + " (" + getLastErrMsg() + ")" ) ;
         if( needDisableAuth ) {
            /* Restore auth */
            if( !updateNodeConfigAndRestart( existCatalogAddr, "auth" , "TRUE" ) ) {
               println( "Restore catalog auth failed" ) ;
               return false ;
            }
            println( "Restore catalog auth succeed" ) ;
         }
         return false ;
      }
   }

   /* Detach node */
   var db ;
   try {
      var addrArray = splitHostAndSvcFromAddr( coordAddr ) ;
      db = new Sdb( addrArray[0], addrArray[1], SDBUSERNAME, SDBPASSWD ) ;
   } catch ( e ) {
      println( "Connect to " + coordAddr + " failed: " + e + " (" + getLastErrMsg() + ")" ) ;
      return false ;
   }
   /* force coord update cata group info */
   try{
      db.setSessionAttr( { PreferedInstance:"M" } ) ;
   } catch( e ){
      println( "Set session attr failed: " + e +
               " (" + getLastErrMsg() + ")" ) ;
      if( needDisableAuth ) {
         /* Restore auth */
         if( !updateNodeConfigAndRestart( existCatalogAddr, "auth" , "TRUE" ) ) {
            println( "Restore catalog auth failed" ) ;
            return false ;
         }
         println( "Restore catalog auth succeed" ) ;
      }
      return false ;
   }

   try
   {
      rg = db.getRG( SDB_CATALOG_GROUP_NAME ) ;
   } catch( e ) {
      println( "Get " + SDB_CATALOG_GROUP_NAME + " rg failed: " + e + " (" + getLastErrMsg() + ")" ) ;
      if( needDisableAuth ) {
         /* Restore auth */
         if( !updateNodeConfigAndRestart( existCatalogAddr, "auth" , "TRUE" ) ) {
            println( "Restore catalog auth failed" ) ;
            return false ;
         }
         println( "Restore catalog auth succeed" ) ;
      }
      return false ;
   }
   for( var i = 0; i < detachNodeArray.length; i++ ) {
      var addrArray = detachNodeArray[ i ] ;
      try {
         rg.detachNode( addrArray[0], addrArray[1],
                        { KeepData: false, enforced: true } ) ;
      } catch( e ) {
         println( "Detach node " + addrArray[0] + ":" + addrArray[1] + " from "
                  + SDB_CATALOG_GROUP_NAME + " failed: "
                  + e + " (" + getLastErrMsg() + ")" ) ;
         if( needDisableAuth ) {
            /* Restore auth */
            if( !updateNodeConfigAndRestart( existCatalogAddr, "auth" , "TRUE" ) ) {
               println( "Restore catalog auth failed" ) ;
               return false ;
            }
            println( "Restore catalog auth succeed" ) ;
         }
         return false ;
      }
      println( "Detach node " + addrArray[0] + ":" + addrArray[1] + " from "
               + SDB_CATALOG_GROUP_NAME + " succeed" ) ;
   }

   if( needDisableAuth ) {
      /* Enable auth */
      if( !updateNodesConfig( existCatalogAddr, "auth" , "TRUE" ) )
      {
         println( "Disable catalog auth failed" ) ;
         return false ;
      }
      try {
         db.reloadConf( { "GroupName": SDB_CATALOG_GROUP_NAME,
                          "NodeID": existCatalogID } ) ;
      } catch( e ) {
         println( "Reload configs failed " + e + " (" + getLastErrMsg() + ")" ) ;
         return false ;
      }
      println( "Restore catalog auth succeed" ) ;
   }
   return true ;
}
/* *****************************************************************************
@discription: 故障组剔除故障节点
@author: Jiaming Wu
@return: true/false
***************************************************************************** */
function detachGroupNode( coordAddr, filename ) {
   var db ;
   var groupsArray ;

   // Get cluster group info
   try {
      groupsArray = readGroupsInfo( filename ) ;
   } catch( e ) {
      println( "Read groups info failed: " + e + " (" + getLastErrMsg() + ")" ) ;
      return false ;
   }

   /* First to detach catalog group nodes */
   var nodeArray = parseGroupNodes( groupsArray, new Array() ) ;
   var cataAddr = nodeArray[0] ;
   if( !detachCatalogNode( coordAddr, cataAddr ) ) {
      return false ;
   }

   /* Detach data group nodes */
   try {
      var addrArray = splitHostAndSvcFromAddr( coordAddr ) ;
      db = new Sdb( addrArray[0], addrArray[1], SDBUSERNAME, SDBPASSWD ) ;
   } catch ( e ) {
      println( "Connect to " + coordAddr + " failed: " + e + " (" + getLastErrMsg() + ")" ) ;
      return false ;
   }

   for( var i = 0; i < groupsArray.length; i++ ) {
      var rgObj = groupsArray[ i ] ;
      if( rgObj.GroupName == SDB_COORD_GROUP_NAME ||
          rgObj.GroupName == SDB_CATALOG_GROUP_NAME ) {
         continue ;
      }
      var groupSize = rgObj.Group.length ;
      var groupName = rgObj.GroupName ;
      var snapshotCursor ;
      /* Get specified group node snapshot */
      try {
         snapshotCursor = db.snapshot( SDB_SNAP_DATABASE, { GroupName: groupName,
                                                            RawData: true } ) ;
      } catch( e ) {
         if( SDB_CLS_NODE_NOT_EXIST == e ){
            println( "Warning: No normal node exists in Group[" + groupName + "]" ) ;
            continue ;
         } else{
            println( "Get group[" + groupName + "] snapshot failed: " + e +
                     " (" + getLastErrMsg() + ")" ) ;
            return false ;
         }
      }

      var exist = false ;
      var bs ;
      var ErrNodes = [] ;
      var detachNodeArr = new Array() ;
      try {
         /* Check whether the primiary node exist and get error nodes info */
         while( ( bs = snapshotCursor.next() ) != undefined ) {
            var obj = bs.toObj() ;
            if( obj.ErrNodes != undefined ) {
               ErrNodes = obj.ErrNodes ;
               continue ;
            }
            if( obj.IsPrimary == true ) {
               exist = true ;
            }
         }
      } catch( e ) {
         println( "Traverse snapshot failed: " + e +
                  " (" + getLastErrMsg() + ")" ) ;
         return false ;
      }

      if( false == exist ) {
         /* Get Nodes which will be detached */
         for( var index = 0; index < ErrNodes.length; index++ ) {
            var nameArr = splitHostAndSvcFromAddr( ErrNodes[ index ].NodeName ) ;
            var tmpDb ;
            try {
               tmpDb = new Sdb( nameArr[ 0 ], nameArr[ 1 ],
                                SDBUSERNAME, SDBPASSWD ) ;
               tmpDb.close() ;
            }
            catch( e ) {
               detachNodeArr.push( { hostname: nameArr[ 0 ],
                                     svcname: nameArr[ 1 ] } ) ;
            }

         }
         /* After detach node, the number of extant group node must greater than MINREPLICANUM */
         if( groupSize - detachNodeArr.length < MINREPLICANUM ) {
            println( "Detach node from " + groupName +
                     " failed: " + "After detachment,the number of exant node in "
                     + groupName + " must greater than " + MINREPLICANUM ) ;
            return false ;
         }

         /* update catalog primary node info to enable detach primary node */
         var catalogMaste ;
         try {
            catalogMaster = db.getRG( SDB_CATALOG_GROUP_NAME ).getMaster() ;
         }catch( e ) {
            println( "Get catalog master failed: " + e + " (" + getLastErrMsg() + ")" ) ;
            return false ;
         }
         var cataDb ;
         try {
            cataDb = new Sdb( catalogMaster._hostname, catalogMaster._servicename,
                              SDBUSERNAME, SDBPASSWD ) ;
         } catch( e ) {
            println( "Connect to catalog master failed: " + e + " (" + getLastErrMsg() + ")" ) ;
            return false ;
         }
         try
         {
            cataDb.SYSCAT.SYSNODES.update( { "$unset": { "PrimaryNode": 1 } }, { "GroupName": groupName } ) ;
         } catch( e ) {
            println( "update catalog master node info failed : " + e + " (" + getLastErrMsg() + ")" ) ;
            return false ;
         }

         /* Detach Error node */
         for( var index = 0; index < detachNodeArr.length; index++ ) {
            var detachNode = detachNodeArr[ index ] ;
            var nodeName = detachNode.hostname + ":" + detachNode.svcname ;
            try {
               db.getRG( groupName ).detachNode( detachNode.hostname,
                                                 detachNode.svcname,
                                                { KeepData: true,
                                                  enforced: true } ) ;
            } catch( e ) {
               println( "Detach node " + nodeName + " from "
                        + groupName + " failed: " + e +
                        " (" + getLastErrMsg() + ")" ) ;
               return false ;
            }
            println( "Detach node " + nodeName + " from "
                     + groupName + " succeed" ) ;
         }
      }
   }
   return true ;
}

/* *****************************************************************************
@discription: 故障组合并故障节点
@author: Jiaming Wu
@return: true/false
***************************************************************************** */
function attachGroupNode( coordAddr, filename ) {
   var groupsArray ;
   var db ;
   /* Get group info */
   try {
      groupsArray = readGroupsInfo( filename ) ;
   } catch ( e ) {
      println( "Read groups info from file[" + filename + "] failed: " + e ) ;
      return false ;
   }

   try {
      var addrArray = splitHostAndSvcFromAddr( coordAddr ) ;
      db = new Sdb( addrArray[0], addrArray[1], SDBUSERNAME, SDBPASSWD ) ;
   } catch ( e ) {
      println( "Connect to " + coordAddr + " failed: " + e + " (" + getLastErrMsg() + ")" ) ;
      return false ;
   }

   for( var index = 0; index < groupsArray.length; index++ ) {
      var tmpGroupObj = groupsArray[ index ] ;
      var groupName = tmpGroupObj.GroupName
      if( SDB_COORD_GROUP_NAME == groupName ) {
         continue ;
      }
      var group = tmpGroupObj.Group ;
      var snapshotCursor ;
      var currentNodeArr = new Array() ;
      try {
         snapshotCursor = db.snapshot( SDB_SNAP_DATABASE, { GroupName: groupName,
                                                            RawData: true } ) ;
      } catch( e ) {
         if( SDB_CLS_NODE_NOT_EXIST == e ){
            println( "Warning: No normal node exists in Group[" + groupName + "]" ) ;
            continue ;
         } else {
            println( "Get group[" + groupName + "] failed: " + e +
                     "(" + getLastErrMsg() + ")" ) ;
            return false ;
         }
      }
      var bs ;
      while( ( bs = snapshotCursor.next() ) != undefined ) {
         var tmpObj = bs.toObj() ;
         if( tmpObj.ErrNodes != undefined ) {
            for( var j = 0; j < tmpObj.ErrNodes.length; j++ ) {
               currentNodeArr.push( tmpObj.ErrNodes[ j ] ) ;
            }
         } else {
            currentNodeArr.push( tmpObj ) ;
         }
      }
      for( var j = 0; j < group.length; j++ ) {
         var node = group[ j ] ;
         var hostname = node.HostName ;
         for( var k = 0; k < node.Service.length; k++ ) {
            var serviveObj = node.Service[ k ] ;
            if( serviveObj.Type == 0 ) {
               var svcname = serviveObj.Name ;
               var filter = new _Filter( { NodeName: hostname + ":" + svcname } ) ;
               var result = filter.match( currentNodeArr ) ;
               if( result.size() == 0 )
               {
                  var option = new Object() ;
                  if( SDB_CATALOG_GROUP_NAME == groupName ) {
                     option.KeepData = false ;
                  }  else {
                     option.KeepData = true ;
                  }
                  try {
                     db.getRG( groupName ).attachNode( hostname, svcname,
                                                       option ) ;
                     println( "Attach node " + hostname + ":" + svcname + " to "
                              + groupName + " succeed " ) ;
                  } catch( e ) {
                     println( "Attach node " + hostname + ":" + svcname + " to "
                              + groupName + " failed: " + e +
                              " (" + getLastErrMsg() + ")" ) ;
                     return false ;
                  }
               }
            }
         }
      }
   }
   return true ;
}
/* *****************************************************************************
@discription: 入口函数
@author: Jianhui Xu
@return: true/false
***************************************************************************** */
function main() {
   println( "Begin to check args..." ) ;
   if ( checkArgs() ) {
      println( "Done" ) ;
   } else {
      println( "Failed" ) ;
      return ;
   }

   println( "Begin to check environment..." ) ;
   if ( checkSdbVersion() ) {
      NEW_VERSION = true ;
   }
   if ( checkHostsEvn( CURHOSTS ) ) {
      println( "Done" ) ;
   } else {
      println( "Failed" ) ;
      return ;
   }

   /* Doing */
   if ( "init" == CUROPR ) {
      println( "Begin to init cluster..." ) ;
      if ( initCluster( COORDADDR, INITFILE, ACTIVE ) ) {
         println( "Done" ) ;
      } else {
         println( "Failed" ) ;
         return ;
      }
   } else if ( "split" == CUROPR ) {
      println( "Begin to split cluster..." ) ;
      if ( !prepareEnv( INITFILE, CURHOSTS ) ) {
         println( "Prepare env failed" ) ;
         return false ;
      }
      if ( splitCluster( CURCATAS, CURHOSTS, ACTIVE ) ) {
         println( "Done" ) ;
      } else {
         println( "Failed" ) ;
         return ;
      }
   } else if ( "merge" == CUROPR ) {
      println( "Begin to merge cluster..." ) ;
      if ( !prepareEnv( INITFILE, ALLHOSTS ) ) {
         println( "Prepare env failed" ) ;
         return false ;
      }
      if ( mergeCluster( CURCATAS, ALLHOSTS, INITFILE, ACTIVE ) ) {
         println( "Done" ) ;
      } else {
         println( "Failed" ) ;
         return ;
      }
   } else if ( "detachGroupNode" == CUROPR ) {
      println( "Begin to detach group node" ) ;
      if ( !prepareEnv( INITFILE, CURHOSTS ) ) {
         println( "Prepare env failed" ) ;
         return false ;
      }
      if ( detachGroupNode( COORDADDR[0], INITFILE ) ) {
         println( "Done" ) ;
      } else {
         println( "Failed" ) ;
         return ;
      }
   } else if ( "attachGroupNode" == CUROPR) {
      println( "Begin to attach group node" ) ;
      if ( !prepareEnv( INITFILE, CURHOSTS ) ) {
         println( "Prepare env failed" ) ;
         return false ;
      }
      if ( attachGroupNode( COORDADDR[0], INITFILE ) ) {
         println( "Done" ) ;
      } else {
         println( "Failed" ) ;
         return ;
      }
   } else {
      println( "Unknow command[" + CUROPR + "]" ) ;
      throw "Unknow command" ;
   }
}

main() ;
