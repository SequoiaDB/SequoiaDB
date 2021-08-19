//@ sourceURL=Deploy.AddHost.Ctrl.js
//"use strict" ;
(function(){
   var sacApp = window.SdbSacManagerModule ;
   //控制器
   sacApp.controllerProvider.register( 'Deploy.AddHost.Ctrl', function( $scope, $compile, $location, $rootScope, SdbRest, SdbPromise, SdbSwap, SdbSignal ){

      SdbSwap.hostList = SdbPromise.init( 1 ) ;

      //读取部署的传参
      var addHostInfo  = $rootScope.tempData( 'Deploy', 'AddHost' ) ;
      var deployModel  = $rootScope.tempData( 'Deploy', 'Model' ) ;
      var deplpyModule = $rootScope.tempData( 'Deploy', 'Module' ) ;
      var installPath  = $rootScope.tempData( 'Deploy', 'InstallPath' ) ;
      var clusterName  = $rootScope.tempData( 'Deploy', 'ClusterName' ) ;
      var discoverConf = $rootScope.tempData( 'Deploy', 'DiscoverConf' ) ;
      var syncConf     = $rootScope.tempData( 'Deploy', 'SyncConf' ) ;

      if( deployModel == null || clusterName == null || installPath == null || deplpyModule == null || addHostInfo == null )
      {
         $location.path( '/Deploy/Index' ).search( { 'r': new Date().getTime() } ) ;
         return ;
      }

      //初始化
      $scope.CheckedHostNum = 0 ;
      var hostInfoList = [] ;
      var hostList = $.extend( true, [], addHostInfo['HostInfo'] ) ;
      var hostSort = function( h1, h2 ){
         return h1['IP'] > h2['IP'] ;
      }
      $.each( hostList, function( index, hostInfo ){
         hostInfo['background'] = '#fff' ;
      } ) ;
      hostList = hostList.sort( hostSort ) ;

      //创建步骤条
      if( discoverConf != null )
      {
         $scope.stepList = _Deploy.BuildDiscoverStep( $scope, $location, $scope['Url']['Action'], deplpyModule ) ;
      }
      else if( syncConf != null )
      {
         $scope.stepList = _Deploy.BuildSyncStep( $scope, $location, $scope['Url']['Action'], deplpyModule ) ;
      }
      else
      {
         $scope.stepList = _Deploy.BuildSdbStep( $scope, $location, deployModel, $scope['Url']['Action'], deplpyModule ) ;
      }

      if( $scope.stepList['info'].length == 0 )
      {
         $location.path( '/Deploy/Index' ).search( { 'r': new Date().getTime() } ) ;
         return ;
      }

      SdbSwap.hostList.resolve( 'HostList', hostList ) ;

      //跳转到上一步
      $scope.GotoScanHost = function(){
         $location.path( '/Deploy/ScanHost' ).search( { 'r': new Date().getTime() } ) ;
      }

      //安装主机
      var installHost = function( installConfig ){
         var data = { 'cmd': 'add host', 'HostInfo': JSON.stringify( installConfig ) } ;
         SdbRest.OmOperation( data, {
            'success': function( taskInfo ){
               $rootScope.tempData( 'Deploy', 'HostTaskID', taskInfo[0]['TaskID'] ) ;
               $location.path( '/Deploy/Task/Host' ).search( { 'r': new Date().getTime() } ) ;
            },
            'failed': function( errorInfo ){
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  installHost( installConfig ) ;
                  return true ;
               } ) ;
            }
         } ) ;
      }

      //跳转下一步 安装主机
      $scope.GotoInstall = function(){
         if( $scope.CheckedHostNum > 0 )
         {
            var tempPoraryHosts = [] ;
            $.each( hostInfoList, function( index, hostInfo ){
               //判断是不是错误的主机
               if( hostInfo['errno'] === 0 && hostInfo['IsUse'] === true )
               {
                  var tempHostInfo = {} ;
                  tempHostInfo['User']          = hostInfo['User'] ;
                  tempHostInfo['Passwd']        = hostInfo['Passwd'] ;
                  tempHostInfo['SshPort']       = hostInfo['SshPort'] ;
                  tempHostInfo['AgentService']   = hostInfo['AgentService'] ;
                  tempHostInfo['HostName']      = hostInfo['HostName'] ;
                  tempHostInfo['IP']            = hostInfo['IP'] ;
                  tempHostInfo['CPU']            = hostInfo['CPU'] ;
                  tempHostInfo['Memory']         = hostInfo['Memory'] ;
                  tempHostInfo['Net']            = hostInfo['Net'] ;
                  tempHostInfo['Port']            = hostInfo['Port'] ;
                  tempHostInfo['Service']         = hostInfo['Service'] ;
                  tempHostInfo['OMA']            = hostInfo['OMA'] ;
                  tempHostInfo['POSTGRESQL']    = hostInfo['POSTGRESQL'] ;
                  tempHostInfo['MYSQL']         = hostInfo['MYSQL'] ;
                  tempHostInfo['Safety']         = hostInfo['Safety'] ;
                  tempHostInfo['OS']            = hostInfo['OS'] ;
                  tempHostInfo['InstallPath']   = hostInfo['InstallPath'] ;
                  tempHostInfo['Disk']            = [] ;
                  $.each( hostInfo['Disk'], function( index2, hostDisk ){
                     if( hostDisk['IsUse'] === true )
                     {
                        var tempHostDisk = {} ;
                        tempHostDisk['Name']    = hostDisk['Name'] ;
                        tempHostDisk['Mount']   = hostDisk['Mount'] ;
                        tempHostDisk['Size']    = hostDisk['Size'] ;
                        tempHostDisk['Free']    = hostDisk['Free'] ;
                        tempHostDisk['IsLocal'] = hostDisk['IsLocal'] ;
                        tempHostInfo['Disk'].push( tempHostDisk ) ;
                     }
                  } ) ;
                  tempPoraryHosts.push( tempHostInfo ) ;
               }
            } ) ;
            var newHostInfo = { 'ClusterName': clusterName, 'HostInfo': tempPoraryHosts, 'User': '-', 'Passwd': '-', 'SshPort': '-', 'AgentService': '-' } ;
            installHost( newHostInfo ) ;
         }
         else
         {
            $scope.Components.Confirm.type = 3 ;
            $scope.Components.Confirm.context = $scope.autoLanguage( '至少选择一台主机，才可以进入下一步操作。' ) ;
            $scope.Components.Confirm.isShow = true ;
            $scope.Components.Confirm.noClose = true ;
         }
      }

      SdbSignal.on( 'CheckedHostNum', function(){
         $scope.CheckedHostNum = 0 ;
         $.each( hostInfoList, function( index, hostInfo ){
            if( hostInfo['IsUse'] == true )
            {
               ++$scope.CheckedHostNum ;
            }
         } ) ;
      } ) ;

      function ComparVersion( a, b )
      {
         var v1 = a.split( '.' ) ;
         var v2 = b.split( '.' ) ;

         v1[0] = parseInt( v1[0] ) ;
         v2[0] = parseInt( v2[0] ) ;

         if( v1[0] > v2[0] )
         {
            return 1 ;
         }
         else if( v1[0] == v2[0] )
         {
            v1[1] = v1[1] ? parseInt( v1[1] ) : 0 ;
            v2[1] = v2[1] ? parseInt( v2[1] ) : 0 ;

            if( v1[1] > v2[1] )
            {
               return 1 ;
            }
            else if( v1[1] == v2[1] )
            {
               v1[2] = v1[2] ? parseInt( v1[2] ) : 0 ;
               v2[2] = v2[2] ? parseInt( v2[2] ) : 0 ;

               if( v1[2] >= v2[2] )
               {
                  return 1 ;
               }
               else
               {
                  return -1 ;
               }
            }
            else
            {
               return -1 ;
            }
         }
         else
         {
            return -1 ;
         }
      }

      function initMySQL( hostInfo )
      {
         var mysqlInfo = hostInfo['MYSQL'] ;

         hostInfo['MySQLList'] = [] ;

         if ( mysqlInfo.length > 0 )
         {
            var defaultUse = 0 ;
            var maxVersion = mysqlInfo[0]['Version'] ;

            $.each( mysqlInfo, function( index, info ){
               mysqlInfo[index]['IsUse'] = false ;
               if( ComparVersion( maxVersion, info['Version'] ) == -1 )
               {
                  defaultUse = index ;
                  maxVersion = info['Version'] ;
               }
            } ) ;

            mysqlInfo[defaultUse]['IsUse'] = true ;
            hostInfo['MySQLList'] = mysqlInfo ;
            hostInfo['MYSQL'] = {
               'Version':  mysqlInfo[defaultUse]['Version'],
               'SdbUser':  mysqlInfo[defaultUse]['SdbUser'],
               'Path':     mysqlInfo[defaultUse]['Path']
            } ;
         }
         else
         {
            hostInfo['MYSQL'] = {} ;
         }
      }

      function initPG( hostInfo )
      {
         var pgInfo = hostInfo['POSTGRESQL'] ;

         hostInfo['PGList'] = [] ;

         if ( pgInfo.length > 0 )
         {
            var defaultUse = 0 ;
            var maxVersion = pgInfo[0]['Version'] ;

            $.each( pgInfo, function( index, info ){
               pgInfo[index]['IsUse'] = false ;
               if( ComparVersion( maxVersion, info['Version'] ) == -1 )
               {
                  defaultUse = index ;
                  maxVersion = info['Version'] ;
               }
            } ) ;

            pgInfo[defaultUse]['IsUse'] = true ;
            hostInfo['PGList'] = pgInfo ;
            hostInfo['POSTGRESQL'] = {
               'Version':  pgInfo[defaultUse]['Version'],
               'SdbUser':  pgInfo[defaultUse]['SdbUser'],
               'Path':     pgInfo[defaultUse]['Path']
            } ;
         }
         else
         {
            hostInfo['POSTGRESQL'] = {} ;
         }
      }

      //获取检查主机的数据
      var checkHost = function(){
         var data = { 'cmd': 'check host', 'HostInfo': JSON.stringify( addHostInfo ) } ;
         SdbRest.OmOperation( data, {
            'success': function( result ){
               hostInfoList = result.sort( hostSort ) ;
               $.each( hostInfoList, function( index, hostInfo ){
                  if( typeof( hostInfo['errno'] ) == 'undefined' || hostInfo['errno'] == 0 )
                  {
                     hostInfo['errno']        = 0 ;
                     hostInfo['CanUse']       = false ;
                     hostInfo['IsUseNum']     = 0 ;
                     hostInfo['CanNotUseNum'] = 0 ;
                     hostInfo['DiskWarning']  = 0 ;

                     var useHostNum = 0 ;
                     var tmpUseNum = 0 ;
                     var useSumSize = 0 ;
                     var filterDiskList = [] ;
                     var diskNameList = {} ;
                     $.each( hostInfo['Disk'], function( index2, diskInfo ){
                        if( diskInfo['CanUse'] == true && diskInfo['IsLocal'] == true )
                        {
                           ++tmpUseNum ;
                        }
                     } ) ;

                     $.each( hostInfo['Disk'], function( index2, diskInfo ){
                        //防止重复挂载盘
                        if( diskNameList[diskInfo['Name']] === 0 || diskNameList[diskInfo['Name']] === 1 )
                        {
                           diskNameList[diskInfo['Name']] = 1 ;
                        }
                        else
                        {
                           diskNameList[diskInfo['Name']] = 0 ;
                        }

                        if ( diskInfo['Mount'] == '/' && tmpUseNum > 1 )
                        {
                           //多个符合条件的挂载盘，默认不选根路径
                           diskInfo['IsUse'] = false ;
                        }
                        else if( diskNameList[diskInfo['Name']] === 1 )
                        {
                           diskInfo['IsUse'] = false ;
                        }
                        else if ( diskInfo['CanUse'] == true && diskInfo['IsLocal'] == true )
                        {
                           useSumSize += diskInfo['Size'] ;
                           ++useHostNum ;
                        }
                        filterDiskList.push( diskInfo ) ;
                     } ) ;

                     //可用磁盘的基准值大小
                     var baseSize = ( useSumSize / useHostNum ) * 0.2 ;

                     $.each( filterDiskList, function( index2, diskInfo ){
                        if( diskInfo['CanUse'] == true )
                        {
                           if( diskInfo['IsLocal'] == true && diskInfo['IsUse'] !== false )
                           {
                              if ( diskInfo['Size'] < baseSize )
                              {
                                 filterDiskList[index2]['IsUse'] = false ;
                              }
                              else
                              {
                                 filterDiskList[index2]['IsUse'] = true ;
                                 ++hostInfo['IsUseNum'] ;

                                 //至少有一个磁盘选中，主机可以用
                                 hostInfo['CanUse'] = true ;
                              }
                           }
                        }
                        else
                        {
                           ++hostInfo['CanNotUseNum'] ;
                        }
                     } ) ;

                     hostInfo['Disk'] = filterDiskList ;
                     hostInfo['DiskWarning'] = sprintf( $scope.autoLanguage( '有?个磁盘剩余容量不足。' ), hostInfo['CanNotUseNum'] ) ;
                     hostInfo['IsUse'] = hostInfo['CanUse'] ;
                     if( hostInfo['OMA']['Path'].length > 0 )
                     {
                        hostInfo['InstallPath'] = hostInfo['OMA']['Path'] ;
                        hostInfo['IsUse'] = false ;
                     }
                     else
                     {
                        hostInfo['InstallPath'] = installPath ;
                     }

                     initMySQL( hostInfo ) ;
                     initPG( hostInfo ) ;

                     $.each( hostList, function( index2, hostInfo2 ){
                        if( hostInfo2['IP'] == hostInfo['IP'] )
                        {
                           hostInfo['User']           = hostInfo2['User'] ;
                           hostInfo['Passwd']         = hostInfo2['Passwd'] ;
                           hostInfo['SshPort']        = hostInfo2['SshPort'] ;
                           hostInfo['AgentService']   = hostInfo2['AgentService'] ;
                           return false ;
                        }
                     } ) ;
                  }
                  else
                  {
                     hostInfo['CanUse'] = false ;
                     hostInfo['IsUse'] = false ;
                     $.each( hostList, function( index2, hostInfo2 ){
                        if( hostInfo2['IP'] == hostInfo['IP'] )
                        {
                           hostInfo['HostName'] = hostInfo2['HostName'] ;
                           return false ;
                        }
                     } ) ;
                  }

                  hostInfo['background'] = '#fff' ;
                  if( hostInfo['CanUse'] == false )
                  {
                      hostInfo['background'] = '#eee' ;
                  }
               } ) ;

               SdbSignal.commit( 'HostInfo', hostInfoList ) ;
            },
            'failed': function( errorInfo ){
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  checkHost() ;
                  return true ;
               } ) ;
            }
         } ) ;
      }
      checkHost() ;
   } ) ;
}());
