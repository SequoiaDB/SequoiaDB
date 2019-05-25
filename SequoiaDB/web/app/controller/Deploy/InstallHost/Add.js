//@ sourceURL=Add.js
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
         $scope.stepList = _Deploy.BuildSdbDiscoverStep( $scope, $location, $scope['Url']['Action'], 'sequoiadb' ) ;
      }
      else if( syncConf != null )
      {
         $scope.stepList = _Deploy.BuildSdbSyncStep( $scope, $location, $scope['Url']['Action'], 'sequoiadb' ) ;
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
               $location.path( '/Deploy/InstallHost' ).search( { 'r': new Date().getTime() } ) ;
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
			         tempHostInfo['AgentService']	= hostInfo['AgentService'] ;
                  tempHostInfo['HostName']		= hostInfo['HostName'] ;
			         tempHostInfo['IP']				= hostInfo['IP'] ;
			         tempHostInfo['CPU']				= hostInfo['CPU'] ;
			         tempHostInfo['Memory']			= hostInfo['Memory'] ;
			         tempHostInfo['Net']				= hostInfo['Net'] ;
			         tempHostInfo['Port']				= hostInfo['Port'] ;
			         tempHostInfo['Service']			= hostInfo['Service'] ;
			         tempHostInfo['OMA']				= hostInfo['OMA'] ;
			         tempHostInfo['Safety']			= hostInfo['Safety'] ;
			         tempHostInfo['OS']				= hostInfo['OS'] ;
			         tempHostInfo['InstallPath']	= hostInfo['InstallPath'] ;
			         tempHostInfo['Disk']				= [] ;
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
            $scope.Components.Confirm.okText = $scope.autoLanguage( '好的' ) ;
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

                     var filterDiskList = [] ;
                     var diskNameList = {} ;
                     $.each( hostInfo['Disk'], function( index2, diskInfo ){
                        if( diskNameList[diskInfo['Name']] === 0 || diskNameList[diskInfo['Name']] === 1 )
                        {
                           diskNameList[diskInfo['Name']] = 1 ;
                        }
                        else
                        {
                           diskNameList[diskInfo['Name']] = 0 ;
                        }
                        if( diskInfo['CanUse'] == true )
                        {
                           if( diskInfo['IsLocal'] == true )
                           {
                              diskInfo['IsUse'] = true ;
                              hostInfo['CanUse'] = true ;
                              ++hostInfo['IsUseNum'] ;
                           }
                           filterDiskList.push( diskInfo ) ;
                        }
                        else
                        {
                           ++hostInfo['CanNotUseNum'] ;
                           filterDiskList.push( diskInfo ) ;
                        }
                     } ) ;
                     $.each( filterDiskList, function( index2, diskInfo ){
                        if( diskNameList[diskInfo['Name']] === 1 )
                        {
                           if( diskInfo['CanUse'] == true && diskInfo['IsLocal'] == true )
                           {
                              //磁盘出现大于1次
                              diskInfo['IsUse'] = false ;
                              if( hostInfo['IsUseNum'] > 0 )
                              {
                                 --hostInfo['IsUseNum'] ;
                              }
                              if( hostInfo['IsUseNum'] == 0 )
                              {
                                 hostInfo['CanUse'] = false ;
                              }
                           }
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

                     $.each( hostList, function( index2, hostInfo2 ){
                        if( hostInfo2['IP'] == hostInfo['IP'] )
                        {
                           hostInfo['User']           = hostInfo2['User'] ;
                           hostInfo['Passwd']         = hostInfo2['Passwd'] ;
                           hostInfo['SshPort']        = hostInfo2['SshPort'] ;
			                  hostInfo['AgentService']	= hostInfo2['AgentService'] ;
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

   sacApp.controllerProvider.register( 'Deploy.AddHost.HostList.Ctrl', function( $scope, SdbSwap, SdbSignal ){

      $scope.CurrentHost = 0 ;
      $scope.Search = { 'text': '' } ;
      $scope.HostInfoList = [] ;

      SdbSwap.hostList.then( function( result ){
         $scope.HostInfoList = result['HostList'] ;
      } ) ;

      SdbSignal.on( 'HostInfo', function( result ){
         $scope.HostInfoList = result ;
         $scope.SwitchHost( $scope.CurrentHost ) ;
         SdbSignal.commit( 'CheckedHostNum', null ) ;
      } ) ;

      //左侧栏的过滤
      $scope.Filter = function(){
         if( $scope.Search.text.length > 0 )
         {
            $.each( $scope.HostInfoList, function( index, hostInfo ){
               if( hostInfo['HostName'].indexOf( $scope.Search.text ) >= 0 )
               {
                  hostInfo['IsHostName'] = true ;
               }
               else
               {
                  hostInfo['IsHostName'] = false ;
               }
               if( hostInfo['IP'].indexOf( $scope.Search.text ) >= 0 )
               {
                  hostInfo['IsIP'] = true ;
               }
               else
               {
                  hostInfo['IsIP'] = false ;
               }
            } ) ;
         }
         else
         {
            $.each( $scope.HostInfoList, function( index, hostInfo ){
               hostInfo['IsHostName'] = false ;
               hostInfo['IsIP'] = false ;
            } ) ;
         }
      } ;

      //切换主机
      $scope.SwitchHost = function( index ){
         $scope.CurrentHost = index ;
         SdbSignal.commit( 'UpdateHostConfig', $scope.HostInfoList[index] ) ;
      }

      //检查主机
      $scope.CheckedHost = function( index ){
         if( typeof( $scope.HostInfoList[index]['errno'] ) == 'undefined' || $scope.HostInfoList[index]['errno'] == 0 )
         {
            if( $scope.HostInfoList[index]['CanUse'] == true )
            {
               $scope.HostInfoList[index]['IsUse'] = !$scope.HostInfoList[index]['IsUse'] ;
               SdbSignal.commit( 'CheckedHostNum', null ) ;
            }
         }
      }

   } ) ;

   sacApp.controllerProvider.register( 'Deploy.AddHost.HostInfo.Ctrl', function( $scope, $rootScope, SdbSignal ){

      $scope.HostInfo = {} ;

      $scope.CpuChart      = { 'percent': 0, 'text': '' } ;
      $scope.MemoryChart   = { 'percent': 0, 'text': '' } ;
      $scope.DiskChart     = { 'percent': 0, 'text': '' } ;

      $scope.DiskTableHeight = 32 ;
      $scope.PortTableHeight = 32 ;
      $scope.ServiceTableHeight = 32 ;
      $scope.DiskTable = {
         'title': {
            'IsUse':    '',
            'Name':     $scope.autoLanguage( '磁盘' ),
            'Mount':    $scope.autoLanguage( '挂载路径' ),
            'IsLocal':  $scope.autoLanguage( '本地磁盘' ),
            'Chart':    $scope.autoLanguage( '容量' )
         },
         'options': {
            'width': {
               'IsUse':    '40px',
               'Name':     '30%',
               'Mount':    '30%',
               'IsLocal':  '80px',
               'Chart':    '40%'
            },
            'max': 10000,
            'tools': false
         },
         'body': []
      } ;
      $scope.PortTable = {
         'title': {
            'Port':  $scope.autoLanguage( '端口' ),
            'CanUse': $scope.autoLanguage( '状态' )
         },
         'options': {
            'width': {
               'Port':     '40%',
               'CanUse':   '60%'
            },
            'max': 10000,
            'tools': false
         },
         'body': []
      } ;
      $scope.ServiceTable = {
         'title': {
            'Name':        $scope.autoLanguage( '服务名' ),
            'IsRunning':   $scope.autoLanguage( '状态' ),
            'Version':     $scope.autoLanguage( '版本' )
         },
         'options': {
            'width': {
               'Name':        '40%',
               'IsRunning':   '30%',
               'Version':     '30%'
            },
            'max': 10000,
            'tools': false
         },
         'body': []
      } ;
      //添加自定义路径 弹窗
      $scope.AddCustomPathWindow = {
         'config': {
            'inputList': [
               {
                  "name": "Name",
                  "webName": $scope.autoLanguage( '磁盘名' ),
                  "required": true,
                  "type": "string",
                  "value": '',
                  "valid": {
                     "min": 1
                  }
               },
               {
                  "name": "Mount",
                  "webName": $scope.autoLanguage( '挂载路径' ),
                  "required": true,
                  "type": "string",
                  "value": '',
                  "valid": {
                     "min": 1
                  }
               },
               {
                  "name": "Free",
                  "webName": $scope.autoLanguage( '可用容量(MB)' ),
                  "required": true,
                  "type": "int",
                  "value": '',
                  "valid": {
                     "min": 1
                  }
               },
               {
                  "name": "Size",
                  "webName": $scope.autoLanguage( '总容量(MB)' ),
                  "required": true,
                  "type": "int",
                  "value": '',
                  "valid": {
                     "min": 1
                  }
               }
            ]
         },
         'callback': {}
      } ;

      //选择颜色
      function switchColor( percent ){
         if( percent <= 60 )
         {
            return '#00CC66' ;
         }
         else if( percent <= 80 )
         {
            return '#FF9933' ;
         }
         else
         {
            return '#D9534F' ;
         }
      }

      //计算表格高度
      function getTableHeight( num ){
         if( num <= 5 )
         {
            return ( num + 1 ) * 31 + 1 ;
         }
         else
         {
            return 218 ;
         }
      }

      //磁盘
      var checkDiskList = function( diskList ){
         var diskFree = 0 ;
         var diskSize = 0 ;
         $.each( diskList, function( index, diskInfo ){
            if( diskInfo['IsLocal'] == true )
            {
               diskSize += diskInfo['Size'] ;
               diskFree += diskInfo['Free'] ;
            }
            var tmpDiskPercent = diskInfo['Size'] == 0 ? 0 : parseInt( ( diskInfo['Size'] - diskInfo['Free'] ) * 100 / diskInfo['Size'] ) ;
            diskInfo['Chart'] = {
               'percent': tmpDiskPercent,
               'style': {
                  'progress': {
                     'background': switchColor( tmpDiskPercent )
                  }
               },
               'text': sprintf( '? / ?', sizeConvert( diskInfo['Size'] - diskInfo['Free'] ), sizeConvert( diskInfo['Size'] ) )
            } ;
         } ) ;
         var disk = parseInt( ( diskSize - diskFree ) * 100 / diskSize ) ;
         $scope.DiskChart = {
            'percent': disk,
            'style': {
               'progress': {
                  'background': switchColor( disk )
               }
            },
            'text': sprintf( '? / ?', sizeConvert( diskSize - diskFree ), sizeConvert( diskSize ) )
         } ;

         $scope.DiskTable['body'] = diskList ;
         $scope.DiskTableHeight = getTableHeight( diskList.length ) ;
      }

      //端口
      var checkPortList = function( portList ){
         $scope.PortTable['body'] = [] ;
         $.each( portList, function( index, portInfo ){
            if( portInfo['Port'].length > 0 )
            {
               $scope.PortTable['body'].push( portInfo ) ;
            }
         } ) ;
         $scope.PortTableHeight = getTableHeight( $scope.PortTable['body'].length ) ;
      }

      //服务
      var checkServiceList = function( serviceList ){
         $scope.ServiceTable['body'] = [] ;
         $.each( serviceList, function( index, serviceInfo ){
            if( serviceInfo['Name'].length > 0 )
            {
               $scope.ServiceTable['body'].push( serviceInfo ) ;
            }
         } ) ;
         $scope.ServiceTableHeight = getTableHeight( $scope.ServiceTable['body'].length ) ;
      }

      //cpu
      var checkCpu = function( cpuInfo ){
         var cpu = parseInt( ( cpuInfo['Other'] + cpuInfo['Sys'] + cpuInfo['User'] ) * 100 / ( cpuInfo['Idle'] + cpuInfo['Other'] + cpuInfo['Sys'] + cpuInfo['User'] ) ) ;
         $scope.CpuChart = { 'percent': cpu, 'style': { 'progress': { 'background': switchColor( cpu ) } } } ;
      }

      //内存
      var checkMemory = function( memoryInfo ){
         var memory = parseInt( ( memoryInfo['Size'] - memoryInfo['Free'] ) * 100 / memoryInfo['Size'] ) ;
         $scope.MemoryChart = {
            'percent': memory,
            'style': {
               'progress': {
                  'background': switchColor( memory )
               }
            },
            'text': sprintf( '? / ?', sizeConvert( memoryInfo['Size'] - memoryInfo['Free'] ), sizeConvert( memoryInfo['Size'] ) )
         } ;
      }

      SdbSignal.on( 'UpdateHostConfig', function( hostInfo ){
         $scope.HostInfo = hostInfo ;
         if( typeof( hostInfo['errno'] ) == 'undefined' || hostInfo['errno'] == 0 )
         {
            checkCpu( hostInfo['CPU'] ) ;
            checkMemory( hostInfo['Memory'] ) ;
            checkDiskList( hostInfo['Disk'] ) ;
            checkPortList( hostInfo['Port'] ) ;
            checkServiceList( hostInfo['Service'] ) ;
         }
         else
         {
            $scope.CpuChart      = { 'percent': 0 } ;
            $scope.MemoryChart   = { 'percent': 0 } ;
            $scope.DiskChart     = { 'percent': 0 } ;
            $scope.DiskTable['body']    = [] ;
            $scope.PortTable['body']    = [] ;
            $scope.ServiceTable['body'] = [] ;
         }
         $rootScope.bindResize() ;
      } ) ;

      //打开 添加自定义路径 弹窗
      $scope.ShowAddCustomPath = function(){
         $.each( $scope.AddCustomPathWindow['config']['inputList'], function( index, inputInfo ){
            inputInfo['value'] = '' ;
         } ) ;
         $scope.AddCustomPathWindow['callback']['SetOkButton']( $scope.autoLanguage( '确定' ), function(){
            var isAllClear = $scope.AddCustomPathWindow['config'].check( function( formVal ){
               var error = [] ;
               if( formVal['Free'] > formVal['Size'] )
               {
                  error.push( { 'name': 'Free', 'error': sprintf( $scope.autoLanguage( '?的值不能大于?。' ), $scope.autoLanguage( '可用容量' ), $scope.autoLanguage( '总容量' ) ) } ) ;
                  error.push( { 'name': 'Size', 'error': sprintf( $scope.autoLanguage( '?的值不能小于?。' ), $scope.autoLanguage( '总容量' ), $scope.autoLanguage( '可用容量' ) ) } ) ;
               }
               else
               {
                  if( formVal['Free'] < 600 )
                  {
                     error.push( { 'name': 'Free', 'error': sprintf( $scope.autoLanguage( '?的值不能小于?。' ), $scope.autoLanguage( '可用容量' ), '600MB' ) } ) ;
                  }
                  if( formVal['Size'] < 600 )
                  {
                     error.push( { 'name': 'Size', 'error': sprintf( $scope.autoLanguage( '?的值不能小于?。' ), $scope.autoLanguage( '总容量' ), '600MB' ) } ) ;
                  }
               }
               return error ;
            } ) ;
            if( isAllClear )
            {
               var formVal = $scope.AddCustomPathWindow['config'].getValue() ;
               formVal['IsUse']   = true ;
               formVal['CanUse']  = true ;
               formVal['IsLocal'] = true ;
               var tmpDiskPercent = formVal['Size'] == 0 ? 0 : parseInt( ( formVal['Size'] - formVal['Free'] ) * 100 / formVal['Size'] ) ;
               formVal['Chart'] = {
                  'percent': tmpDiskPercent,
                  'style': {
                     'progress': {
                        'background': switchColor( tmpDiskPercent )
                     }
                  },
                  'text': sprintf( '? / ?', sizeConvert( formVal['Size'] - formVal['Free'] ), sizeConvert( formVal['Size'] ) )
               } ;
               $scope.HostInfo['Disk'].push( formVal ) ;
               ++$scope.HostInfo['IsUseNum'] ;
               $scope.HostInfo['CanUse'] = true ;
               checkDiskList( $scope.HostInfo['Disk'] ) ;
            }
            return isAllClear ;
         } ) ;
         $scope.AddCustomPathWindow['callback']['SetTitle']( $scope.autoLanguage( '添加自定义路径' ) ) ;
         $scope.AddCustomPathWindow['callback']['Open']() ;
      }

      //计算磁盘
      $scope.CountCheckedDisk = function(){
         setTimeout( function(){
            $scope.HostInfo['IsUseNum'] = 0 ;
            $.each( $scope.HostInfo['Disk'], function( index, diskInfo ){
               if( diskInfo['IsUse'] == true )
               {
                  ++$scope.HostInfo['IsUseNum'] ;
                  return false ;
               }
            } ) ;
            if( $scope.HostInfo['IsUseNum'] > 0 )
            {
               $scope.HostInfo['CanUse'] = true ;
               $scope.HostInfo['background'] = '#fff' ;
            }
            else
            {
               if( $scope.HostInfo['IsUse'] == true )
               {
                  $scope.HostInfo['IsUse'] = false ;
               }
               $scope.HostInfo['CanUse'] = false ;
               $scope.HostInfo['background'] = '#eee' ;
            }
            SdbSignal.commit( 'CheckedHostNum', null ) ;
            $scope.$apply() ;
         } ) ;
      }

   } ) ;

}());