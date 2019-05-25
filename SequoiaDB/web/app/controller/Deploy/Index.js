//@ sourceURL=Index.js
//"use strict" ;
(function(){
   var sacApp = window.SdbSacManagerModule ;
   //控制器
   sacApp.controllerProvider.register( 'Deploy.Index.Ctrl', function( $scope, $compile, $location, $rootScope, SdbFunction, SdbRest, Loading ){

      var defaultShow = $rootScope.tempData( 'Deploy', 'Index' ) ;

      //初始化
      $scope.EditHostGridOptions = { 'titleWidth': [ '200px', '200px', 100 ] } ;
      $scope.ShowType  = 1 ;
      //集群列表
      $scope.clusterList = [] ;
      //默认选的cluster
      $scope.currentCluster = 0 ;
      //默认显示业务还是主机列表
      $scope.currentPage = 'module' ;
      //业务列表
      $scope.moduleList = [] ;
      //主机列表
      $scope.HostList = [] ;
      //业务类型列表
      $scope.moduleType = [] ;
      //业务列表数量
      $scope.ModuleNum = 0 ;
      //业务集群模式数量
      $scope.DistributionNum = 0 ;
      //主机列表数量
      $scope.HostNum = 0 ;
      //选择主机的网格选项
      $scope.HostGridOptions = { 'titleWidth': [ '30px', '60px', 30, 30, 40 ] } ;
      //主机和业务的关联表(也就是有安装业务的主机列表)
      var host_module_table = [] ;
      //循环查询的业务
      var autoQueryModuleIndex = [] ;
      //关联关系列表
      $scope.RelationshipList = [] ;
      //主机表格
      $scope.HostListTable = {
         'title': {
            'Check':            '',
            'Error.Flag':       $scope.autoLanguage( '状态' ),
            'HostName':         $scope.autoLanguage( '主机名' ),
            'IP':               $scope.autoLanguage( 'IP地址' ),
            'BusinessName':     $scope.autoLanguage( '业务' ),
            'Packages':         $scope.autoLanguage( '包' )
         },
         'body': [],
         'options': {
            'width': {
               'Check':          '30px',
               'Error.Flag':     '60px',
               'HostName':       '25%',
               'IP':             '25%',
               'BusinessName':   '25%',
               'Packages':       '25%'
            },
            'sort': {
               'Check':                 false,
               'Error.Flag':            true,
               'HostName':              true,
               'IP':                    true,
               'BusinessName':          true,
               'Packages':              true
            },
            'max': 50,
            'filter': {
               'Check':             null,
               'Error.Flag':        'indexof',
               'HostName':          'indexof',
               'IP':                'indexof',
               'BusinessName':      'indexof',
               'Packages':          'indexof'
            }
         },
         'callback': {}
      } ;

      //清空Deploy域的数据
      $rootScope.tempData( 'Deploy' ) ;

      //获取业务关联关系
      var getRelationship = function(){
         var data = { 'cmd': 'list relationship' } ;
         SdbRest.OmOperation( data, {
            'success': function( result ){
               $scope.RelationshipList = result ;
               $.each( $scope.moduleList, function( index, moduleInfo ){
                  moduleInfo['Relationship'] = [] ;
                  $.each( $scope.RelationshipList, function( index2, relationInfo ){
                     if( relationInfo['From'] == moduleInfo['BusinessName'] || relationInfo['To'] == moduleInfo['BusinessName'] )
                     {
                        moduleInfo['Relationship'].push( relationInfo ) ;
                     }
                  } ) ;
               } ) ;
            },
            'failed': function( errorInfo ){
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  getRelationship() ;
                  return true ;
               } ) ;
            }
         }, {
            'showLoading': false
         } ) ;
      }

      //计算每个业务的资源
      var countModule_Host = function(){
         $.each( $scope.moduleList, function( index, moduleInfo ){
            if( isArray( moduleInfo['Location'] ) )
            {
               var cpu = 0 ;
               var memory = 0 ;
               var disk = 0 ;
               var length = 0 ;
               $.each( moduleInfo['Location'], function( index2, hostInfo ){
                  var index3 = hostModuleTableIsExist( hostInfo['HostName'] ) ;
                  if( index3 >= 0 )
                  {
                     ++length ;
                     cpu += host_module_table[index3]['Info']['CPU'] ;
                     memory += host_module_table[index3]['Info']['Memory'] ;
                     disk += host_module_table[index3]['Info']['Disk'] ;
                     if( $scope.moduleList[index]['Error']['Type'] == 'Host' || $scope.moduleList[index]['Error']['Flag'] == 0 )
                     {
                        if( host_module_table[index3]['Error']['Flag'] == 0 )
                        {
                           $scope.moduleList[index]['Error']['Flag'] = 0 ;
                        }
                        else
                        {
                           $scope.moduleList[index]['Error']['Flag'] = host_module_table[index3]['Error']['Flag'] ;
                           $scope.moduleList[index]['Error']['Type'] = 'Host' ;
                           $scope.moduleList[index]['Error']['Message'] = sprintf( $scope.autoLanguage( '主机 ? 状态异常: ?。' ), 
                                                                                   host_module_table[index3]['HostName'],
                                                                                   host_module_table[index3]['Error']['Message'] ) ;
                        }
                     }
                  }
               } ) ;
               $scope.moduleList[index]['Chart']['Host']['CPU'] = { 'percent': fixedNumber( cpu / length, 2 ), 'style': { 'progress': { 'background': '#87CEFA' } } } ;
               $scope.moduleList[index]['Chart']['Host']['Memory'] = { 'percent': fixedNumber( memory / length, 2 ), 'style': { 'progress': { 'background': '#DDA0DD' } } } ;
               $scope.moduleList[index]['Chart']['Host']['Disk'] = { 'percent':  fixedNumber( disk / length, 2 ), 'style': { 'progress': { 'background': '#FFA07A' } } } ;
            }
         } ) ;
      }
      
      //host_module_table是否已经存在该主机
      var hostModuleTableIsExist = function( hostName ){
         var flag = -1 ;
         $.each( host_module_table, function( index, hostInfo ){
            if( hostInfo['HostName'] == hostName )
            {
               flag = index ;
               return false ;
            }
         } ) ;
         return flag ;
      }

      //$scope.HostList是否存在该主机
      var hostListIsExist = function( hostName ){
         var flag = -1 ;
         $.each( $scope.HostList, function( index, hostInfo ){
            if( hostInfo['HostName'] == hostName )
            {
               flag = index ;
               return false ;
            }
         } ) ;
         return flag ;
      }

      //查询主机状态
      var queryHostStatus = function(){
         var isFirstQueryHostStatus = true ;
         var queryHostList ;
         SdbRest.OmOperation( null, {
            'init': function(){
               queryHostList = { 'HostInfo': [] } ;
               $.each( $scope.HostList, function( index, hostInfo ){
                  if( isFirstQueryHostStatus || hostInfo['ClusterName'] == $scope.clusterList[ $scope.currentCluster ]['ClusterName'] )
                  {
                     queryHostList['HostInfo'].push( { 'HostName': hostInfo['HostName'] } ) ;
                  }
               } ) ;
               isFirstQueryHostStatus = false ;
               return { 'cmd': 'query host status', 'HostInfo': JSON.stringify( queryHostList ) } ;
            },
            'before': function(){
               if( queryHostList['HostInfo'].length == 0 )
               {
                  setTimeout( queryHostStatus, 1 ) ;
                  return false ;
               }
            },
            'success': function( hostStatusList ){
               $.each( hostStatusList[0]['HostInfo'], function( index, statusInfo ){
                  var index2 = hostModuleTableIsExist( statusInfo['HostName'] ) ;
                  if( index2 >= 0 )
                  {
                     if( isNaN( statusInfo['errno'] ) || statusInfo['errno'] == 0  )
                     {
                        if( typeof( host_module_table[index2]['CPU'] ) == 'object' )
                        {
                           var resource = host_module_table[index2] ;
                           var old_idle1   = resource['CPU']['Idle']['Megabit'] ;
                           var old_idle2   = resource['CPU']['Idle']['Unit'] ;
                           var old_cpuSum1 = resource['CPU']['Idle']['Megabit'] +
                                             resource['CPU']['Other']['Megabit'] +
                                             resource['CPU']['Sys']['Megabit'] +
                                             resource['CPU']['User']['Megabit'] ;
                           var old_cpuSum2 = resource['CPU']['Idle']['Unit'] +
                                             resource['CPU']['Other']['Unit'] +
                                             resource['CPU']['Sys']['Unit'] +
                                             resource['CPU']['User']['Unit'] ;
                           var idle1   = statusInfo['CPU']['Idle']['Megabit'] ;
                           var idle2   = statusInfo['CPU']['Idle']['Unit'] ;
                           var cpuSum1 = statusInfo['CPU']['Idle']['Megabit'] +
                                         statusInfo['CPU']['Other']['Megabit'] +
                                         statusInfo['CPU']['Sys']['Megabit'] +
                                         statusInfo['CPU']['User']['Megabit'] ;
                           var cpuSum2 = statusInfo['CPU']['Idle']['Unit'] +
                                         statusInfo['CPU']['Other']['Unit'] +
                                         statusInfo['CPU']['Sys']['Unit'] +
                                         statusInfo['CPU']['User']['Unit'] ;
                           host_module_table[index2]['Info']['CPU'] = ( ( 1 - ( ( idle1 - old_idle1 ) * 1024 + ( idle2 - old_idle2 ) / 1024 ) / ( ( cpuSum1 - old_cpuSum1 ) * 1024 + ( cpuSum2 - old_cpuSum2 ) / 1024 ) ) * 100 ) ;
                        }
                        else
                        {
                           host_module_table[index2]['Info']['CPU'] = 0 ;
                        }
                        host_module_table[index2]['CPU'] = statusInfo['CPU'] ;
                        var diskFree = 0 ;
                        var diskSize = 0 ;
                        $.each( statusInfo['Disk'], function( index2, diskInfo ){
                           diskFree += diskInfo['Free'] ;
                           diskSize += diskInfo['Size'] ;
                        } ) ;
                        if( diskSize == 0 )
                        {
                           host_module_table[index2]['Info']['Disk'] = 0 ;
                        }
                        else
                        {
                           host_module_table[index2]['Info']['Disk'] = ( 1 - diskFree / diskSize ) * 100 ;
                        }
                        host_module_table[index2]['Info']['Memory'] = statusInfo['Memory']['Used'] / statusInfo['Memory']['Size'] * 100 ;
                        host_module_table[index2]['Error']['Flag'] = 0 ;
                        var index3 = hostListIsExist( statusInfo['HostName'] ) ;
                        if( index3 >= 0 )
                        {
                           $scope.HostList[index3]['Error']['Flag'] = 0 ;
                        }
                     }
                     else
                     {
                        host_module_table[index2]['Info']['CPU'] = 0 ;
                        host_module_table[index2]['Info']['Disk'] = 0 ;
                        host_module_table[index2]['Info']['Memory'] = 0 ;
                        host_module_table[index2]['Error']['Flag'] = statusInfo['errno'] ;
                        host_module_table[index2]['Error']['Message'] = statusInfo['detail'] ;
                        var index3 = hostListIsExist( statusInfo['HostName'] ) ;
                        if( index3 >= 0 )
                        {
                           $scope.HostList[index3]['Error']['Flag'] = statusInfo['errno'] ;
                           $scope.HostList[index3]['Error']['Message'] = statusInfo['detail'] ;
                        }
                     }
                  }
                  else
                  {
                     if( statusInfo['errno'] == 0 || typeof( statusInfo['errno'] ) == 'undefined' )
                     {
                        var index3 = hostListIsExist( statusInfo['HostName'] ) ;
                        if( index3 >= 0 )
                        {
                           $scope.HostList[index3]['Error']['Flag'] = 0 ;
                        }
                     }
                     else
                     {
                        var index3 = hostListIsExist( statusInfo['HostName'] ) ;
                        if( index3 >= 0 )
                        {
                           $scope.HostList[index3]['Error']['Flag'] = statusInfo['errno'] ;
                           $scope.HostList[index3]['Error']['Message'] = statusInfo['detail'] ;
                        }
                     }
                  }
               } ) ;
               countModule_Host() ;
            }
         }, {
            'showLoading': false,
            'delay': 5000,
            'loop': true
         } ) ;
      }

      //获取sequoiadb的节点信息
      var getNodesList = function( moduleIndex ){
         if( $.inArray( moduleIndex, autoQueryModuleIndex ) == -1 )
         {
            return ;
         }
         $scope.moduleList[moduleIndex]['BusinessInfo'] = {} ;
         var moduleName = $scope.moduleList[moduleIndex]['BusinessName'] ;
         var data = { 'cmd': 'list nodes', 'BusinessName': moduleName } ;
         SdbRest.OmOperation( data, {
            'success': function( nodeList ){
               $scope.moduleList[moduleIndex]['BusinessInfo']['NodeList'] = nodeList ;
            },
            'failed': function( errorInfo ){
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  getNodesList( moduleIndex ) ;
                  return true ;
               } ) ;
            }
         }, {
            'showLoading': false
         } ) ;
      } ;

      //获取sequoiadb业务信息
      var getCollectionInfo = function( moduleIndex, clusterIndex ){
         if( $.inArray( moduleIndex, autoQueryModuleIndex ) == -1 )
         {
            return ;
         }

         var clusterName = $scope.moduleList[moduleIndex]['ClusterName'] ;
         var moduleName = $scope.moduleList[moduleIndex]['BusinessName'] ;
         var moduleMode = $scope.moduleList[moduleIndex]['DeployMod'] ;
         var sql ;
         if( moduleMode == 'standalone' )
         {
            sql = 'SELECT t1.Name, t1.Details.TotalIndexPages, t1.Details.PageSize, t1.Details.TotalDataPages, t1.Details.LobPageSize, t1.Details.TotalLobPages FROM (SELECT * FROM $SNAPSHOT_CL split BY Details) AS t1' ;
         }
         else
         {
            sql = 'SELECT t1.Name, t1.Details.TotalIndexPages, t1.Details.PageSize, t1.Details.TotalDataPages, t1.Details.LobPageSize, t1.Details.TotalLobPages FROM (SELECT * FROM $SNAPSHOT_CL WHERE NodeSelect="master" split BY Details) AS t1' ;
         }
         SdbRest.Exec2( clusterName, moduleName, sql, {
            'before': function(){
               if( $.inArray( moduleIndex, autoQueryModuleIndex ) == -1 || clusterIndex != $scope.currentCluster )
               {
                  return false ;
               }
            },
            'success': function( clList ){
               var index = 0 ;
               var data = 0 ;
               var lob = 0 ;
               var indexPercent = 0 ;
               var dataPercent = 0 ;
               var lobPercent = 0 ;
               $.each( clList, function( clIndex, clInfo ){
                  index += clInfo['PageSize'] * clInfo['TotalIndexPages'] ;
                  data += clInfo['PageSize'] * clInfo['TotalDataPages'] ;
                  lob += clInfo['LobPageSize'] * clInfo['TotalLobPages'] ;
               } ) ;
               var sum = index + data + lob ;
               var indexPercent = fixedNumber( index / sum * 100, 2 ) ;
               var dataPercent  = fixedNumber( data / sum * 100, 2 ) ;
               var lobPercent   = 100 - indexPercent - dataPercent ;
               if( isNaN( indexPercent ) || index == 0 )
               {
                  indexPercent = 0 ;
               }
               if( isNaN( dataPercent ) || data == 0 )
               {
                  dataPercent = 0 ;
               }
               if( isNaN( lobPercent ) || lob == 0 )
               {
                  lobPercent = 0 ;
               }
               $scope.moduleList[ moduleIndex ]['Chart']['Module']['value'] = [
                  [ 0, indexPercent, true, false ],
                  [ 1, dataPercent, true, false ],
                  [ 2, lobPercent, true, false ]
               ] ;

               if( $scope.moduleList[moduleIndex]['Error']['Type'] == 'Module cl' )
               {
                  $scope.moduleList[moduleIndex]['Error']['Flag'] = 0 ;
               }
            },
            'failed': function( errorInfo ){
               //if( moduleMode == 'standalone' )
               {
                  if( $scope.moduleList[moduleIndex]['Error']['Type'] == 'Module cl' || $scope.moduleList[moduleIndex]['Error']['Flag'] == 0 )
                  {
                     $scope.moduleList[moduleIndex]['Error']['Flag'] = errorInfo['errno'] ;
                     $scope.moduleList[moduleIndex]['Error']['Type'] = 'Module cl' ;
                     $scope.moduleList[moduleIndex]['Error']['Message'] = sprintf( $scope.autoLanguage( '节点错误: ?，错误码 ?。' ),
                                                                                   errorInfo['description'],
                                                                                   errorInfo['errno'] ) ;
                  }
               }
            }
         }, {
            'showLoading':false,
            'delay': 5000,
            'loop': true
         } ) ;
      }

      //获取sequoiadb的错误节点信息
      var getErrNodes = function( moduleIndex, clusterIndex ){
         if( $.inArray( moduleIndex, autoQueryModuleIndex ) == -1 )
         {
            return ;
         }
         var moduleMode = $scope.moduleList[moduleIndex]['DeployMod'] ;
         if( moduleMode == 'standalone' )
         {
            return ;
         }
         var clusterName = $scope.moduleList[moduleIndex]['ClusterName'] ;
         var moduleName = $scope.moduleList[moduleIndex]['BusinessName'] ;
         var data = { 'cmd': 'snapshot system', 'selector': JSON.stringify( { 'ErrNodes': 1 } ) } ;
         SdbRest.DataOperation2( clusterName, moduleName, data, {
            'before': function(){
               if( $.inArray( moduleIndex, autoQueryModuleIndex ) == -1 || clusterIndex != $scope.currentCluster )
               {
                  return false ;
               }
            },
            'success': function( errNodes ){
               errNodes = errNodes[0]['ErrNodes'] ;
               if( $scope.moduleList[moduleIndex]['Error']['Type'] == 'Module node' || $scope.moduleList[moduleIndex]['Error']['Flag'] == 0 )
               {
                  if( errNodes.length > 0 )
                  {
                     $scope.moduleList[moduleIndex]['Error']['Flag'] = errNodes[0]['Flag'] ;
                     $scope.moduleList[moduleIndex]['Error']['Type'] = 'Module node' ;
                     $scope.moduleList[moduleIndex]['Error']['Message'] = sprintf( $scope.autoLanguage( '节点错误: ?，错误码 ?。' ),
                                                                                   errNodes[0]['NodeName'],
                                                                                   errNodes[0]['Flag'] ) ;
                  }
                  else if( errNodes.length == 0 )
                  {
                     $scope.moduleList[moduleIndex]['Error']['Flag'] = 0 ;
                  }
               }
            }
         }, {
            'showLoading': false,
            'delay': 5000,
            'loop': true
         } ) ;
      }
      //查询业务
      var queryModule = function(){
         var clusterList = [] ;
         var clusterIsExist = function( clusterName ){
            var flag = 0 ;
            var isFind = false ;
            $.each( clusterList, function( index, clusterInfo ){
               if( clusterInfo['ClusterName'] == clusterName )
               {
                  isFind = true ;
                  flag = clusterInfo['index'] ;
                  ++clusterList[index]['index'] ;
                  return false ;
               }
            } ) ;
            if( isFind == false )
            {
               clusterList.push( { 'ClusterName': clusterName, 'index': 1 } ) ;
            }
            return flag ;
         }
         var data = { 'cmd': 'query business' } ;
         SdbRest.OmOperation( data, {
            'success': function( moduleList ){
               $scope.moduleList = moduleList ;
               host_module_table = [] ;
               autoQueryModuleIndex = [] ;
               //获取业务关联信息
               getRelationship() ;
               $.each( $scope.moduleList, function( index, moduleInfo ){

                  var colorId = clusterIsExist( moduleInfo['ClusterName'] ) ;

                  $scope.moduleList[index]['Color'] = colorId ;

                  $scope.moduleList[index]['Error'] = {} ;
                  $scope.moduleList[index]['Error']['Flag'] = 0 ;
                  $scope.moduleList[index]['Error']['Type'] = '' ;
                  $scope.moduleList[index]['Error']['Message'] = '' ;

                  $scope.moduleList[index]['Chart'] = {} ;
                  $scope.moduleList[index]['Chart']['Module'] = {} ;
                  $scope.moduleList[index]['Chart']['Module']['options'] = $.extend( true, {}, window.SdbSacManagerConf.StorageScaleEchart ) ;
                  $scope.moduleList[index]['Chart']['Module']['options']['title']['text'] = $scope.autoLanguage( '元数据比例' ) ;

                  $scope.moduleList[index]['Chart']['Host'] = {} ;
                  $scope.moduleList[index]['Chart']['Host']['CPU'] = { 'percent': 0 } ;
                  $scope.moduleList[index]['Chart']['Memory'] = { 'percent': 0 } ;
                  $scope.moduleList[index]['Chart']['Disk'] = { 'percent': 0 } ;
                  if( isArray( moduleInfo['Location'] ) )
                  {
                     $.each( moduleInfo['Location'], function( index2, hostInfo ){
                        if( hostModuleTableIsExist( hostInfo['HostName'] ) == -1 )
                        {
                           host_module_table.push( { 'HostName': hostInfo['HostName'], 'Info': {}, 'Error': {} } ) ;
                        }
                     } ) ;
                  }
                  if( moduleInfo['BusinessType'] == 'sequoiadb' && moduleInfo['ClusterName'] == $scope.clusterList[ $scope.currentCluster ]['ClusterName'] )
                  {
                     autoQueryModuleIndex.push( index ) ;
                  }
               } ) ;
               $scope.SwitchCluster( $scope.currentCluster ) ;
            },
            'failed': function( errorInfo, retryFun ){
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  retryFun() ;
                  return true ;
               } ) ;
            }
         } ) ;
      }

      //查询主机
      var queryHost = function(){
         var data = { 'cmd': 'query host' } ;
         SdbRest.OmOperation( data, {
            'success': function( hostList ){
               $scope.HostList = hostList ;
               $scope.HostListTable['body'] = $scope.HostList ;
               $.each( $scope.HostList, function( index ){
                  $scope.HostList[index]['Error'] = {} ;
                  $scope.HostList[index]['Error']['Flag'] = 0 ;
               } ) ;
               $scope.SwitchCluster( $scope.currentCluster ) ;
               if( defaultShow == 'host' )
               {
                  $scope.SwitchPage( defaultShow ) ;
               }
            },
            'failed': function( errorInfo, retryFun ){
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  retryFun() ;
                  return true ;
               } ) ;
            }
         } ) ;
      }

      //查询集群
      var queryCluster = function(){
         var data = { 'cmd': 'query cluster' } ;
         SdbRest.OmOperation( data, {
            'success': function( clusterList ){
               $scope.clusterList = clusterList ;
               if( $scope.clusterList.length > 0 )
               {
                  queryHost() ;
                  queryModule() ;
               }
            },
            'failed': function( errorInfo, retryFun ){
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  retryFun() ;
                  return true ;
               } ) ;
            }
         } ) ;
      }

      //跳转到监控业务
      $scope.GotoMonitorModule = function( clusterName, moduleType, moduleMode, moduleName ){
         SdbFunction.LocalData( 'SdbClusterName', clusterName ) ;
         SdbFunction.LocalData( 'SdbModuleType', moduleType ) ;
         SdbFunction.LocalData( 'SdbModuleMode', moduleMode ) ;
         SdbFunction.LocalData( 'SdbModuleName', moduleName ) ;
         switch( moduleType )
         {
         case 'sequoiadb':
            $location.path( '/Monitor/Index' ).search( { 'r': new Date().getTime() } ) ; break ;
         default:
            break ;
         }
      }

      //跳转到监控主机
      $scope.GotoMonitorHost = function( clusterName, moduleType, moduleMode, moduleName ){
         SdbFunction.LocalData( 'SdbClusterName', clusterName ) ;
         SdbFunction.LocalData( 'SdbModuleType', moduleType ) ;
         SdbFunction.LocalData( 'SdbModuleMode', moduleMode ) ;
         SdbFunction.LocalData( 'SdbModuleName', moduleName ) ;
         switch( moduleType )
         {
         case 'sequoiadb':
            $location.path( '/Monitor/SDB-Host/List/Index' ).search( { 'r': new Date().getTime() } ) ; break ;
         default:
            break ;
         }
      }

      //跳转到业务数据
      $scope.GotoDataModule = function( clusterName, moduleType, moduleMode, moduleName ){
         SdbFunction.LocalData( 'SdbClusterName', clusterName ) ;
         SdbFunction.LocalData( 'SdbModuleType', moduleType ) ;
         SdbFunction.LocalData( 'SdbModuleMode', moduleMode ) ;
         SdbFunction.LocalData( 'SdbModuleName', moduleName ) ;
         switch( moduleType )
         {
         case 'sequoiadb':
            $location.path( '/Data/SDB-Database/Index' ).search( { 'r': new Date().getTime() } ) ; break ;
         case 'sequoiasql-oltp':
            $location.path( '/Data/OLTP-Database/Index' ).search( { 'r': new Date().getTime() } ) ; break ;
         case 'sequoiasql':
            $location.path( '/Data/SQL-Metadata/Index' ).search( { 'r': new Date().getTime() } ) ; break ;
         case 'hdfs':
            $location.path( '/Data/HDFS-web/Index' ).search( { 'r': new Date().getTime() } ) ; break ;
         case 'spark':
            $location.path( '/Data/SPARK-web/Index' ).search( { 'r': new Date().getTime() } ) ; break ;
         case 'yarn':
            $location.path( '/Data/YARN-web/Index' ).search( { 'r': new Date().getTime() } ) ; break ;
         default:
            break ;
         }
      }

      //切换业务和主机
      $scope.SwitchPage = function( page ){
         $scope.currentPage = page ;
         $scope.bindResize() ;
      }

      //切换集群
      $scope.SwitchCluster = function( index ){
         $scope.currentCluster = index ;
         if( $scope.clusterList.length > 0 )
         {
            var clusterName = $scope.clusterList[ index ]['ClusterName'] ;
            $scope.ModuleNum = 0 ;
            $scope.DistributionNum = 0 ;
            autoQueryModuleIndex = [] ;
            $.each( $scope.moduleList, function( index2, moduleInfo ){
               if( moduleInfo['ClusterName'] == clusterName )
               {
                  ++$scope.ModuleNum ;
                  autoQueryModuleIndex.push( index2 ) ;
                  if( moduleInfo['BusinessType'] == 'sequoiadb' )
                  {
                     if( moduleInfo['DeployMod'] == 'distribution' )
                     {
                        ++ $scope.DistributionNum ;
                     }
                     getNodesList( index2 ) ;
                     getCollectionInfo( index2, index ) ;
                     getErrNodes( index2, index ) ;
                  }
               }
               
               //查询鉴权
               var data = {
                  'cmd': 'query business authority',
                  'filter': JSON.stringify( { "BusinessName": moduleInfo['BusinessName'] } ) 
               }
               SdbRest.OmOperation( data, {
                  'success': function( authorityResult ){
                     if( authorityResult.length > 0 )
                     {
                        $scope.moduleList[index2]['authority'] = authorityResult[0]['User'] ;
                     }
                  }
               }, {
                  'showLoading': false
               } ) ;
            } ) ;
            $scope.HostNum = 0 ;
            $scope.HostListTable['body'] = [] ;
            $.each( $scope.HostList, function( index2, hostInfo ){
               if( hostInfo['ClusterName'] == clusterName )
               {
                  $scope.HostListTable['body'].push( hostInfo )
                  ++$scope.HostNum ;
               }
            } ) ;
         }
         $scope.bindResize() ;
      }

      //获取业务类型列表
      var GetModuleType = function(){
         var data = { 'cmd': 'list business type' } ;
         SdbRest.OmOperation( data, {
            'success': function( moduleType ){
               $scope.moduleType = moduleType ;
            },
            'failed': function( errorInfo, retryFun ){
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  retryFun() ;
                  return true ;
               } ) ;
            }
         } ) ;
      }

      //全选
      $scope.SelectAll = function(){
         $.each( $scope.HostList, function( index ){
            if( $scope.clusterList[ $scope.currentCluster ]['ClusterName'] == $scope.HostList[index]['ClusterName'] )
            {
               $scope.HostList[index]['checked'] = true ;
            }
         } ) ;
      }

      //反选
      $scope.Unselect = function(){
         $.each( $scope.HostList, function( index ){
            if( $scope.clusterList[ $scope.currentCluster ]['ClusterName'] == $scope.HostList[index]['ClusterName'] )
            {
               $scope.HostList[index]['checked'] = !$scope.HostList[index]['checked'] ;
            }
         } ) ;
      }

      //添加主机
      $scope.AddHost = function(){
         if( $scope.clusterList.length > 0 )
         {
            $rootScope.tempData( 'Deploy', 'Model', 'Host' ) ;
            $rootScope.tempData( 'Deploy', 'Module', 'None' ) ;
            $rootScope.tempData( 'Deploy', 'ClusterName', $scope.clusterList[ $scope.currentCluster ]['ClusterName'] ) ;
            $rootScope.tempData( 'Deploy', 'InstallPath', $scope.clusterList[ $scope.currentCluster ]['InstallPath'] ) ;
            $location.path( '/Deploy/ScanHost' ).search( { 'r': new Date().getTime() } ) ;
         }
      }

      //删除主机
      $scope.RemoveHost = function(){
         if( $scope.clusterList.length > 0 )
         {
            var hostList = [] ;
            $.each( $scope.HostList, function( index ){
               if( $scope.HostList[index]['checked'] == true && $scope.clusterList[ $scope.currentCluster ]['ClusterName'] == $scope.HostList[index]['ClusterName'] )
               {
                  hostList.push( { 'HostName': $scope.HostList[index]['HostName'] } ) ;
               }
            } ) ;
            if( hostList.length > 0 )
            {
               var data = {
                  'cmd': 'remove host',
                  'HostInfo': JSON.stringify( { 'ClusterName': $scope.clusterList[ $scope.currentCluster ]['ClusterName'], 'HostInfo': hostList } )
               } ;
               SdbRest.OmOperation( data, {
                  'success': function( taskInfo ){
                     $rootScope.tempData( 'Deploy', 'Model', 'Task' ) ;
                     $rootScope.tempData( 'Deploy', 'Module', 'None' ) ;
                     $rootScope.tempData( 'Deploy', 'HostTaskID', taskInfo[0]['TaskID'] ) ;
                     $location.path( '/Deploy/InstallHost' ).search( { 'r': new Date().getTime() } ) ;
                  },
                  'failed': function( errorInfo ){
                     _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                        $scope.RemoveHost() ;
                        return true ;
                     } ) ;
                  }
               } ) ;
            }
            else
            {
               $scope.Components.Confirm.type = 3 ;
               $scope.Components.Confirm.context = $scope.autoLanguage( '至少选择一台的主机。' ) ;
               $scope.Components.Confirm.isShow = true ;
               $scope.Components.Confirm.okText = $scope.autoLanguage( '好的' ) ;
            }
         }
      }

      //解绑主机
      $scope.UnbindHost = function(){
         if( $scope.clusterList.length > 0 )
         {
            var hostList = { "HostInfo": [] } ;
            var hostListTips = '' ;
            $.each( $scope.HostList, function( index ){
               if( $scope.HostList[index]['checked'] == true && $scope.clusterList[ $scope.currentCluster ]['ClusterName'] == $scope.HostList[index]['ClusterName'] )
               {
                  hostList['HostInfo'].push( { 'HostName': $scope.HostList[index]['HostName'] } ) ;
               }
            } ) ;
            if( hostList['HostInfo'].length > 0 )
            {
               var clusterName = $scope.clusterList[ $scope.currentCluster ]['ClusterName'] ;
               var data = { 'cmd': 'unbind host', 'ClusterName': clusterName, 'HostInfo': JSON.stringify( hostList ) } ;
               SdbRest.OmOperation( data, {
                  'success': function( taskInfo ){
                     $scope.Components.Confirm.type = 4 ;
                     $scope.Components.Confirm.context = sprintf( $scope.autoLanguage( '主机解绑成功。' ) ) ;
                     $scope.Components.Confirm.isShow = true ;
                     $scope.Components.Confirm.noClose = true ;
                     $scope.Components.Confirm.normalOK = true ;
                     $scope.Components.Confirm.okText = $scope.autoLanguage( '确定' ) ;
                     $scope.Components.Confirm.ok = function(){
                        $scope.Components.Confirm.normalOK = false ;
                        $scope.Components.Confirm.isShow = false ;
                        $scope.Components.Confirm.noClose = false ;
                        queryHost() ;
                     }
                  },
                  'failed': function( errorInfo ){
                     _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                        $scope.UnbindHost() ;
                        return true ;
                     } ) ;
                  }
               } ) ;
            }
            else
            {
               $scope.Components.Confirm.type = 3 ;
               $scope.Components.Confirm.context = $scope.autoLanguage( '至少选择一台的主机。' ) ;
               $scope.Components.Confirm.isShow = true ;
               $scope.Components.Confirm.okText = $scope.autoLanguage( '好的' ) ;
            }
         }
      }

      //前往部署安装包
      $scope.DeployPackage = function(){
         var hostList = [] ;
         $.each( $scope.HostList, function( index ){
            if( $scope.HostList[index]['checked'] == true && $scope.clusterList[ $scope.currentCluster ]['ClusterName'] == $scope.HostList[index]['ClusterName'] )
            {
               hostList.push(
                  {
                     'HostName': $scope.HostList[index]['HostName'],
                     'IP': $scope.HostList[index]['IP'],
                     'User': $scope.HostList[index]['User'],
                     'Packages': $scope.HostList[index]['Packages']
                  } 
               ) ;
            }
         } ) ;
         if( hostList.length > 0 )
         {
            $rootScope.tempData( 'Deploy', 'Model',  'Package' ) ;
            $rootScope.tempData( 'Deploy', 'Module', 'None' ) ;
            $rootScope.tempData( 'Deploy', 'HostList', hostList ) ;
            $rootScope.tempData( 'Deploy', 'ClusterName', $scope.clusterList[ $scope.currentCluster ]['ClusterName'] ) ;
            $location.path( '/Deploy/Package' ).search( { 'r': new Date().getTime() } ) ;
         }
         else
         {
            $scope.Components.Confirm.type = 3 ;
            $scope.Components.Confirm.context = $scope.autoLanguage( '至少选择一台的主机。' ) ;
            $scope.Components.Confirm.isShow = true ;
            $scope.Components.Confirm.okText = $scope.autoLanguage( '好的' ) ;
         }
      }

      //删除主机 下拉菜单
      $scope.RemoveHostDropdown = {
         'config': [],
         'OnClick': function( index ){
            if( index == 0 )
            {
               $scope.RemoveHost() ;
            }
            else if( index == 1 )
            {
               $scope.UnbindHost() ;
            }
            $scope.RemoveHostDropdown['callback']['Close']() ;
         },
         'callback': {}
      }

      //打开 删除主机下拉菜单
      $scope.OpenRemoveHost = function( event ){
         if( $scope.clusterList.length > 0 )
         {
            $scope.RemoveHostDropdown['config'] = [] ;
            $scope.RemoveHostDropdown['config'].push( { 'key': $scope.autoLanguage( '卸载主机' ) } ) ;
            $scope.RemoveHostDropdown['config'].push( { 'key': $scope.autoLanguage( '解绑主机' ) } ) ;
            $scope.RemoveHostDropdown['callback']['Open']( event.currentTarget ) ;
         }
      }

      //编辑主机 下拉菜单
      $scope.EditHostDropdown = {
         'config': [],
         'OnClick': function( index ){
            if( index == 0 )
            {
               $scope.DeployPackage() ;
            }
            else if( index == 1 )
            {
               $scope.ShowUpdateHostIP() ;
            }
            $scope.EditHostDropdown['callback']['Close']() ;
         },
         'callback': {}
      }

      //打开 修改主机下拉菜单
      $scope.OpenEditHostDropdown = function( event ){
         if( $scope.clusterList.length > 0 )
         {
            $scope.EditHostDropdown['config'] = [] ;
            $scope.EditHostDropdown['config'].push( { 'key': $scope.autoLanguage( '部署包' ) } ) ;
            $scope.EditHostDropdown['config'].push( { 'key': $scope.autoLanguage( '更新主机信息' ) } ) ;
            $scope.EditHostDropdown['callback']['Open']( event.currentTarget ) ;
         }
      }

      //业务扩容 弹窗
      $scope.ExtendWindow = {
         'config': {},
         'callback': {}
      } ;

      //打开 业务扩容 弹窗
      $scope.ShowExtendWindow = function(){
         if( $scope.clusterList.length > 0 && $scope.DistributionNum != 0 )
         {
            $scope.ExtendWindow['config'] = {
               inputList: [
                  {
                     "name": 'moduleName',
                     "webName": $scope.autoLanguage( '业务名' ),
                     "type": "select",
                     "value": null,
                     "valid": []
                  }
               ]
            } ;
            var clusterName = $scope.clusterList[ $scope.currentCluster ]['ClusterName'] ;
            $.each( $scope.moduleList, function( index, moduleInfo ){
               if( clusterName == moduleInfo['ClusterName'] && moduleInfo['BusinessType'] == 'sequoiadb' && moduleInfo['DeployMod'] == 'distribution' )
               {
                  if( $scope.ExtendWindow['config']['inputList'][0]['value'] == null )
                  {
                     $scope.ExtendWindow['config']['inputList'][0]['value'] = index ;
                  }
                  $scope.ExtendWindow['config']['inputList'][0]['valid'].push( { 'key': moduleInfo['BusinessName'], 'value': index } )
               }
            } ) ;
            $scope.ExtendWindow['callback']['SetOkButton']( $scope.autoLanguage( '确定' ), function(){
               var isAllClear = $scope.ExtendWindow['config'].check() ;
               if( isAllClear )
               {
                  var formVal = $scope.ExtendWindow['config'].getValue() ;
                  $rootScope.tempData( 'Deploy', 'Model',       'Module' ) ;
                  $rootScope.tempData( 'Deploy', 'Module',      'sequoiadb' ) ;
                  $rootScope.tempData( 'Deploy', 'ModuleName',  $scope.moduleList[ formVal['moduleName'] ]['BusinessName'] ) ;
                  $rootScope.tempData( 'Deploy', 'ClusterName', $scope.clusterList[ $scope.currentCluster ]['ClusterName'] ) ;
                  $rootScope.tempData( 'Deploy', 'DeployMod',   $scope.moduleList[ formVal['moduleName'] ]['DeployMod'] ) ;
                  SdbFunction.LocalData( 'SdbClusterName', $scope.clusterList[ $scope.currentCluster ]['ClusterName'] ) ;
                  SdbFunction.LocalData( 'SdbModuleName',  $scope.moduleList[ formVal['moduleName'] ]['BusinessName'] ) ;
                  $location.path( '/Deploy/SDB-ExtendConf' ).search( { 'r': new Date().getTime() } ) ;
               }
               return isAllClear ;
            } ) ;
            $scope.ExtendWindow['callback']['SetTitle']( $scope.autoLanguage( '业务扩容' ) ) ;
            $scope.ExtendWindow['callback']['SetIcon']( '' ) ;
            $scope.ExtendWindow['callback']['Open']() ;
         }
      }

      //业务减容 弹窗
      $scope.ShrinkWindow = {
         'config': {},
         'callback': {}
      }

      //打开 业务减容 弹窗
      $scope.ShowShrinkWindow = function(){
         if( $scope.clusterList.length > 0 && $scope.DistributionNum != 0 )
         {
            $scope.ShrinkWindow['config'] = {
               inputList: [
                  {
                     "name": 'moduleName',
                     "webName": $scope.autoLanguage( '业务名' ),
                     "type": "select",
                     "value": null,
                     "valid": []
                  }
               ]
            } ;
            var clusterName = $scope.clusterList[ $scope.currentCluster ]['ClusterName'] ;
            $.each( $scope.moduleList, function( index, moduleInfo ){
               if( clusterName == moduleInfo['ClusterName'] && moduleInfo['BusinessType'] == 'sequoiadb' && moduleInfo['DeployMod'] == 'distribution' )
               {
                  if( $scope.ShrinkWindow['config']['inputList'][0]['value'] == null )
                  {
                     $scope.ShrinkWindow['config']['inputList'][0]['value'] = index ;
                  }
                  $scope.ShrinkWindow['config']['inputList'][0]['valid'].push( { 'key': moduleInfo['BusinessName'], 'value': index } )
               }
            } ) ;
            $scope.ShrinkWindow['callback']['SetOkButton']( $scope.autoLanguage( '确定' ), function(){
               var isAllClear = $scope.ShrinkWindow['config'].check() ;
               if( isAllClear )
               {
                  var formVal = $scope.ShrinkWindow['config'].getValue() ;
                  $rootScope.tempData( 'Deploy', 'Model',       'Module' ) ;
                  $rootScope.tempData( 'Deploy', 'Module',      'sequoiadb' ) ;
                  $rootScope.tempData( 'Deploy', 'ModuleName',  $scope.moduleList[ formVal['moduleName'] ]['BusinessName'] ) ;
                  $rootScope.tempData( 'Deploy', 'ClusterName', $scope.clusterList[ $scope.currentCluster ]['ClusterName'] ) ;
                  $rootScope.tempData( 'Deploy', 'Shrink', true ) ;
                  SdbFunction.LocalData( 'SdbModuleName',  $scope.moduleList[ formVal['moduleName'] ]['BusinessName'] ) ;
                  $location.path( '/Deploy/SDB-ShrinkConf' ).search( { 'r': new Date().getTime() } ) ;
               }
               return isAllClear ;
            } ) ;
            $scope.ShrinkWindow['callback']['SetTitle']( $scope.autoLanguage( '业务减容' ) ) ;
            $scope.ShrinkWindow['callback']['SetIcon']( '' ) ;
            $scope.ShrinkWindow['callback']['Open']() ;
         }
      }

      //同步业务 弹窗
      $scope.SyncWindow = {
         'config': {},
         'callback': {}
      }

      //打开 同步业务 弹窗
      $scope.ShowSyncWindow = function(){
         if( $scope.clusterList.length > 0 && $scope.ModuleNum != 0 )
         {
            $scope.SyncWindow['config'] = {
               inputList: [
                  {
                     "name": 'moduleName',
                     "webName": $scope.autoLanguage( '业务名' ),
                     "type": "select",
                     "value": null,
                     "valid": []
                  }
               ]
            } ;
            var clusterName = $scope.clusterList[ $scope.currentCluster ]['ClusterName'] ;
            $.each( $scope.moduleList, function( index, moduleInfo ){
               if( clusterName == moduleInfo['ClusterName'] && moduleInfo['BusinessType'] == 'sequoiadb')
               {
                  if( $scope.SyncWindow['config']['inputList'][0]['value'] == null )
                  {
                     $scope.SyncWindow['config']['inputList'][0]['value'] = index ;
                  }
                  $scope.SyncWindow['config']['inputList'][0]['valid'].push( { 'key': moduleInfo['BusinessName'], 'value': index } )
               }
            } ) ;
            $scope.SyncWindow['callback']['SetOkButton']( $scope.autoLanguage( '确定' ), function(){
               var isAllClear = $scope.SyncWindow['config'].check() ;
               if( isAllClear )
               {
                  var formVal = $scope.SyncWindow['config'].getValue() ;
                  $rootScope.tempData( 'Deploy', 'ModuleName',  $scope.moduleList[ formVal['moduleName'] ]['BusinessName'] ) ;
                  $rootScope.tempData( 'Deploy', 'ClusterName', $scope.clusterList[ $scope.currentCluster ]['ClusterName'] ) ;
                  $rootScope.tempData( 'Deploy', 'InstallPath', $scope.clusterList[ $scope.currentCluster ]['InstallPath'] ) ;
                  $location.path( '/Deploy/SDB-Sync' ).search( { 'r': new Date().getTime() } ) ;
               }
               return isAllClear ;
            } ) ;
            $scope.SyncWindow['callback']['SetTitle']( $scope.autoLanguage( '同步业务' ) ) ;
            $scope.SyncWindow['callback']['SetIcon']( '' ) ;
            $scope.SyncWindow['callback']['Open']() ;
         }
      }

      //添加业务 弹窗
      $scope.InstallModule = {
         'config': {},
         'callback': {}
      } ;
      //打开 添加业务 弹窗
      $scope.ShowInstallModule = function(){
         if( $scope.clusterList.length > 0 )
         {
            if( $scope.HostNum == 0 )
            {
               $scope.Components.Confirm.type = 3 ;
               $scope.Components.Confirm.context = $scope.autoLanguage( '集群还没有安装主机。' ) ;
               $scope.Components.Confirm.isShow = true ;
               $scope.Components.Confirm.okText = $scope.autoLanguage( '安装主机' ) ;
               $scope.Components.Confirm.ok = function(){
                  $scope.AddHost() ;
               }
               return ;
            }
            $scope.InstallModule['config'] = {
               inputList: [
                  {
                     "name": 'moduleName',
                     "webName": $scope.autoLanguage( '业务名' ),
                     "type": "string",
                     "required": true,
                     "value": "",
                     "valid": {
                        "min": 1,
                        "max": 127,
                        "regex": '^[0-9a-zA-Z]+$'
                     }
                  },
                  {
                     "name": 'moduleType',
                     "webName": $scope.autoLanguage( '业务类型' ),
                     "type": "select",
                     "value": 0,
                     "valid": []
                  }
               ]
            } ;
            var num = 1 ;
            var defaultName = '' ;
            while( true )
            {
               var isFind = false ;
               defaultName = sprintf( 'myModule?', num ) ;
               $.each( $scope.moduleList, function( index, moduleInfo ){
                  if( defaultName == moduleInfo['BusinessName'] )
                  {
                     isFind = true ;
                     return false ;
                  }
               } ) ;
               if( isFind == false )
               {
                  $.each( $rootScope.OmTaskList, function( index, taskInfo ){
                     if( taskInfo['Status'] != 4 && defaultName == taskInfo['Info']['BusinessName'] )
                     {
                        isFind = true ;
                        return false ;
                     }
                  } ) ;
                  if( isFind == false )
                  {
                     break ;
                  }
               }
               ++num ;
            }
            $scope.InstallModule['config']['inputList'][0]['value'] = defaultName ;
            $.each( $scope.moduleType, function( index, typeInfo ){
               $scope.InstallModule['config']['inputList'][1]['valid'].push( { 'key': typeInfo['BusinessDesc'], 'value': index } ) ;
            } ) ;
            $scope.InstallModule['callback']['SetOkButton']( $scope.autoLanguage( '确定' ), function(){
               var isAllClear = $scope.InstallModule['config'].check( function( formVal ){
                  var isFind = false ;
                  $.each( $scope.moduleList, function( index, moduleInfo ){
                     if( formVal['moduleName'] == moduleInfo['BusinessName'] )
                     {
                        isFind = true ;
                        return false ;
                     }
                  } ) ;
                  if( isFind == false )
                  {
                     $.each( $rootScope.OmTaskList, function( index, taskInfo ){
                        if( taskInfo['Status'] != 4 && formVal['moduleName'] == taskInfo['Info']['BusinessName'] )
                        {
                           isFind = true ;
                           return false ;
                        }
                     } ) ;
                  }
                  if( isFind == true )
                  {
                     return [ { 'name': 'moduleName', 'error': $scope.autoLanguage( '业务名已经存在' ) } ]
                  }
                  else
                  {
                     return [] ;
                  }
               } ) ;
               if( isAllClear )
               {
                  var formVal = $scope.InstallModule['config'].getValue() ;
                  $rootScope.tempData( 'Deploy', 'Model', 'Module' ) ;
                  $rootScope.tempData( 'Deploy', 'Module', $scope.moduleType[ formVal['moduleType'] ]['BusinessType'] ) ;
                  $rootScope.tempData( 'Deploy', 'ModuleName', formVal['moduleName'] ) ;
                  $rootScope.tempData( 'Deploy', 'ClusterName', $scope.clusterList[ $scope.currentCluster ]['ClusterName'] ) ;
                  if( $scope.moduleType[ formVal['moduleType'] ]['BusinessType'] == 'sequoiadb' )
                  {
                     $location.path( '/Deploy/SDB-Conf' ).search( { 'r': new Date().getTime() } ) ;
                  }
                  //当业务类型是OLTP时
                  else if( $scope.moduleType[ formVal['moduleType'] ]['BusinessType'] == 'sequoiasql-oltp' )
                  {
                     var checkSqlHost = 0 ;
                     $.each( $scope.HostList, function( index, hostInfo ){
                        if( hostInfo['ClusterName'] == $scope.clusterList[ $scope.currentCluster ]['ClusterName'] )
                        {
                           $.each( hostInfo['Packages'], function( packIndex, packInfo ){
                              if( packInfo['Name'] == 'sequoiasql-oltp' )
                              {
                                 ++checkSqlHost ;
                              }
                           } ) ;
                        }
                     } ) ;
                     if( checkSqlHost == 0 )
                     {
                        $scope.Components.Confirm.type = 3 ;
                        $scope.Components.Confirm.context = $scope.autoLanguage( '创建 SequoiaSQL OLTP 业务需要主机已经部署 SequoiaSQL OLTP 包。' ) ;
                        $scope.Components.Confirm.isShow = true ;
                        $scope.Components.Confirm.okText = $scope.autoLanguage( '好的' ) ;
                     }
                     else
                     {
                        var businessConf = {} ;
                        businessConf['ClusterName'] = $scope.clusterList[ $scope.currentCluster ]['ClusterName'] ;
                        businessConf['BusinessName'] = formVal['moduleName'] ;
                        businessConf['BusinessType'] = $scope.moduleType[ formVal['moduleType'] ]['BusinessType'] ;
                        businessConf['DeployMod'] = '' ;
                        businessConf['Property'] = [] ;
                        $rootScope.tempData( 'Deploy', 'ModuleConfig', businessConf ) ;
                        $location.path( '/Deploy/OLTP-Mod' ).search( { 'r': new Date().getTime() } ) ;
                     }
                  }
                  else if( $scope.moduleType[ formVal['moduleType'] ]['BusinessType'] == 'sequoiasql' )
                  {
                     /*
                     var tempHostInfo = [] ;
			            $.each( $scope.HostList, function( index, value ){
                        if( $scope.clusterList[$scope.currentCluster]['ClusterName'] == value['ClusterName'] )
                        {
				               tempHostInfo.push( { 'HostName': value['HostName'] } ) ;
                        }
			            } ) ;
                     var businessConf = {} ;
                     businessConf['ClusterName']  = $scope.clusterList[ $scope.currentCluster ]['ClusterName'] ;
                     businessConf['BusinessName'] = formVal['moduleName'] ;
                     businessConf['BusinessType'] = $scope.moduleType[ formVal['moduleType'] ]['BusinessType'] ;
                     businessConf['DeployMod'] = 'olap' ;
                     businessConf['Property'] = [
                        { "Name": "deploy_standby", "Value": "false" },
                        { "Name": "segment_num", "Value": tempHostInfo.length + '' }
                     ] ;
                     businessConf['HostInfo'] = tempHostInfo ;
                     $rootScope.tempData( 'Deploy', 'ModuleConfig', businessConf ) ;
                     $location.path( '/Deploy/SSQL-Mod' ) ;
                     */
                     $location.path( '/Deploy/SSQL-Conf' ).search( { 'r': new Date().getTime() } ) ;
                  }
                  else if( $scope.moduleType[ formVal['moduleType'] ]['BusinessType'] == 'zookeeper' )
                  {
                     var tempHostInfo = [] ;
			            $.each( $scope.HostList, function( index, value ){
                        if( $scope.clusterList[$scope.currentCluster]['ClusterName'] == value['ClusterName'] )
                        {
				               tempHostInfo.push( { 'HostName': value['HostName'] } ) ;
                        }
			            } ) ;
                     var businessConf = {} ;
                     businessConf['ClusterName']  = $scope.clusterList[ $scope.currentCluster ]['ClusterName'] ;
                     businessConf['BusinessName'] = formVal['moduleName'] ;
                     businessConf['BusinessType'] = $scope.moduleType[ formVal['moduleType'] ]['BusinessType'] ;
                     businessConf['DeployMod'] = 'distribution' ;
                     businessConf['Property'] = [ { 'Name': 'zoonodenum', 'Value': '3' } ] ;
                     businessConf['HostInfo'] = tempHostInfo ;
                     $rootScope.tempData( 'Deploy', 'ModuleConfig', businessConf ) ;
                     $location.path( '/Deploy/ZKP-Mod' ).search( { 'r': new Date().getTime() } ) ;
                  }
               }
               return isAllClear ;
            } ) ;
            $scope.InstallModule['callback']['SetTitle']( $scope.autoLanguage( '创建业务' ) ) ;
            $scope.InstallModule['callback']['SetIcon']( '' ) ;
            $scope.InstallModule['callback']['Open']() ;
         }
      }

      //发现业务 弹窗
      $scope.AppendModule = {
         'config': {},
         'callback': {}
      } ;
      //打开 发现业务 弹窗
      $scope.ShowAppendModule = function(){
         if( $scope.clusterList.length > 0 )
         {
            $scope.AppendModule['config'] = {
               inputList: [
                  {
                     "name": 'moduleName',
                     "webName": $scope.autoLanguage( '业务名' ),
                     "type": "string",
                     "required": true,
                     "value": "",
                     "valid": {
                        "min": 1,
                        "max": 127,
                        "regex": '^[0-9a-zA-Z]+$'
                     }
                  },
                  {
                     "name": 'moduleType',
                     "webName": $scope.autoLanguage( '业务类型' ),
                     "type": "select",
                     "value": 'sequoiadb',
                     "valid": [
                        { 'key': 'SequoiaDB', 'value': 'sequoiadb' },
                        { 'key': 'Spark', 'value': 'spark' },
                        { 'key': 'Hdfs', 'value': 'hdfs' },
                        { 'key': 'Yarn', 'value': 'yarn' }
                     ]
                  }
               ]
            } ;
            var num = 1 ;
            var defaultName = '' ;
            while( true )
            {
               var isFind = false ;
               defaultName = sprintf( 'myModule?', num ) ;
               $.each( $scope.moduleList, function( index, moduleInfo ){
                  if( defaultName == moduleInfo['BusinessName'] )
                  {
                     isFind = true ;
                     return false ;
                  }
               } ) ;
               if( isFind == false )
               {
                  $.each( $rootScope.OmTaskList, function( index, taskInfo ){
                     if( taskInfo['Status'] != 4 && defaultName == taskInfo['Info']['BusinessName'] )
                     {
                        isFind = true ;
                        return false ;
                     }
                  } ) ;
                  if( isFind == false )
                  {
                     break ;
                  }
               }
               ++num ;
            }
            $scope.AppendModule['config']['inputList'][0]['value'] = defaultName ;
            $scope.AppendModule['callback']['SetOkButton']( $scope.autoLanguage( '确定' ), function(){
               var isAllClear = $scope.AppendModule['config'].check( function( formVal ){
                  var isFind = false ;
                  $.each( $scope.moduleList, function( index, moduleInfo ){
                     if( formVal['moduleName'] == moduleInfo['BusinessName'] )
                     {
                        isFind = true ;
                        return false ;
                     }
                  } ) ;
                  if( isFind == false )
                  {
                     $.each( $rootScope.OmTaskList, function( index, taskInfo ){
                        if( taskInfo['Status'] != 4 && formVal['moduleName'] == taskInfo['Info']['BusinessName'] )
                        {
                           isFind = true ;
                           return false ;
                        }
                     } ) ;
                  }
                  if( isFind == true )
                  {
                     return [ { 'name': 'moduleName', 'error': $scope.autoLanguage( '业务名已经存在' ) } ]
                  }
                  else
                  {
                     return [] ;
                  }
               } ) ;
               if( isAllClear )
               {
                  $scope.AppendModule['callback']['Close']() ;
                  var formVal = $scope.AppendModule['config'].getValue() ;
                  if( formVal['moduleType'] == 'sequoiadb' )
                  {
                     setTimeout( function(){
                        $scope.ShowAppendSdb( formVal['moduleName'] ) ;
                        $scope.$apply() ;
                     } ) ;
                  }
                  else if( formVal['moduleType'] == 'sequoiasql' )
                  {
                     setTimeout( function(){
                        $scope.ShowAppendSSQL( formVal['moduleName'] ) ;
                        $scope.$apply() ;
                     } ) ;
                  }
                  else
                  {
                     setTimeout( function(){
                        $scope.ShowAppendOtherModule( formVal['moduleName'], formVal['moduleType'] ) ;
                        $scope.$apply() ;
                     } ) ;
                  }
               }
               else
               {
                  return false ;
               }
            } ) ;
            $scope.AppendModule['callback']['SetTitle']( $scope.autoLanguage( '发现业务' ) ) ;
            $scope.AppendModule['callback']['SetIcon']( '' ) ;
            $scope.AppendModule['callback']['Open']() ;
         }
      }

      //从发现前往添加主机
      var gotoAddHost = function( configure ){
         $rootScope.tempData( 'Deploy', 'Model', 'Deploy' ) ;
         $rootScope.tempData( 'Deploy', 'Module', 'None' ) ;
         $rootScope.tempData( 'Deploy', 'ClusterName', $scope.clusterList[ $scope.currentCluster ]['ClusterName'] ) ;
         $rootScope.tempData( 'Deploy', 'InstallPath', $scope.clusterList[ $scope.currentCluster ]['InstallPath'] ) ;
         $rootScope.tempData( 'Deploy', 'DiscoverConf', configure ) ;
         $location.path( '/Deploy/ScanHost' ).search( { 'r': new Date().getTime() } ) ;
      }

      //发现sdb 弹窗
      $scope.AppendSdb = {
         'config': {},
         'callback': {}
      }

      //打开 发现sdb 弹窗
      $scope.ShowAppendSdb = function( moduleName ){
         $scope.AppendSdb['config'] = {
            inputList: [
               {
                  "name": 'HostName',
                  "webName": $scope.autoLanguage( '地址' ),
                  "type": "string",
                  "required": true,
                  "desc": $scope.autoLanguage( 'coord节点或standalone所在的主机名或者IP地址' ),
                  "value": "",
                  "valid": {
                     "min": 1
                  }
               },
               {
                  "name": 'ServiceName',
                  "webName": $scope.autoLanguage( '服务名' ),
                  "type": "port",
                  "required": true,
                  "desc": $scope.autoLanguage( 'coord节点或standalone端口号' ),
                  "value": '',
                  "valid": {}
               },
               {
                  "name": 'User',
                  "webName": $scope.autoLanguage( '数据库用户名' ),
                  "type": "string",
                  "value": ""
               },
               {
                  "name": 'Passwd',
                  "webName": $scope.autoLanguage( '数据库密码' ),
                  "type": "password",
                  "value": ""
               },
               {
                  "name": 'AgentService',
                  "webName": $scope.autoLanguage( '代理端口' ),
                  "type": "port",
                  "value": '11790',
                  "valid": {}
               }
            ]
         }
         $scope.AppendSdb['callback']['SetOkButton']( $scope.autoLanguage( '确定' ), function(){
            var isAllClear = $scope.AppendSdb['config'].check() ;
            if( isAllClear )
            {
               var formVal = $scope.AppendSdb['config'].getValue() ;
               var configure = {} ;
               configure['ClusterName']  = $scope.clusterList[ $scope.currentCluster ]['ClusterName'] ;
               configure['BusinessType'] = 'sequoiadb' ;
               configure['BusinessName'] = moduleName ;
               configure['BusinessInfo'] = formVal ;
               discoverModule( configure ) ;
            }
            return isAllClear ;
         } ) ;
         $scope.AppendSdb['callback']['SetTitle']( 'SequoiaDB' ) ;
         $scope.AppendSdb['callback']['SetIcon']( '' ) ;
         $scope.AppendSdb['callback']['Open']() ;
      }

      //发现ssql 弹窗
      $scope.AppendSSQL = {
         'config': {},
         'callback': {}
      } ;

      //打开 发现ssql 弹窗
      $scope.ShowAppendSSQL = function( moduleName ){
         $scope.AppendSSQL['config'] = {
            inputList: [
               {
                  "name": 'HostName',
                  "webName": $scope.autoLanguage( '主机名' ),
                  "type": "string",
                  "required": true,
                  "value": "",
                  "valid": {
                     "min": 1
                  }
               },
               {
                  "name": 'ServiceName',
                  "webName": $scope.autoLanguage( '服务名' ),
                  "type": "port",
                  "required": true,
                  "value": '',
                  "valid": {}
               },
               {
                  "name": 'InstallPath',
                  "webName": $scope.autoLanguage( '安装路径' ),
                  "type": "string",
                  "required": true,
                  "value": "",
                  "valid": {
                     "min": 1
                  }
               },
               {
                  "name": 'DbName',
                  "webName": $scope.autoLanguage( '数据库名' ),
                  "type": "string",
                  "required": true,
                  "value": "",
                  "valid": {
                     "min": 1
                  }
               },
               {
                  "name": 'User',
                  "webName": $scope.autoLanguage( '数据库用户名' ),
                  "type": "string",
                  "required": true,
                  "value": "",
                  "valid": {
                     "min": 1
                  }
               },
               {
                  "name": 'Passwd',
                  "webName": $scope.autoLanguage( '数据库密码' ),
                  "type": "string",
                  "required": true,
                  "value": "",
                  "valid": {
                     "min": 1
                  }
               }
            ]
         } ;
         $scope.AppendSSQL['callback']['SetOkButton']( $scope.autoLanguage( '确定' ), function(){
            var isAllClear = $scope.AppendSSQL['config'].check() ;
            if( isAllClear )
            {
               var formVal = $scope.AppendSSQL['config'].getValue() ;
               var configure = {} ;
               configure['ClusterName']  = $scope.clusterList[ $scope.currentCluster ]['ClusterName'] ;
               configure['BusinessType'] = 'sequoiasql' ;
               configure['BusinessName'] = moduleName ;
               configure['BusinessInfo'] = formVal ;
               discoverModule( configure ) ;
            }
            return isAllClear ;
         } ) ;
         $scope.AppendSSQL['callback']['SetTitle']( 'SequoiaSQL' ) ;
         $scope.AppendSSQL['callback']['SetIcon']( '' ) ;
         $scope.AppendSSQL['callback']['Open']() ;
      }
      
      //发现其他业务 弹窗
      $scope.AppendOtherModule = {
         'config': {},
         'callback': {}
      } ;

      //打开 发现其他业务 弹窗
      $scope.ShowAppendOtherModule = function( moduleName, moduleType ){
         $scope.AppendOtherModule['config'] = {
            inputList: [
               {
                  "name": 'HostName',
                  "webName": $scope.autoLanguage( '主机名' ),
                  "type": "string",
                  "required": true,
                  "value": "",
                  "valid": {
                     "min": 1
                  }
               },
               {
                  "name": 'WebServicePort',
                  "webName": $scope.autoLanguage( '服务名' ),
                  "type": "port",
                  "required": true,
                  "value": '',
                  "valid": {}
               }
            ]
         } ;
         $scope.AppendOtherModule['callback']['SetOkButton']( $scope.autoLanguage( '确定' ), function(){
            var isAllClear = $scope.AppendOtherModule['config'].check() ;
            if( isAllClear )
            {
               var formVal = $scope.AppendOtherModule['config'].getValue() ;
               var configure = {} ;
               configure['ClusterName']  = $scope.clusterList[ $scope.currentCluster ]['ClusterName'] ;
               configure['BusinessType'] = moduleType ;
               configure['BusinessName'] = moduleName ;
               configure['BusinessInfo'] = formVal ;
               discoverModule( configure ) ;
            }
            return isAllClear ;
         } ) ;
         $scope.AppendOtherModule['callback']['SetTitle']( moduleType ) ;
         $scope.AppendOtherModule['callback']['SetIcon']( '' ) ;
         $scope.AppendOtherModule['callback']['Open']() ;
      }
      
      //发现业务
      var discoverModule = function( configure ){
         var data = { 'cmd': 'discover business', 'ConfigInfo': JSON.stringify( configure ) } ;
         SdbRest.OmOperation( data, {
            'success': function(){
               if( configure['BusinessType'] == 'sequoiadb' )
               {
                  $rootScope.tempData( 'Deploy', 'ModuleName', configure['BusinessName'] ) ;
                  $rootScope.tempData( 'Deploy', 'ClusterName', configure['ClusterName'] ) ;
                  $location.path( '/Deploy/SDB-Discover' ).search( { 'r': new Date().getTime() }  ) ;
               }
               else
               {
                  $location.path( '/Deploy/Index' ).search( { 'r': new Date().getTime() }  ) ;
               }
            }, 
            'failed': function( errorInfo ){
               if( configure['BusinessType'] == 'sequoiadb' &&
                   isArray( errorInfo['hosts'] ) &&
                   errorInfo['hosts'].length > 0 )
               {
                  $scope.Components.Confirm.type = 3 ;
                  $scope.Components.Confirm.context = $scope.autoLanguage( '发现SequoiaDB需要先在集群中添加该业务的所有主机。是否前往添加主机？' ) ;
                  $scope.Components.Confirm.isShow = true ;
                  $scope.Components.Confirm.okText = $scope.autoLanguage( '是' ) ;
                  $scope.Components.Confirm.ok = function(){
                     $rootScope.tempData( 'Deploy', 'DiscoverHostList', errorInfo['hosts'] ) ;
                     gotoAddHost( configure ) ;
                  }
               }
               else
               {
                  _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                     discoverModule( configure ) ;
                     return true ;
                  } ) ;
               }
               
            }
         } ) ;
      }

      //卸载业务
      var uninstallModule = function( index ){
         if( typeof( $scope.moduleList[index]['AddtionType'] ) == 'undefined' || $scope.moduleList[index]['AddtionType'] != 1 )
         {
            var data = { 'cmd': 'remove business', 'BusinessName': $scope.moduleList[index]['BusinessName'] } ;
            SdbRest.OmOperation( data, {
               'success': function( taskInfo ){
                  $rootScope.tempData( 'Deploy', 'Model', 'Task' ) ;
                  $rootScope.tempData( 'Deploy', 'Module', 'None' ) ;
                  $rootScope.tempData( 'Deploy', 'ModuleTaskID', taskInfo[0]['TaskID'] ) ;
                  $location.path( '/Deploy/InstallModule' ).search( { 'r': new Date().getTime() } ) ;
               },
               'failed': function( errorInfo ){
                  _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                     uninstallModule( index ) ;
                     return true ;
                  } ) ;
               }
            } ) ;
         }
         else
         {
            var data = { 'cmd': 'undiscover business', 'ClusterName': $scope.moduleList[index]['ClusterName'], 'BusinessName': $scope.moduleList[index]['BusinessName'] } ;
            SdbRest.OmOperation( data, {
               'success': function(){
                  $location.path( '/Deploy/Index' ).search( { 'r': new Date().getTime() }  ) ;
               },
               'failed': function( errorInfo ){
                  _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                     uninstallModule( index ) ;
                     return true ;
                  } ) ;
               }
            } ) ;
         }
      }

      //卸载业务 弹窗
      $scope.UninstallModuleWindow = {
         'config': {},
         'callback': {}
      } ;

      //打开 卸载业务 弹窗
      $scope.ShowUninstallModule = function(){
         if( $scope.clusterList.length > 0 )
         {
            if( $scope.ModuleNum == 0 )
            {
               $scope.Components.Confirm.type = 3 ;
               $scope.Components.Confirm.context = $scope.autoLanguage( '已经没有业务了。' ) ;
               $scope.Components.Confirm.isShow = true ;
               $scope.Components.Confirm.okText = $scope.autoLanguage( '好的' ) ;
               return ;
            }
            var clusterName = $scope.clusterList[ $scope.currentCluster ]['ClusterName'] ;
            $scope.UninstallModuleWindow['config'] = {
               'inputList': [
                  {
                     "name": 'moduleIndex',
                     "webName": $scope.autoLanguage( '业务名' ),
                     "type": "select",
                     "value": null,
                     "valid": []
                  }
               ]
            }
            $.each( $scope.moduleList, function( index, moduleInfo ){
               if( clusterName == moduleInfo['ClusterName'] )
               {
                  if( $scope.UninstallModuleWindow['config']['inputList'][0]['value'] == null )
                  {
                     $scope.UninstallModuleWindow['config']['inputList'][0]['value'] = index ;
                  }
                  $scope.UninstallModuleWindow['config']['inputList'][0]['valid'].push( { 'key': moduleInfo['BusinessName'], 'value': index } )
               }
            } ) ;
            $scope.UninstallModuleWindow['callback']['SetOkButton']( $scope.autoLanguage( '确定' ), function(){
               var isAllClear = $scope.UninstallModuleWindow['config'].check() ;
               if( isAllClear )
               {
                  var formVal = $scope.UninstallModuleWindow['config'].getValue() ;
                  uninstallModule( formVal['moduleIndex'] ) ;
               }
               return isAllClear ;
            } ) ;
            $scope.UninstallModuleWindow['callback']['SetTitle']( $scope.autoLanguage( '卸载业务' ) ) ;
            $scope.UninstallModuleWindow['callback']['Open']() ;
         }
      }

      $scope.SwitchParam = function( type ){
         $scope.ShowType = type ;
      }

      //解绑业务
      var unbindModule = function( clusterName, businessName ){
         var data = {
            'cmd': 'unbind business', 'ClusterName': clusterName, 'BusinessName': businessName
         } ;
         SdbRest.OmOperation( data, {
            'success': function(){
               $scope.Components.Confirm.type = 4 ;
               $scope.Components.Confirm.context = sprintf( $scope.autoLanguage( '业务：? 解绑成功。' ), businessName ) ;
               $scope.Components.Confirm.isShow = true ;
               $scope.Components.Confirm.noClose = true ;
               $scope.Components.Confirm.normalOK = true ;
               $scope.Components.Confirm.okText = $scope.autoLanguage( '确定' ) ;
               $scope.Components.Confirm.ok = function(){
                  $scope.Components.Confirm.isShow = false ;
                  $scope.Components.Confirm.noClose = false ;
                  $scope.Components.Confirm.normalOK = false ;
                  $location.path( '/Deploy/Index' ).search( { 'r': new Date().getTime() }  ) ;
               }
            },
            'failed': function( errorInfo ){
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  unbindModule( clusterName, businessName ) ;
                  return true ;
               } ) ;
            }
         } ) ;
      }

      //解绑业务 弹窗
      $scope.UnbindModuleWindow = {
         'config': {},
         'callback': {}
      }

      //打开 解绑业务 弹窗
      $scope.ShowUnbindModule = function(){
         if( $scope.clusterList.length > 0 && $scope.ModuleNum != 0 )
         {
            var clusterName = $scope.clusterList[ $scope.currentCluster ]['ClusterName'] ;
            $scope.UnbindModuleWindow['config'] = {
               'inputList': [
                  {
                     "name": 'moduleIndex',
                     "webName": $scope.autoLanguage( '业务名' ),
                     "type": "select",
                     "value": null,
                     "valid": []
                  }
               ]
            }
            $.each( $scope.moduleList, function( index, moduleInfo ){
               if( clusterName == moduleInfo['ClusterName'] )
               {
                  if( $scope.UnbindModuleWindow['config']['inputList'][0]['value'] == null )
                  {
                     $scope.UnbindModuleWindow['config']['inputList'][0]['value'] = index ;
                  }
                  $scope.UnbindModuleWindow['config']['inputList'][0]['valid'].push( { 'key': moduleInfo['BusinessName'], 'value': index } )
               }
            } ) ;
            $scope.UnbindModuleWindow['callback']['SetOkButton']( $scope.autoLanguage( '确定' ), function(){
               var isAllClear = $scope.UnbindModuleWindow['config'].check() ;
               if( isAllClear )
               {
                  var formVal = $scope.UnbindModuleWindow['config'].getValue() ;
                  var businessName = $scope.moduleList[ formVal['moduleIndex'] ]['BusinessName'] ;
                  var clusterName = $scope.clusterList[ $scope.currentCluster ]['ClusterName'] ;
                  unbindModule( clusterName, businessName ) ;
               }
               return isAllClear ;
            } ) ;
            $scope.UnbindModuleWindow['callback']['SetTitle']( $scope.autoLanguage( '业务解绑' ) ) ;
            $scope.UnbindModuleWindow['callback']['Open']() ;
         }
      
      }

      //创建集群
      var createCluster = function( clusterInfo, success ){
         var data = { 'cmd': 'create cluster', 'ClusterInfo': JSON.stringify( clusterInfo ) } ;
         SdbRest.OmOperation( data, {
            'success': function(){
               success() ;
            },
            'failed': function( errorInfo ){
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  createCluster( clusterInfo, success ) ;
                  return true ;
               } ) ;
            }
         } ) ;
      }

      //创建集群 弹窗
      $scope.CreateClusterWindow = {
         'config': [],
         'callback': {}
      } ;

      //打开 创建集群 弹窗
      $scope.ShowCreateCluster = function(){
         $scope.ShowType = 1 ;
         $scope.CreateClusterWindow['config'] = [
            {
               'inputList': [
                  {
                     "name": 'ClusterName',
                     "webName": $scope.autoLanguage( '集群名' ),
                     "type": "string",
                     "required": true,
                     "value": "",
                     "valid": {
                        'min': 1,
                        'max': 127,
                        'regex': '^[0-9a-zA-Z]+$'
                     }
                  },
                  {
                     'name': 'Desc',
                     'webName': $scope.autoLanguage( '描述' ),
                     'type': 'string',
                     'value': '',
                     'valid': {
                        'min': 0,
                        'max': 1024
                     }
                  },
                  {
                     "name": 'SdbUser',
                     "webName": $scope.autoLanguage( '用户名' ),
                     "type": "string",
                     "required": true,
                     "value": 'sdbadmin',
                     "valid": {
                        'min': 1,
                        'max': 1024
                     }
                  },
                  {
                     "name": 'SdbPasswd',
                     "webName": $scope.autoLanguage( '密码' ),
                     "type": "string",
                     "required": true,
                     "value": 'sdbadmin',
                     "valid": {
                        'min': 1,
                        'max': 1024
                     }
                  },
                  {
                     "name": 'SdbUserGroup',
                     "webName": $scope.autoLanguage( '用户组' ),
                     "type": "string",
                     "required": true,
                     "value": 'sdbadmin_group',
                     "valid": {
                        'min': 1,
                        'max': 1024
                     }
                  },
                  {
                     "name": 'InstallPath',
                     "webName": $scope.autoLanguage( '安装路径' ),
                     "type": "string",
                     "required": true,
                     "value": '/opt/sequoiadb/',
                     "valid": {
                        'min': 1,
                        'max': 2048
                     }
                  }
               ]
            },
            {
               'inputList': [
                  {
                     "name": 'HostFile',
                     "webName": 'HostFile',
                     "type": "switch",
                     "value": true,
                     "desc": $scope.autoLanguage( '是否授权om对系统hosts文件的修改' ),
                     "onChange": function( name, key ){
                        $scope.CreateClusterWindow['config'][1]['inputList'][0]['value'] = !key ;
                     }
                  },
                  {
                     'name': 'RootUser',
                     'webName': 'RootUser',
                     'type': 'switch',
                     'disabled': true,
                     "value": true,
                     "desc": $scope.autoLanguage( '是否允许om使用系统root用户' )
                  }
               ]
            }
         ]
         var num = 1 ;
         var defaultName = '' ;
         while( true )
         {
            var isFind = false ;
            defaultName = sprintf( 'myCluster?', num ) ;
            $.each( $scope.clusterList, function( index, clusterInfo ){
               if( defaultName == clusterInfo['ClusterName'] )
               {
                  isFind = true ;
                  return false ;
               }
            } ) ;
            if( isFind == false )
            {
               break ;
            }
            ++num ;
         }
         $scope.CreateClusterWindow['config'][0]['inputList'][0]['value'] = defaultName ;
         $scope.CreateClusterWindow['callback']['SetOkButton']( $scope.autoLanguage( '确定' ), function(){
            var isAllClear = $scope.CreateClusterWindow['config'][0].check( function( formVal ){
               var isFind = false ;
               $.each( $scope.clusterList, function( index, clusterInfo ){
                  if( formVal['ClusterName'] == clusterInfo['ClusterName'] )
                  {
                     isFind = true ;
                     return false ;
                  }
               } ) ;
               if( isFind == true )
               {
                  return [ { 'name': 'ClusterName', 'error': $scope.autoLanguage( '集群名已经存在' ) } ]
               }
               else
               {
                  return [] ;
               }
            } ) ;
            if( isAllClear )
            {
               var formVal = $scope.CreateClusterWindow['config'][0].getValue() ;
               var formVal2 = $scope.CreateClusterWindow['config'][1].getValue() ;
               formVal['GrantConf'] = [] ;
               formVal['GrantConf'].push( { 'Name': 'HostFile', 'Privilege': formVal2['HostFile'] } ) ;
               formVal['GrantConf'].push( { 'Name': 'RootUser', 'Privilege': formVal2['RootUser'] } ) ;
               createCluster( formVal, function(){
                  $location.path( '/Deploy/Index' ).search( { 'r': new Date().getTime() }  ) ;
               } ) ;
            }
            return isAllClear ;
         } ) ;
         $scope.CreateClusterWindow['callback']['SetTitle']( $scope.autoLanguage( '创建集群' ) ) ;
         $scope.CreateClusterWindow['callback']['Open']() ;
      }

      //删除集群
      var removeCluster = function( clusterIndex ){
         var clusterName = $scope.clusterList[clusterIndex]['ClusterName'] ;
         var data = { 'cmd': 'remove cluster', 'ClusterName': clusterName } ;
         SdbRest.OmOperation( data, {
            'success': function(){
               if( clusterIndex == $scope.currentCluster )
               {
                  $scope.currentCluster = 0 ;
               }
               $location.path( '/Deploy/Index' ).search( { 'r': new Date().getTime() }  ) ;
            },
            'failed': function( errorInfo ){
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  removeCluster( clusterIndex ) ;
                  return true ;
               } ) ;
            }
         } ) ;
      }

      //删除集群 弹窗
      $scope.RemoveClusterWindow = {
         'config': {},
         'callback': {}
      } ;

      //打开 删除集群 弹窗
      $scope.ShowRemoveCluster = function(){
         if( $scope.clusterList.length == 0 )
         {
            return ;
         }
         $scope.RemoveClusterWindow['config'] = {
            'inputList': [
               {
                  "name": 'ClusterName',
                  "webName": $scope.autoLanguage( '集群名' ),
                  "type": "select",
                  "value": $scope.currentCluster,
                  "valid": []
               }
            ]
         } ;
         $.each( $scope.clusterList, function( index ){
            $scope.RemoveClusterWindow['config']['inputList'][0]['valid'].push( { 'key': $scope.clusterList[index]['ClusterName'], 'value': index } ) ;
         } ) ;
         $scope.RemoveClusterWindow['callback']['SetOkButton']( $scope.autoLanguage( '确定' ), function(){
            var isAllClear = $scope.RemoveClusterWindow['config'].check() ;
            if( isAllClear )
            {
               var formVal = $scope.RemoveClusterWindow['config'].getValue() ;
               removeCluster( formVal['ClusterName'] ) ;
            }
            return isAllClear ;
         } ) ;
         $scope.RemoveClusterWindow['callback']['SetTitle']( $scope.autoLanguage( '删除集群' ) ) ;
         $scope.RemoveClusterWindow['callback']['Open']() ;
      }

      //一键部署 弹窗
      $scope.DeployModuleWindow = {
         'config': {},
         'callback': {}
      } ;

      //打开 一键部署 弹窗
      $scope.ShowDeployModule = function(){
         var num = 1 ;
         var defaultName = '' ;
         $scope.ShowType = 1 ;
         while( true )
         {
            var isFind = false ;
            defaultName = sprintf( 'myCluster?', num ) ;
            $.each( $scope.clusterList, function( index, clusterInfo ){
               if( defaultName == clusterInfo['ClusterName'] )
               {
                  isFind = true ;
                  return false ;
               }
            } ) ;
            if( isFind == false )
            {
               break ;
            }
            ++num ;
         }
         $scope.DeployModuleWindow['config'] = [
            { 
               'inputList': [
                  {
                     "name": 'ClusterName',
                     "webName": $scope.autoLanguage( '集群名' ),
                     "type": "string",
                     "required": true,
                     "value": defaultName,
                     "valid": {
                        'min': 1,
                        'max': 127,
                        'regex': '^[0-9a-zA-Z]+$'
                     }
                  },
                  {
                     'name': 'Desc',
                     'webName': $scope.autoLanguage( '描述' ),
                     'type': 'string',
                     'value': '',
                     'valid': {
                        'min': 0,
                        'max': 1024
                     }
                  },
                  {
                     "name": 'SdbUser',
                     "webName": $scope.autoLanguage( '用户名' ),
                     "type": "string",
                     "required": true,
                        "value": 'sdbadmin',
                     "valid": {
                        'min': 1,
                        'max': 1024
                     }
                  },
                  {
                     "name": 'SdbPasswd',
                     "webName": $scope.autoLanguage( '密码' ),
                     "type": "string",
                     "required": true,
                     "value": 'sdbadmin',
                     "valid": {
                        'min': 1,
                        'max': 1024
                     }
                  },
                  {
                     "name": 'SdbUserGroup',
                     "webName": $scope.autoLanguage( '用户组' ),
                     "type": "string",
                     "required": true,
                     "value": 'sdbadmin_group',
                     "valid": {
                        'min': 1,
                        'max': 1024
                     }
                  },
                  {
                     "name": 'InstallPath',
                     "webName": $scope.autoLanguage( '安装路径' ),
                     "type": "string",
                     "required": true,
                     "value": '/opt/sequoiadb/',
                     "valid": {
                        'min': 1,
                        'max': 2048
                     }
                  }
               ]
            },
            {
               'inputList': [
                  {
                     "name": 'moduleName',
                     "webName": $scope.autoLanguage( '业务名' ),
                     "type": "string",
                     "required": true,
                     "value": "myModule",
                     "valid": {
                        "min": 1,
                        "max": 127,
                        "regex": '^[0-9a-zA-Z]+$'
                     }
                  },
                  {
                     "name": 'moduleType',
                     "webName": $scope.autoLanguage( '业务类型' ),
                     "type": "select",
                     "value": 0,
                     "valid": []
                  },
               ]  
            },
            {
               'inputList': [
                  {
                     "name": 'HostFile',
                     "webName": 'HostFile',
                     "type": "switch",
                     "value": true,
                     "desc": $scope.autoLanguage( '是否授权om对系统hosts文件的修改' ),
                     "onChange": function( name, key ){
                        $scope.DeployModuleWindow['config'][2]['inputList'][0]['value'] = !key ;
                     }
                  },
                  {
                     'name': 'RootUser',
                     'webName': 'RootUser',
                     'type': 'switch',
                     'disabled': true,
                     "value": true,
                     "desc": $scope.autoLanguage( '是否允许om使用系统root用户' )
                  }
               ]
            }
         ] ;
         num = 1 ;
         defaultName = '' ;
         while( true )
         {
            var isFind = false ;
            defaultName = sprintf( 'myModule?', num ) ;
            $.each( $scope.moduleList, function( index, moduleInfo ){
               if( defaultName == moduleInfo['BusinessName'] )
               {
                  isFind = true ;
                  return false ;
               }
            } ) ;
            if( isFind == false )
            {
               $.each( $rootScope.OmTaskList, function( index, taskInfo ){
                  if( taskInfo['Status'] != 4 && defaultName == taskInfo['Info']['BusinessName'] )
                  {
                     isFind = true ;
                     return false ;
                  }
               } ) ;
               if( isFind == false )
               {
                  break ;
               }
            }
            ++num ;
         }
         $scope.DeployModuleWindow['config'][1]['inputList'][0]['value'] = defaultName ;
         $.each( $scope.moduleType, function( index, typeInfo ){
            if( typeInfo['BusinessType'] == 'sequoiadb' )
            {
               $scope.DeployModuleWindow['config'][1]['inputList'][1]['valid'].push( { 'key': typeInfo['BusinessDesc'], 'value': index } ) ;
            }
            else
            {
               return ;
            }
         } ) ;
         $scope.DeployModuleWindow['callback']['SetOkButton']( $scope.autoLanguage( '确定' ), function(){
            var isAllClear1 = $scope.DeployModuleWindow['config'][0].check( function( formVal ){
               var rv = [] ;
               var isFind = false ;
               $.each( $scope.clusterList, function( index, clusterInfo ){
                  if( formVal['ClusterName'] == clusterInfo['ClusterName'] )
                  {
                     isFind = true ;
                     return false ;
                  }
               } ) ;
               if( isFind == true )
               {
                  rv.push( { 'name': 'ClusterName', 'error': $scope.autoLanguage( '集群名已经存在' ) } ) ;
               }
               return rv ;
            } ) ;
            var isAllClear2 = $scope.DeployModuleWindow['config'][1].check( function( formVal ){
               var rv = [] ;
               isFind = false ;
               $.each( $scope.moduleList, function( index, moduleInfo ){
                  if( formVal['moduleName'] == moduleInfo['BusinessName'] )
                  {
                     isFind = true ;
                     return false ;
                  }
               } ) ;
               if( isFind == false )
               {
                  $.each( $rootScope.OmTaskList, function( index, taskInfo ){
                     if( taskInfo['Status'] != 4 && formVal['moduleName'] == taskInfo['Info']['BusinessName'] )
                     {
                        isFind = true ;
                        return false ;
                     }
                  } ) ;
               }
               if( isFind == true )
               {
                  rv.push( { 'name': 'moduleName', 'error': $scope.autoLanguage( '业务名已经存在' ) } ) ;
               }
               return rv ;
            } ) ;
            var isAllClear3 = $scope.DeployModuleWindow['config'][2].check() ;
            if( isAllClear1 && isAllClear2 && isAllClear3 )
            {
               var formVal1 = $scope.DeployModuleWindow['config'][0].getValue() ;
               var formVal2 = $scope.DeployModuleWindow['config'][1].getValue() ;
               var formVal3 = $scope.DeployModuleWindow['config'][2].getValue() ;
               $.each( formVal2, function( key, value ){
                  formVal1[key] = value ;
               } ) ;
               $.each( formVal3, function( key, value ){
                  formVal1[key] = value ;
               } ) ;
               createCluster( formVal1, function(){
                  $rootScope.tempData( 'Deploy', 'ClusterName', formVal1['ClusterName'] ) ;
                  $rootScope.tempData( 'Deploy', 'ModuleName', formVal1['moduleName'] ) ;
                  $rootScope.tempData( 'Deploy', 'Model', 'Deploy' ) ;
                  $rootScope.tempData( 'Deploy', 'Module', $scope.moduleType[ formVal1['moduleType'] ]['BusinessType'] ) ;
                  $rootScope.tempData( 'Deploy', 'InstallPath', formVal1['InstallPath'] ) ;
                  $location.path( '/Deploy/ScanHost' ).search( { 'r': new Date().getTime() } ) ;
               } ) ;
            }
            return isAllClear1 && isAllClear2 && isAllClear3 ;
         } ) ;
         $scope.DeployModuleWindow['callback']['SetTitle']( $scope.autoLanguage( '部署' ) ) ;
         $scope.DeployModuleWindow['callback']['Open']() ;
      }

      //设置资源授权
      var setGrant = function( conf ){
         var data = { 'cmd': 'grant sysconf', 'name': conf['name'], 'privilege': conf['privilege'], 'clustername': conf['clustername'] } ;
         SdbRest.OmOperation( data, {
            'success': function(){
               queryCluster();
            },
            'failed': function( errorInfo ){
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  setGrant( conf ) ;
                  return true ;
               } ) ;
            }
         } ) ;
      }

      //通过哪个cluster打开下拉菜单
      var chooseCluster = $scope.currentCluster ;

      //资源授权 弹窗
      $scope.ResourceGrantWindow = {
         'config': {},
         'callback': {}
      }

      //打开 资源授权
      $scope.ShowResourceGrant = function(){
         $scope.ResourceGrantWindow['config'] = {
            'inputList': [
               {
                  "name": 'ClusterName',
                  "webName": $scope.autoLanguage( '集群名' ),
                  "type": "normal",
                  "value": $scope.clusterList[chooseCluster]['ClusterName']
               },
               {
                  "name": 'HostFile',
                  "webName": 'HostFile',
                  "type": "switch",
                  "value": $scope.clusterList[chooseCluster]['GrantConf'][0]['Privilege'],
                  "desc": $scope.autoLanguage( '是否授权om对系统hosts文件的修改' ),
                  "onChange": function( name, key ){
                     $scope.ResourceGrantWindow['config']['inputList'][1]['value'] = !key ;
                     setGrant( { 'name': 'HostFile', 'privilege': !key, 'clustername': $scope.clusterList[chooseCluster]['ClusterName'] } ) ;
                  }
               },
               {
                  'name': 'RootUser',
                  'webName': 'RootUser',
                  'type': 'switch',
                  'disabled': true,
                  "value": true,
                  "desc": $scope.autoLanguage( '是否允许om使用系统root用户' )
               }
            ]
         } ;
         $scope.ResourceGrantWindow['callback']['SetCloseButton']( $scope.autoLanguage( '关闭' ), function(){
            $scope.ResourceGrantWindow['callback']['Close']() ;
         } ) ;
         $scope.ResourceGrantWindow['callback']['SetTitle']( $scope.autoLanguage( '资源授权' ) ) ;
         $scope.ResourceGrantWindow['callback']['Open']() ;
      }

      //逐个更新主机信息
      var updateHostsInfo = function( hostList, index, success ) {
         if( index == hostList.length )
         {
            setTimeout( success ) ;
            return ;
         }

         if( hostList[index]['Flag'] != 0 )
         {
            updateHostsInfo( hostList, index + 1, success ) ;
            return ;
         }

         var hostInfo = {
            'HostInfo' : [
               {
                  'HostName': hostList[index]['HostName'],
                  'IP': hostList[index]['IP']
               }   
            ]
         }
         var data = { 'cmd': 'update host info', 'HostInfo': JSON.stringify( hostInfo ) } ;
         SdbRest.OmOperation( data, {
            'success': function( scanInfo ){
               hostList[index]['Status'] = $scope.autoLanguage( '更新主机信息成功。' ) ;
               $scope.HostList[ hostList[index]['SourceIndex'] ]['IP'] = hostList[index]['IP'] ;
               updateHostsInfo( hostList, index + 1, success ) ;
            },
            'failed': function( errorInfo ){
               Loading.close() ;
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  Loading.create() ;
                  updateHostsInfo( hostList, index, success ) ;
                  return true ;
               } ) ;
            }
         }, {
            'showLoading': false
         } ) ;
      }

      //逐个扫描主机
      var scanHosts = function( hostList, index, success ){
      
         if( index == hostList.length )
         {
            success() ;
            return ;
         }

         if( hostList[index]['IP'] == $scope.HostList[ hostList[index]['SourceIndex'] ]['IP'] )
         {
            hostList[index]['Status'] = $scope.autoLanguage( 'IP地址没有改变，跳过。' ) ;
            scanHosts( hostList, index + 1, success ) ;
            return ;
         }

         hostList[index]['Status'] = $scope.autoLanguage( '正在检测...' ) ;

         var scanHostInfo = [ {
            'IP': hostList[index]['IP'],
            'SshPort': hostList[index]['SshPort'],
            'AgentService': hostList[index]['AgentService']
         } ] ;
         var clusterName = $scope.clusterList[$scope.currentCluster]['ClusterName'] ;
         var clusterUser = $scope.clusterList[$scope.currentCluster]['SdbUser'] ;
         var clusterPwd  = $scope.clusterList[$scope.currentCluster]['SdbPasswd'] ;
         var hostInfo = {
            'ClusterName': clusterName,
            'HostInfo': scanHostInfo,
            'User': clusterUser,
            'Passwd': clusterPwd,
            'SshPort': '-',
            'AgentService': '-'
         } ;
         var data = { 'cmd': 'scan host', 'HostInfo': JSON.stringify( hostInfo ) } ;
         SdbRest.OmOperation( data, {
            'success':function( scanInfo ){
               if( scanInfo[0]['errno'] == -38 || scanInfo[0]['errno'] == 0 )
               {
                  if( scanInfo[0]['HostName'] == hostList[index]['HostName'] )
                  {
                     hostList[index]['Flag'] = 0 ;
                     hostList[index]['Status'] = $scope.autoLanguage( '匹配成功。' ) ;
                  }
                  else
                  {
                     hostList[index]['Status'] = $scope.sprintf( $scope.autoLanguage( '主机名匹配错误，IP地址?的主机名是?。' ), scanInfo[0]['IP'], scanInfo[0]['HostName'] ) ;
                  }
               }
               else
               {
                  hostList[index]['Status'] = $scope.autoLanguage( '错误' ) + ': ' + scanInfo[0]['detail'] ;
               }
               scanHosts( hostList, index + 1, success ) ;
            },
            'failed': function( errorInfo ){
               Loading.close() ;
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  Loading.create() ;
                  scanHosts( hostList, index, success ) ;
                  return true ;
               } ) ;
            }
         }, {
            'showLoading': false
         } ) ;
      }

      //更新主机IP 弹窗
      $scope.UpdateHostIP = {
         'config': {},
         'callback': {}
      } ;

      //打开 更新主机IP 弹窗
      $scope.ShowUpdateHostIP = function(){
         if( $scope.clusterList.length > 0 )
         {
            $scope.UpdateHostList = [] ;
            $.each( $scope.HostList, function( index ){
               if( $scope.HostList[index]['checked'] == true && $scope.clusterList[ $scope.currentCluster ]['ClusterName'] == $scope.HostList[index]['ClusterName'] )
               {
                  $scope.UpdateHostList.push( {
                     'HostName': $scope.HostList[index]['HostName'],
                     'IP': $scope.HostList[index]['IP'],
                     'SshPort': $scope.HostList[index]['SshPort'],
                     'AgentService': $scope.HostList[index]['AgentService'],
                     'Flag': -1,
                     'Status': ( $scope.HostList[index]['Error']['Flag'] == 0 ? '' : $scope.autoLanguage( '错误' ) + ': ' + $scope.HostList[index]['Error']['Message'] ),
                     'SourceIndex': index
                  } ) ;
               }
            } ) ;
            if( $scope.UpdateHostList.length > 0 )
            {
               //设置确定按钮
               $scope.UpdateHostIP['callback']['SetOkButton']( $scope.autoLanguage( '确定' ), function(){
                  Loading.create() ;
                  scanHosts( $scope.UpdateHostList, 0, function(){
                     updateHostsInfo( $scope.UpdateHostList, 0, function(){
                        $scope.$apply() ;
                        Loading.cancel() ;
                     } ) ;
                  } ) ;
                  return false ;
               } ) ;
               //设置标题
               $scope.UpdateHostIP['callback']['SetTitle']( $scope.autoLanguage( '更新主机信息' ) ) ;
               //设置图标
               $scope.UpdateHostIP['callback']['SetIcon']( '' ) ;
               //打开窗口
               $scope.UpdateHostIP['callback']['Open']() ;
            }
            else
            {
               $scope.Components.Confirm.type = 3 ;
               $scope.Components.Confirm.context = $scope.autoLanguage( '至少选择一台的主机。' ) ;
               $scope.Components.Confirm.isShow = true ;
               $scope.Components.Confirm.okText = $scope.autoLanguage( '好的' ) ;
            }
         }
      }

      //设置鉴权 弹窗
      $scope.SetAuthority = {
         'config': {},
         'callback': {}
      } ;

      //表单
      var authorityform = {
         'inputList': [
            {
               "name": "BusinessName",
               "webName": $scope.autoLanguage( '业务名' ),
               "type": "string",
               "disabled": true,
               "value": ''
            },
            {
               "name": "User",
               "webName": $scope.autoLanguage( '用户名' ),
               "type": "string",
               "required": true,
               "value": "",
               "valid": {
                  "min": 1,
                  "max": 127,
                  "regex": '^[0-9a-zA-Z]+$'
               }
            },
            {
               "name": "Password",
               "webName": $scope.autoLanguage( '密码' ),
               "type": "password",
               "required": true,
               "value": "",
               "valid": {
                  "min": 1,
                  "max": 127,
                  "regex": '^[0-9a-zA-Z]+$'
               }
            }
         ]
      } ;

      //保存当前选中的业务名
      var chooseBusinessName = '' ;
      $scope.SaveBsName = function( businessName )
      {
         chooseBusinessName = businessName ;
      }

      //打开 设置鉴权 弹窗
      $scope.ShowSetAuthority = function( businessName, businessType ){
         if( typeof( businessName ) == 'undefined' )
         {
            businessName = chooseBusinessName ;
         }
         
         authorityform['inputList'][1]['value'] = '' ;
         authorityform['inputList'][2]['value'] = '' ;

         if( businessType == 'sequoiasql-oltp' )
         {
            authorityform['inputList'][3] = {
               "name": "DbName",
               "webName": $scope.autoLanguage( '默认数据库' ),
               "type": "string",
               "value": ""
            } ;
         }
         else
         {
            if( typeof( authorityform['inputList'][3] ) != 'undefined' )
            {
               authorityform['inputList'][3] = {} ;
            }
         }

         //关闭鉴权下拉菜单
         $scope.AuthorityDropdown['callback']['Close']() ;
         var form = authorityform ;
         form['inputList'][0]['value'] = businessName ;
         $scope.SetAuthority['config'] = form ;
         $scope.SetAuthority['callback']['SetOkButton']( $scope.autoLanguage( '确定' ), function(){
            var isAllClear = form.check() ;
               if( isAllClear )
               {
                  var formVal = form.getValue() ;
                  var data = {
                     'cmd': 'set business authority',
                     'BusinessName': businessName,
                     'User': formVal['User'],
                     'Passwd': formVal['Password']
                  } ;
                  if( businessType == 'sequoiasql-oltp' && typeof( formVal['DbName'] ) != 'undefined' )
                  {
                     data['DbName'] = formVal['DbName'] ;
                  }
                  SdbRest.OmOperation( data, {
                     'success': function( SetAuthorityResult ){
                        queryModule() ;
                     },
                     'failed': function( errorInfo, retryRun ){
                        _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                           retryRun() ;
                           return true ;
                        } ) ;
                     }
                  }, {
                     'showLoading': false
                  } ) ;
                  $scope.SetAuthority['callback']['Close']() ;
               }
         } ) ;
         $scope.SetAuthority['callback']['SetTitle']( $scope.autoLanguage( '设置鉴权' ) ) ;
         $scope.SetAuthority['callback']['SetIcon']( '' ) ;
         $scope.SetAuthority['callback']['Open']() ;

      }

      //删除鉴权
      $scope.DropAuthorityModel = function( businessName ){
         if( typeof( businessName ) == 'undefined' )
         {
            businessName = chooseBusinessName ;
         }
         //关闭下拉菜单
         $scope.AuthorityDropdown['callback']['Close']() ;
         $scope.Components.Confirm.isShow = true ;
         $scope.Components.Confirm.type = 1 ;
         $scope.Components.Confirm.okText = $scope.autoLanguage( '确定' ) ;
         $scope.Components.Confirm.closeText = $scope.autoLanguage( '取消' ) ;
         $scope.Components.Confirm.title = $scope.autoLanguage( '要删除该业务的鉴权吗？' ) ;
         $scope.Components.Confirm.context = $scope.autoLanguage( '业务名' ) + ': ' + businessName ;
         $scope.Components.Confirm.ok = function(){
            var data = {
               'cmd': 'remove business authority',
               'BusinessName': businessName
            }
            SdbRest.OmOperation( data, {
               'success': function( SetAuthorityResult ){
                  queryModule() ;
               },
               'failed': function( errorInfo, retryRun ){
                  _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                     retryRun() ;
                     return true ;
                  } ) ;
               }
            }, {
               'showLoading': false
            } ) ;
            return true ;
         }
      }

      //鉴权下拉菜单
      $scope.AuthorityDropdown = {
         'config': [
            { 'field': $scope.autoLanguage( '修改鉴权' ), 'value': "edit" },
            { 'field': $scope.autoLanguage( '删除鉴权' ), 'value': "delete" }
         ],
         'callback': {}
      } ;
      
      //打开鉴权下拉菜单
      $scope.OpenShowAuthorityDropdown = function( event, businessName ){
         chooseBusinessName = businessName ;
         $scope.AuthorityDropdown['callback']['Open']( event.currentTarget ) ;
      }

      $scope.GotoExpansion = function(){
         $location.path( '/Deploy/SDB-ExtendConf' ).search( { 'r': new Date().getTime() } ) ;
      }

      //添加业务下拉菜单
      $scope.AddModuleDropdown = {
         'config': [
            { 'key': $scope.autoLanguage( '创建业务' ) },
            { 'key': $scope.autoLanguage( '发现业务' ) }
         ],
         'OnClick': function( index ){
            if( index == 0 )
            {
               $scope.ShowInstallModule() ;
            }
            else
            {
               $scope.ShowAppendModule() ;
            }
            $scope.AddModuleDropdown['callback']['Close']() ;
         },
         'callback': {}
      }

      //打开 添加业务下拉菜单
      $scope.OpenAddModuleDropdown = function( event ){
         if( $scope.clusterList.length > 0 )
         {
            $scope.AddModuleDropdown['callback']['Open']( event.currentTarget ) ;
         }
      }

      //删除业务下拉菜单
      $scope.DeleteModuleDropdown = {
         'config': [
            { 'key': $scope.autoLanguage( '卸载业务' ) },
            { 'key': $scope.autoLanguage( '业务解绑' ) }
         ],
         'OnClick': function( index ){
            if( index == 0 )
            {
               $scope.ShowUninstallModule() ;
            }
            else
            {
               $scope.ShowUnbindModule() ;
            }
            $scope.DeleteModuleDropdown['callback']['Close']() ;
         },
         'callback': {}
      }

      //打开 删除业务下拉菜单
      $scope.OpenDeleteModuleDropdown = function( event ){
         if( $scope.clusterList.length > 0 )
         {
            $scope.DeleteModuleDropdown['callback']['Open']( event.currentTarget ) ;
         }
      }

      //修改业务下拉菜单
      $scope.EditModuleDropdown = {
         'config': [],
         'OnClick': function( index ){
            if( index == 0 )
            {
               $scope.ShowExtendWindow() ;
            }
            else if( index == 1 )
            {
               $scope.ShowShrinkWindow() ;
            }
            else
            {
               $scope.ShowSyncWindow() ;
            }
            $scope.EditModuleDropdown['callback']['Close']() ;
         },
         'callback': {}
      }

      //打开 修改业务下拉菜单
      $scope.OpenEditModuleDropdown = function( event ){
         if( $scope.clusterList.length > 0 )
         {
            $scope.EditModuleDropdown['config'] = [] ;
            var disabled = false ;
            if( $scope.DistributionNum == 0 )
            {
               disabled = true ;
            }
            $scope.EditModuleDropdown['config'].push( { 'key': $scope.autoLanguage( '业务扩容' ), 'disabled': disabled } ) ;
            $scope.EditModuleDropdown['config'].push( { 'key': $scope.autoLanguage( '业务减容' ), 'disabled': disabled } ) ;
            $scope.EditModuleDropdown['config'].push( { 'key': $scope.autoLanguage( '同步业务' ) } ) ;
            $scope.EditModuleDropdown['callback']['Open']( event.currentTarget ) ;
         }
      }

      //创建关联
      var createRelation = function( config ){
         var option = {} ;
         option['transaction']      = config['transaction'] ;
         option['preferedinstance'] = config['preferedinstance'] ;
         option['DbName']           = config['DbName'] ;

         if( config['address'].length > 0 )
         {
            option['address'] = '' ;
            $.each( config['address'], function( index, value ){
               option['address'].length == 0 ? option['address'] = value : option['address'] = option['address'] + ',' + value ;
            } ) ;
         }
         
         var data = {
            'cmd'    : 'create relationship',
            'Name'   : config['Name'],
            'From'   : config['From'],
            'To'     : config['To'],
            'Options': JSON.stringify( option )
         }
         SdbRest.OmOperation( data, {
            'success': function( result ){
               $scope.Components.Confirm.type = 4 ;
               $scope.Components.Confirm.context = sprintf( $scope.autoLanguage( '创建关联成功：?' ), config['Name'] ) ;
               $scope.Components.Confirm.isShow = true ;
               $scope.Components.Confirm.noClose = true ;
               $scope.Components.Confirm.normalOK = true ;
               $scope.Components.Confirm.okText = $scope.autoLanguage( '确定' ) ;
               $scope.Components.Confirm.ok = function(){
                  $scope.Components.Confirm.isShow = false ;
                  $scope.Components.Confirm.noClose = false ;
                  $scope.Components.Confirm.normalOK = false ;
                  getRelationship() ;
               }
            },
            'failed': function( errorInfo ){
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  createRelation( config ) ;
                  return true ;
               } ) ;
            }
         }, {
            'showLoading': false
         } ) ;
      }

      //解除关联
      var removeRelation = function( name ){
         var data = {
            'cmd' : 'remove relationship',
            'Name'  : name
         }
         SdbRest.OmOperation( data, {
            'success': function( SetAuthorityResult ){
               $scope.Components.Confirm.type = 4 ;
               $scope.Components.Confirm.context = sprintf( $scope.autoLanguage( '解除关联成功：?' ), name ) ;
               $scope.Components.Confirm.isShow = true ;
               $scope.Components.Confirm.noClose = true ;
               $scope.Components.Confirm.normalOK = true ;
               $scope.Components.Confirm.okText = $scope.autoLanguage( '确定' ) ;
               $scope.Components.Confirm.ok = function(){
                  $scope.Components.Confirm.isShow = false ;
                  $scope.Components.Confirm.noClose = false ;
                  $scope.Components.Confirm.normalOK = false ;
                  getRelationship() ;
               }
            },
            'failed': function( errorInfo ){
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  removeRelation( name ) ;
                  return true ;
               } ) ;
            }
         }, {
            'showLoading': false
         } ) ;
      }

      //关联操作 下拉菜单
      $scope.RelationDropdown = {
         'config': [],
         'OnClick': function( index ){
            if( index == 0 )
            {
               $scope.OpenCreateRelation() ;
            }
            else if( index == 1 )                
            {
               $scope.OpenRemoveRelation() ;
            }
            $scope.RelationDropdown['callback']['Close']() ;
         },
         'callback': {}
      } ;

      //是否禁用创建关联按钮
      var createRelationDisabled = false ;

      //打开 关联操作 下拉菜单
      $scope.OpenRelationDropdown = function( event ){
         var disabled = false ;
         var sqlModule = 0 ;
         var sdbModule = 0 ;
         $.each( $scope.moduleList, function( index, moduleInfo ){
            if( moduleInfo['BusinessType'] == 'sequoiadb' )
            {
               ++sdbModule ;
            }
            else if( moduleInfo['BusinessType'] == 'sequoiasql-oltp' )
            {
               ++sqlModule ;
            }
         } ) ;
         if( sqlModule == 0 || sdbModule == 0 )
         {
            createRelationDisabled = true ;
         }
         else
         {
            createRelationDisabled = false ;
         }
         $scope.RelationDropdown['config'] = [] ;
         $scope.RelationDropdown['config'].push( { 'key': $scope.autoLanguage( '创建关联' ), 'disabled': createRelationDisabled } ) ;

         if( $scope.RelationshipList.length == 0 )
         {
            disabled = true ;
         }
         $scope.RelationDropdown['config'].push( { 'key': $scope.autoLanguage( '解除关联' ), 'disabled': disabled } ) ;
         $scope.RelationDropdown['callback']['Open']( event.currentTarget ) ;
      }

      //创建关联 弹窗
      $scope.CreateRelationWindow = {
         'config': {},
         'callback': {}
      } ;

      //打开 创建关联 弹窗
      $scope.OpenCreateRelation= function(){
         if( createRelationDisabled == true )
         {
            return ;
         }

         //被关联的业务
         var chooseModule = -1 ;
         
         //选择被关联业务之后查询该业务下的coord节点
         var listCoordNodes = function( index ){
            var eachNodeList = function( nodeList ){
                $.each( nodeList, function( nodeIndex, nodeInfo ){
                  if( nodeInfo['Role'] == 'coord' || nodeInfo['Role'] == 'standalone' )
                  {
                     $scope.CreateRelationWindow['config']['inputList'][7]['valid']['list'].push( {
                        'key': nodeInfo['HostName'] + ':' + nodeInfo['ServiceName'],
                        'value': nodeInfo['HostName'] + ':' + nodeInfo['ServiceName'],
                        'checked': false
                     } ) ;
                  }
                  else
                  {
                     return ;
                  }
               } ) ;
            }
            $scope.CreateRelationWindow['config']['inputList'][7]['valid']['list'] = [] ;
            if( typeof( $scope.moduleList[index]['BusinessInfo'] ) == 'undefined' )
            {
               var moduleName = $scope.moduleList[index]['BusinessName'] ;
               var data = { 'cmd': 'list nodes', 'BusinessName': moduleName } ;
               SdbRest.OmOperation( data, {
                  'success': function( nodeList ){
                     eachNodeList( nodeList ) ;
                  },
                  'failed': function( errorInfo ){
                     _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                        getNodesList( moduleIndex ) ;
                        return true ;
                     } ) ;
                  }
               }, {
                  'showLoading': false
               } ) ;
            }
            else
            {
               eachNodeList( $scope.moduleList[index]['BusinessInfo']['NodeList'] ) ;
            }
         }
         $scope.CreateRelationWindow['config'] = {
            inputList: [
               {
                  "name": 'Name',
                  "webName": $scope.autoLanguage( '关联名' ),
                  "type": "string",
                  "required": true,
                  "value": '',
                  "valid": {
                     "min": 1,
                     "max": 63,
                     "regex": "^[a-zA-Z]+[0-9a-zA-Z_]*$"
                  }
               },
               {
                  "name": "Type",
                  "webName": $scope.autoLanguage( '关联类型' ),
                  "required": true,
                  "type": "select",
                  "value": 0,
                  "valid": [
                     { 'key': 'SequoiaSQL-OLTP - SequoiaDB', 'value': 0 }
                  ]
               },
               {
                  "name": "From",
                  "webName": $scope.autoLanguage( '关联业务名' ),
                  "required": true,
                  "type": "select",
                  "value": '',
                  "valid": [],
                  "onChange": function( name, key, value ){
                     $scope.CreateRelationWindow['config']['inputList'][0]['value'] = sprintf(
                        '?_?_?',
                        value,
                        $scope.moduleList[$scope.CreateRelationWindow['config']['inputList'][3]['value']]['BusinessName'],
                        $scope.CreateRelationWindow['config']['inputList'][4]['value']
                     ) ;
                  }
               },
               {
                  "name": "To",
                  "webName": $scope.autoLanguage( '被关联业务名' ),
                  "required": true,
                  "type": "select",
                  "value": chooseModule,
                  "valid":[],
                  "onChange": function( name, key, value ){
                     listCoordNodes( value ) ;
                     $scope.CreateRelationWindow['config']['inputList'][0]['value'] = sprintf(
                        '?_?_?',
                        $scope.CreateRelationWindow['config']['inputList'][2]['value'],
                        $scope.moduleList[value]['BusinessName'],
                        $scope.CreateRelationWindow['config']['inputList'][4]['value']
                     ) ;
                  }
               },
               {
                  "name": 'DbName',
                  "webName": $scope.autoLanguage( '数据库' ),
                  "type": "string",
                  "required": true,
                  "value": "",
                  "valid": {
                     "min": 1,
                     "max": 63,
                     "regex": "^[a-zA-Z_]+[0-9a-zA-Z_]*$",
                     "regexError": sprintf( $scope.pAutoLanguage( '?由字母和数字或\"_\"组成，并且以字母或\"_\"起头。' ), $scope.pAutoLanguage( '数据表名' ) )
                  },
                  "onChange": function( name, key, value ){
                     $scope.CreateRelationWindow['config']['inputList'][0]['value'] = sprintf(
                        '?_?_?',
                        $scope.CreateRelationWindow['config']['inputList'][2]['value'],
                        $scope.moduleList[$scope.CreateRelationWindow['config']['inputList'][3]['value']]['BusinessName'],
                        key
                     ) ;
                  }
               },
               {
                  "name": "preferedinstance",
                  "webName": "preferedinstance",
                  "type": "select",
                  "value": 'a',
                  "valid":[
                     { 'key': 's', 'value': 's' },
                     { 'key': 'm', 'value': 'm' },
                     { 'key': 'a', 'value': 'a' },
                     { 'key': '1', 'value': '1' },
                     { 'key': '2', 'value': '2' },
                     { 'key': '3', 'value': '3' },
                     { 'key': '4', 'value': '4' },
                     { 'key': '5', 'value': '5' },
                     { 'key': '6', 'value': '6' },
                     { 'key': '7', 'value': '7' }
                  ]
               },
               {
                  "name": "transaction",
                  "webName": "transaction",
                  "type": "select",
                  "value": "off",
                  "valid":[
                     { 'key': 'on', 'value': 'on' },
                     { 'key': 'off', 'value': 'off' }
                  ]
               },
               {
                  "name": "address",
                  "webName": $scope.autoLanguage( '选择被关联节点' ),
                  "type": "multiple",
                  "value": [],
                  "valid": {
                     'min': 0,
                     'list': []
                  }
               }
            ]
         } ;

         
         
         $.each( $scope.moduleList, function( index, moduleInfo ){
            if( moduleInfo['BusinessType'] == 'sequoiasql-oltp' )
            {
               $scope.CreateRelationWindow['config']['inputList'][2]['valid'].push(
                  { 'key': moduleInfo['BusinessName'], 'value': moduleInfo['BusinessName'] }
               ) ;
            }
            else if( moduleInfo['BusinessType'] == 'sequoiadb' )
            {
               //将第一个业务作为默认选项
               if( chooseModule == -1 )
               {
                  chooseModule = index ;
                  $scope.CreateRelationWindow['config']['inputList'][3]['value'] = chooseModule ;
                  listCoordNodes( chooseModule ) ;
               }
               $scope.CreateRelationWindow['config']['inputList'][3]['valid'].push(
                  { 'key': moduleInfo['BusinessName'], 'value': index }
               ) ;
            }
            else
            {
               return ;
            }
         } ) ;
         $scope.CreateRelationWindow['config']['inputList'][2]['value'] = $scope.CreateRelationWindow['config']['inputList'][2]['valid'][0]['value'] ;
         $scope.CreateRelationWindow['config']['inputList'][0]['value'] = sprintf(
            '?_?',
            $scope.CreateRelationWindow['config']['inputList'][2]['value'],
            $scope.moduleList[$scope.CreateRelationWindow['config']['inputList'][3]['value']]['BusinessName']
         ) ;
         $scope.CreateRelationWindow['callback']['SetOkButton']( $scope.autoLanguage( '确定' ), function(){
            var isAllClear = $scope.CreateRelationWindow['config'].check() ;
            if( isAllClear )
            {
               var formVal = $scope.CreateRelationWindow['config'].getValue() ;
               formVal['To'] = $scope.moduleList[formVal['To']]['BusinessName'] ;
               createRelation( formVal ) ;               
            }
            return isAllClear ;
         } ) ;
         $scope.CreateRelationWindow['callback']['SetTitle']( $scope.autoLanguage( '创建关联' ) ) ;
         $scope.CreateRelationWindow['callback']['Open']() ;
      }
      
      //解除关联 弹窗
      $scope.RemoveRelationWindow = {
         'config': {},
         'callback': {}
      } ;

      //打开 解除关联 弹窗
      $scope.OpenRemoveRelation= function(){
         if( $scope.RelationshipList.length == 0 )
         {
            return;
         }

         $scope.RemoveRelationWindow['config'] = {
            inputList: [
               {
                  "name": "Type",
                  "webName": $scope.autoLanguage( '关联类型' ),
                  "required": true,
                  "type": "select",
                  "value": 0,
                  "valid": [
                     { 'key': 'SequoiaSQL-OLTP - SequoiaDB', 'value': 0 }
                  ]
               },
               {
                  "name": "Name",
                  "webName": $scope.autoLanguage( '关联名' ),
                  "type": "select",
                  "required": true,
                  "value": '',
                  "valid": []
               }
            ]
         } ;

         $.each( $scope.RelationshipList, function( index, relationInfo ){
            $scope.RemoveRelationWindow['config']['inputList'][1]['valid'].push(
               { 'key': relationInfo['Name'], 'value': relationInfo['Name'] }
            ) ;
         } ) ;
         $scope.RemoveRelationWindow['config']['inputList'][1]['value'] = $scope.RemoveRelationWindow['config']['inputList'][1]['valid'][0]['value'] ;

         $scope.RemoveRelationWindow['callback']['SetOkButton']( $scope.autoLanguage( '确定' ), function(){
            var isAllClear = $scope.RemoveRelationWindow['config'].check() ;
            if( isAllClear )
            {
               var formVal = $scope.RemoveRelationWindow['config'].getValue() ;
               removeRelation( formVal['Name'] ) ;
            }
            return isAllClear ;
         } ) ;
         $scope.RemoveRelationWindow['callback']['SetTitle']( $scope.autoLanguage( '解除关联' ) ) ;
         $scope.RemoveRelationWindow['callback']['Open']() ;
      }

      //关联信息 弹窗
      $scope.RelationshipWindow = {
         'config': [],
         'callback': {}
      } ;
      
      //打开 关联信息 弹窗
      $scope.ShowRelationship = function( moduleName ){
         $scope.RelationshipWindow['config'] = [] ;
         $.each( $scope.RelationshipList, function( index, info ){
            if( moduleName == info['From'] )
            {
               info['where'] = 'From' ;
            }
            else if( moduleName == info['To'] )
            {
               info['where'] = 'To' ;
            }
            else
            {
               return ;
            }
            $scope.RelationshipWindow['config'].push( info ) ;

         } ) ;
         $scope.RelationshipWindow['callback']['SetTitle']( $scope.autoLanguage( '关联信息' ) ) ;
         $scope.RelationshipWindow['callback']['Open']() ;
      }

      //集群操作下拉菜单
      $scope.ClusterDropdown = {
         'config': [
            { 'key': $scope.autoLanguage( '资源授权' ) },
            { 'key': $scope.autoLanguage( '删除集群' ) }
         ],
         'OnClick': function( index ){
            if( index == 0 )
            {
               $scope.ShowResourceGrant() ;
               $scope.ResourceGrantWindow['config']['inputList'][0]['disabled'] = true ;
               $scope.ResourceGrantWindow['config']['inputList'][1]['value'] = $scope.clusterList[chooseCluster]['GrantConf'][0]['Privilege'] ;
            }
            else
            {
               $scope.Components.Confirm.type = 3 ;
               $scope.Components.Confirm.context = sprintf( $scope.autoLanguage( '是否确定删除集群：?？' ), $scope.clusterList[chooseCluster]['ClusterName'] ) ;
               $scope.Components.Confirm.isShow = true ;
               $scope.Components.Confirm.okText = $scope.autoLanguage( '确定' ) ;
               $scope.Components.Confirm.ok = function(){
                  removeCluster( chooseCluster ) ;
               }
            }
            $scope.ClusterDropdown['callback']['Close']() ;
         },
         'callback': {}
      } ;

      //打开 集群操作下拉菜单
      $scope.OpenClusterDropdown = function( event, index ){
         chooseCluster = index ;
         $scope.ClusterDropdown['callback']['Open']( event.currentTarget ) ;
      }

      //执行
      GetModuleType() ;
      queryCluster() ;
      queryHostStatus() ;

      if( defaultShow == 'host' )
      {
         $scope.SwitchPage( defaultShow ) ;         
      }
   } ) ;
}());