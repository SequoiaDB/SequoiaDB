//@ sourceURL=Deploy.Index.Ctrl.js
//"use strict" ;
(function(){
   var sacApp = window.SdbSacManagerModule ;
   //主控制器
   sacApp.controllerProvider.register( 'Deploy.Index.Ctrl', function( $scope, $location, $rootScope, SdbFunction, SdbRest, SdbSignal, SdbSwap, SdbPromise, Loading ){
      var defaultShow = $rootScope.tempData( 'Deploy', 'Index' ) ;
      //初始化
      $scope.ShowType  = 1 ;
      //集群列表
      $scope.ClusterList = [] ;
      //默认选的cluster
      $scope.CurrentCluster = 0 ;
      //默认显示业务还是主机列表
      $scope.CurrentPage = 'instance' ;
      //主机列表数量
      $scope.HostNum = 0 ;
      //主机列表
      SdbSwap.hostList = [] ;

      //业务列表数量
      $scope.ModuleNum = 0 ;
      //业务列表
      $scope.ModuleList = [] ;

      //实例
      $scope.InstanceNum = 0 ;
      $scope.InstanceList = [] ;
      //存储列表
      $scope.StorageNum = 0 ;
      $scope.StorageList = [] ;
      //业务集群模式数量
      SdbSwap.distributionNum = 0 ;
      //SDB业务数量
      SdbSwap.sdbModuleNum = 0 ;
      //业务类型列表
      SdbSwap.moduleType = [] ;

      //右侧高度偏移量
      $scope.BoxHeight = { 'offsetY': -119 } ;
      //判断如果是Firefox浏览器的话，调整右侧高度偏移量
      var browser = SdbFunction.getBrowserInfo() ;
      if( browser[0] == 'firefox' )
      {
         $scope.BoxHeight = { 'offsetY': -131 } ;
      }

      //关联关系列表
      SdbSwap.relationshipList = [] ;

      //主机和业务的关联表(也就是有安装业务的主机列表)
      var host_module_table = [] ;
      //循环查询的业务
      var autoQueryModuleIndex = [] ;
      //清空Deploy域的数据
      $rootScope.tempData( 'Deploy' ) ;

      SdbSwap.RelationshipPromise = SdbPromise.init( 2, 1 ) ;

      //获取业务关联关系
      SdbSwap.getRelationship = function(){
         var data = { 'cmd': 'list relationship' } ;
         SdbRest.OmOperation( data, {
            'success': function( result ){
               SdbSwap.RelationshipPromise.resolve( 'relationship', result ) ;
               SdbSwap.relationshipList = result ;
               $.each( $scope.ModuleList, function( index, moduleInfo ){
                  moduleInfo['Relationship'] = [] ;
                  $.each( SdbSwap.relationshipList, function( index2, relationInfo ){
                     if( relationInfo['From'] == moduleInfo['BusinessName'] || relationInfo['To'] == moduleInfo['BusinessName'] )
                     {
                        moduleInfo['Relationship'].push( relationInfo ) ;
                     }
                  } ) ;
               } ) ;
            },
            'failed': function( errorInfo ){
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  SdbSwap.getRelationship() ;
                  return true ;
               } ) ;
            }
         }, {
            'showLoading': false
         } ) ;
      }

      SdbSwap.generateModuleName = function( prefix ){
         var num = 1 ;
         var defaultName = '' ;
         while( true )
         {
            var isFind = false ;
            defaultName = sprintf( prefix + '?', num ) ;
            $.each( $scope.ModuleList, function( index, moduleInfo ){
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
         return defaultName ;
      }

      //服务分类
      function classifyModule()
      {
         var instanceList = [] ;
         var storageList = [] ;
         $.each( $scope.ModuleList, function( index, moduleInfo ){
            if( moduleInfo['ClusterName'] == $scope.ClusterList[ $scope.CurrentCluster ]['ClusterName'] )
            {
               if( moduleInfo['BusinessType'] == 'sequoiadb' )
               {
                  moduleInfo['BusinessDesc'] = 'SequoiaDB' ;
                  storageList.push( moduleInfo ) ;
               }
               else
               {
                  if( moduleInfo['BusinessType'] == 'sequoiasql-mysql' )
                  {
                     moduleInfo['BusinessDesc'] = 'MySQL' ;
                  }
                  else if( moduleInfo['BusinessType'] == 'sequoiasql-postgresql' )
                  {
                     moduleInfo['BusinessDesc'] = 'PostgreSQL' ;
                  }
                  instanceList.push( moduleInfo ) ;
               }
            }
         } ) ;

         $scope.InstanceNum = instanceList.length ;
         $scope.InstanceList = [] ;

         $scope.StorageNum = storageList.length ;
         $scope.StorageList = [] ;

         if( $scope.CurrentPage == 'instance' )
         {
            $scope.InstanceList = instanceList ;
         }
         else if( $scope.CurrentPage == 'storage' )
         {
            $scope.StorageList = storageList ;
         }
      }

      //计算每个服务的资源
      function countModule_Host()
      {
         $.each( $scope.ModuleList, function( index, moduleInfo ){
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
                     if( $scope.ModuleList[index]['Error']['Type'] == 'Host' || $scope.ModuleList[index]['Error']['Flag'] == 0 )
                     {
                        if( host_module_table[index3]['Error']['Flag'] == 0 )
                        {
                           $scope.ModuleList[index]['Error']['Flag'] = 0 ;
                        }
                        else
                        {
                           $scope.ModuleList[index]['Error']['Flag'] = host_module_table[index3]['Error']['Flag'] ;
                           $scope.ModuleList[index]['Error']['Type'] = 'Host' ;
                           $scope.ModuleList[index]['Error']['Message'] = sprintf( $scope.autoLanguage( '主机 ? 状态异常: ?。' ), 
                                                                                   host_module_table[index3]['HostName'],
                                                                                   host_module_table[index3]['Error']['Message'] ) ;
                        }
                     }
                  }
               } ) ;
               $scope.ModuleList[index]['Chart']['Host']['CPU'] = { 'percent': fixedNumber( cpu / length, 2 ), 'style': { 'progress': { 'background': '#87CEFA' } } } ;
               $scope.ModuleList[index]['Chart']['Host']['Memory'] = { 'percent': fixedNumber( memory / length, 2 ), 'style': { 'progress': { 'background': '#DDA0DD' } } } ;
               $scope.ModuleList[index]['Chart']['Host']['Disk'] = { 'percent':  fixedNumber( disk / length, 2 ), 'style': { 'progress': { 'background': '#FFA07A' } } } ;
            }
         } ) ;
      }
      
      //host_module_table是否已经存在该主机
      function hostModuleTableIsExist( hostName )
      {
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

      //SdbSwap.hostList是否存在该主机
      function hostListIsExist( hostName )
      {
         var flag = -1 ;
         $.each( SdbSwap.hostList, function( index, hostInfo ){
            if( hostInfo['HostName'] == hostName )
            {
               flag = index ;
               return false ;
            }
         } ) ;
         return flag ;
      }

      //查询主机状态
      function queryHostStatus()
      {
         var isFirstQueryHostStatus = true ;
         var queryHostList ;
         SdbRest.OmOperation( null, {
            'init': function(){
               queryHostList = { 'HostInfo': [] } ;
               if( typeof( SdbSwap.hostList ) == 'undefined' )
               {
                  SdbSwap.hostList = [] ;
               }
               $.each( SdbSwap.hostList, function( index, hostInfo ){
                  if( isFirstQueryHostStatus || hostInfo['ClusterName'] == $scope.ClusterList[ $scope.CurrentCluster ]['ClusterName'] )
                  {
                     if( typeof( hostInfo['HostName'] ) == 'undefined' )
                     {
                        return ;
                     }
                     queryHostList['HostInfo'].push( { 'HostName': hostInfo['HostName'] } ) ;
                  }
               } ) ;
               isFirstQueryHostStatus = false ;
               return { 'cmd': 'query host status', 'HostInfo': JSON.stringify( queryHostList ) } ;
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
                           SdbSwap.hostList[index3]['Error']['Flag'] = 0 ;
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
                           SdbSwap.hostList[index3]['Error']['Flag'] = statusInfo['errno'] ;
                           SdbSwap.hostList[index3]['Error']['Message'] = statusInfo['detail'] ;
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
                           SdbSwap.hostList[index3]['Error']['Flag'] = 0 ;
                        }
                     }
                     else
                     {
                        var index3 = hostListIsExist( statusInfo['HostName'] ) ;
                        if( index3 >= 0 )
                        {
                           SdbSwap.hostList[index3]['Error']['Flag'] = statusInfo['errno'] ;
                           SdbSwap.hostList[index3]['Error']['Message'] = statusInfo['detail'] ;
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
      function getNodesList( moduleIndex )
      {
         if( $.inArray( moduleIndex, autoQueryModuleIndex ) == -1 )
         {
            return ;
         }
         $scope.ModuleList[moduleIndex]['BusinessInfo'] = {} ;
         var moduleName = $scope.ModuleList[moduleIndex]['BusinessName'] ;
         var data = { 'cmd': 'list nodes', 'BusinessName': moduleName } ;
         SdbRest.OmOperation( data, {
            'success': function( nodeList ){
               $scope.ModuleList[moduleIndex]['BusinessInfo']['NodeList'] = nodeList ;
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

      //获取sequoiadb服务信息
      function getCollectionInfo( moduleIndex, clusterIndex )
      {
         if( $.inArray( moduleIndex, autoQueryModuleIndex ) == -1 )
         {
            return ;
         }

         var clusterName = $scope.ModuleList[moduleIndex]['ClusterName'] ;
         var moduleName = $scope.ModuleList[moduleIndex]['BusinessName'] ;
         var moduleMode = $scope.ModuleList[moduleIndex]['DeployMod'] ;
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
               if( $.inArray( moduleIndex, autoQueryModuleIndex ) == -1 || clusterIndex != $scope.CurrentCluster )
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
               $scope.ModuleList[ moduleIndex ]['Chart']['Module']['value'] = [
                  [ 0, indexPercent, true, false ],
                  [ 1, dataPercent, true, false ],
                  [ 2, lobPercent, true, false ]
               ] ;

               if( $scope.ModuleList[moduleIndex]['Error']['Type'] == 'Module cl' )
               {
                  $scope.ModuleList[moduleIndex]['Error']['Flag'] = 0 ;
               }
            },
            'failed': function( errorInfo ){
               //if( moduleMode == 'standalone' )
               {
                  if( $scope.ModuleList[moduleIndex]['Error']['Type'] == 'Module cl' || $scope.ModuleList[moduleIndex]['Error']['Flag'] == 0 )
                  {
                     $scope.ModuleList[moduleIndex]['Error']['Flag'] = errorInfo['errno'] ;
                     $scope.ModuleList[moduleIndex]['Error']['Type'] = 'Module cl' ;
                     $scope.ModuleList[moduleIndex]['Error']['Message'] = sprintf( $scope.autoLanguage( '节点错误: ?，错误码 ?。' ),
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
      function getErrNodes( moduleIndex, clusterIndex )
      {
         if( $.inArray( moduleIndex, autoQueryModuleIndex ) == -1 )
         {
            return ;
         }
         var moduleMode = $scope.ModuleList[moduleIndex]['DeployMod'] ;
         if( moduleMode == 'standalone' )
         {
            return ;
         }
         var clusterName = $scope.ModuleList[moduleIndex]['ClusterName'] ;
         var moduleName = $scope.ModuleList[moduleIndex]['BusinessName'] ;
         var data = { 'cmd': 'snapshot system', 'selector': JSON.stringify( { 'ErrNodes': 1 } ) } ;
         SdbRest.DataOperation2( clusterName, moduleName, data, {
            'before': function(){
               if( $.inArray( moduleIndex, autoQueryModuleIndex ) == -1 || clusterIndex != $scope.CurrentCluster )
               {
                  return false ;
               }
            },
            'success': function( errNodes ){
               errNodes = errNodes[0]['ErrNodes'] ;
               if( $scope.ModuleList[moduleIndex]['Error']['Type'] == 'Module node' || $scope.ModuleList[moduleIndex]['Error']['Flag'] == 0 )
               {
                  if( errNodes.length > 0 )
                  {
                     $scope.ModuleList[moduleIndex]['Error']['Flag'] = errNodes[0]['Flag'] ;
                     $scope.ModuleList[moduleIndex]['Error']['Type'] = 'Module node' ;
                     $scope.ModuleList[moduleIndex]['Error']['Message'] = sprintf( $scope.autoLanguage( '节点错误: ?，错误码 ?。' ),
                                                                                   errNodes[0]['NodeName'],
                                                                                   errNodes[0]['Flag'] ) ;
                  }
                  else if( errNodes.length == 0 )
                  {
                     $scope.ModuleList[moduleIndex]['Error']['Flag'] = 0 ;
                  }
               }
            }
         }, {
            'showLoading': false,
            'delay': 5000,
            'loop': true
         } ) ;
      }

      function clusterIsExist( clusterList, type, clusterName )
      {
         var flag = 0 ;
         var isFind = false ;
         $.each( clusterList, function( index, clusterInfo ){
            if( clusterInfo['ClusterName'] == clusterName && clusterInfo['type'] == type )
            {
               isFind = true ;
               flag = clusterInfo['index'] ;
               ++clusterList[index]['index'] ;
               return false ;
            }
         } ) ;
         if( isFind == false )
         {
            clusterList.push( { 'ClusterName': clusterName, 'type': type, 'index': 1 } ) ;
         }
         return flag ;
      }

      //查询服务
      SdbSwap.queryModule = function(){
         var clusterList = [] ;
         var data = { 'cmd': 'query business' } ;
         SdbRest.OmOperation( data, {
            'success': function( moduleList ){
               SdbSwap.RelationshipPromise.resolve( 'moduleList', moduleList ) ;
               $scope.ModuleList = moduleList ;

               classifyModule() ;

               if( $scope.InstanceNum == 0 && $scope.StorageNum == 0 )
               {
                  $scope.CurrentPage = 'storage' ;
               }

               host_module_table = [] ;
               autoQueryModuleIndex = [] ;
               //获取服务关联信息
               SdbSwap.getRelationship() ;
               $.each( $scope.ModuleList, function( index, moduleInfo ){

                  var colorId = clusterIsExist( clusterList, moduleInfo['BusinessType'] == 'sequoiadb' ? 1 : 2, moduleInfo['ClusterName'] ) ;

                  $scope.ModuleList[index]['Color'] = colorId ;

                  $scope.ModuleList[index]['Error'] = {} ;
                  $scope.ModuleList[index]['Error']['Flag'] = 0 ;
                  $scope.ModuleList[index]['Error']['Type'] = '' ;
                  $scope.ModuleList[index]['Error']['Message'] = '' ;

                  $scope.ModuleList[index]['Chart'] = {} ;
                  $scope.ModuleList[index]['Chart']['Module'] = {} ;
                  $scope.ModuleList[index]['Chart']['Module']['options'] = $.extend( true, {}, window.SdbSacManagerConf.StorageScaleEchart ) ;
                  $scope.ModuleList[index]['Chart']['Module']['options']['title']['text'] = $scope.autoLanguage( '元数据比例' ) ;

                  $scope.ModuleList[index]['Chart']['Host'] = {} ;
                  $scope.ModuleList[index]['Chart']['Host']['CPU'] = { 'percent': 0 } ;
                  $scope.ModuleList[index]['Chart']['Memory'] = { 'percent': 0 } ;
                  $scope.ModuleList[index]['Chart']['Disk'] = { 'percent': 0 } ;
                  if( isArray( moduleInfo['Location'] ) )
                  {
                     $.each( moduleInfo['Location'], function( index2, hostInfo ){
                        if( hostModuleTableIsExist( hostInfo['HostName'] ) == -1 )
                        {
                           host_module_table.push( { 'HostName': hostInfo['HostName'], 'Info': {}, 'Error': {} } ) ;
                        }
                     } ) ;
                  }
                  if( moduleInfo['BusinessType'] == 'sequoiadb' && moduleInfo['ClusterName'] == $scope.ClusterList[ $scope.CurrentCluster ]['ClusterName'] )
                  {
                     autoQueryModuleIndex.push( index ) ;
                  }
               } ) ;
               $scope.SwitchCluster( $scope.CurrentCluster ) ;
            },
            'failed': function( errorInfo ){
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  SdbSwap.queryModule() ;
                  return true ;
               } ) ;
            }
         } ) ;
      }

      //查询集群
      var queryCluster = function(){
         var data = { 'cmd': 'query cluster' } ;
         SdbRest.OmOperation( data, {
            'success': function( ClusterList ){
               $scope.ClusterList = ClusterList ;
               if( $scope.ClusterList.length > 0 )
               {
                  SdbSignal.commit( 'queryHost' ) ;
                  SdbSwap.queryModule() ;
               }
               else
               {
                  $scope.CurrentPage = 'storage' ;
               }
            },
            'failed': function( errorInfo ){
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  queryCluster() ;
                  return true ;
               } ) ;
            }
         } ) ;
      }

      //跳转到服务数据
      $scope.GotoDataModule = function( clusterName, moduleType, moduleMode, moduleName ){
         SdbFunction.LocalData( 'SdbClusterName', clusterName ) ;
         SdbFunction.LocalData( 'SdbModuleType', moduleType ) ;
         SdbFunction.LocalData( 'SdbModuleMode', moduleMode ) ;
         SdbFunction.LocalData( 'SdbModuleName', moduleName ) ;
         switch( moduleType )
         {
         case 'sequoiadb':
            $location.path( '/Data/SDB-Database/Index' ).search( { 'r': new Date().getTime() } ) ; break ;
         case 'sequoiasql-postgresql':
            $location.path( '/Data/SequoiaSQL/PostgreSQL/Database/Index' ).search( { 'r': new Date().getTime() } ) ; break ;
         case 'sequoiasql-mysql':
            $location.path( '/Data/SequoiaSQL/MySQL/Database/Index' ).search( { 'r': new Date().getTime() } ) ; break ;
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

      //切换服务和主机
      $scope.SwitchPage = function( page ){
         $scope.CurrentPage = page ;
         classifyModule() ;
         $scope.bindResize() ;
      }

      //查询鉴权
      SdbSwap.queryAuth = function( businessName ){
         //查询鉴权
         var data = {
            'cmd': 'query business authority',
            'filter': JSON.stringify( { "BusinessName": businessName } ) 
         }
         SdbRest.OmOperation( data, {
            'success': function( authorityResult ){
               var index = -1 ;
               $.each( $scope.ModuleList, function( index2, info ){
                  if( info['BusinessName'] == businessName )
                  {
                     index = index2 ;
                     return false ;
                  }
               } ) ;
               if( index >= 0 )
               {
                  if( authorityResult.length > 0 )
                  {
                     $scope.ModuleList[index]['authority'] = authorityResult ;
                  }
                  else
                  {
                     $scope.ModuleList[index]['authority'] = [{}] ;
                  }
               }
            }
         }, {
            'showLoading': false
         } ) ;
      }

      //更新主机数量
      SdbSignal.on( 'updateHostNum', function( hostNum ){
         $scope.HostNum = hostNum ;
      } ) ;

      //切换集群
      $scope.SwitchCluster = function( index ){
         $scope.CurrentCluster = index ;
         classifyModule() ;
         if( $scope.InstanceNum == 0 && $scope.StorageNum == 0 && $scope.CurrentPage == 'instance' )
         {
            $scope.CurrentPage = 'storage' ;
         }
         if( $scope.ClusterList.length > 0 )
         {
            var clusterName = $scope.ClusterList[ index ]['ClusterName'] ;
            $scope.ModuleNum = 0 ;
            SdbSwap.distributionNum = 0 ;
            SdbSwap.sdbModuleNum = 0 ;
            autoQueryModuleIndex = [] ;
            $.each( $scope.ModuleList, function( index2, moduleInfo ){
               if( moduleInfo['ClusterName'] == clusterName )
               {
                  ++$scope.ModuleNum ;
                  autoQueryModuleIndex.push( index2 ) ;
                  if( moduleInfo['BusinessType'] == 'sequoiadb' )
                  {
                     if( moduleInfo['DeployMod'] == 'distribution' )
                     {
                        ++SdbSwap.distributionNum ;
                     }
                     ++SdbSwap.sdbModuleNum ;
                     getNodesList( index2 ) ;
                     getCollectionInfo( index2, index ) ;
                     getErrNodes( index2, index ) ;
                  }

                  if( moduleInfo['BusinessType'] == 'sequoiadb' || moduleInfo['BusinessType'] == 'sequoiasql-postgresql' || moduleInfo['BusinessType'] == 'sequoiasql-mysql' )
                  {
                     SdbSwap.queryAuth( moduleInfo['BusinessName'] ) ;
                  }
               }
            } ) ;

            $scope.HostNum = 0 ;
            var hostTableContent = [] ;
            $.each( SdbSwap.hostList, function( index2, hostInfo ){
               if( hostInfo['ClusterName'] == clusterName )
               {
                  hostTableContent.push( hostInfo )
                  ++$scope.HostNum ;
               }
            } ) ;
            SdbSignal.commit( 'updateHostNum', $scope.HostNum ) ;
            SdbSignal.commit( 'updateHostTable', hostTableContent ) ;
         }
         $scope.bindResize() ;
      }

      //获取服务类型列表
      function GetModuleType()
      {
         var data = { 'cmd': 'list business type' } ;
         SdbRest.OmOperation( data, {
            'success': function( moduleType ){
               SdbSwap.moduleType = moduleType ;
            },
            'failed': function( errorInfo ){
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  GetModuleType() ;
                  return true ;
               } ) ;
            }
         } ) ;
      }

      //切换创建集群弹窗的tab
      $scope.SwitchParam = function( type ){
         $scope.ShowType = type ;
      }

      //创建集群
      function createCluster( clusterInfo, success )
      {
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
                        'regex': '^[0-9a-zA-Z_-]+$'
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
                     "desc": $scope.autoLanguage( '用户名和密码需要与安装 SequoiaDB 的用户名和密码一致' ),
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
                     "desc": $scope.autoLanguage( '用户名和密码需要与安装 SequoiaDB 的用户名和密码一致' ),
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
            $.each( $scope.ClusterList, function( index, clusterInfo ){
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
               $.each( $scope.ClusterList, function( index, clusterInfo ){
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
      function removeCluster( clusterIndex )
      {
         var clusterName = $scope.ClusterList[clusterIndex]['ClusterName'] ;
         var data = { 'cmd': 'remove cluster', 'ClusterName': clusterName } ;
         SdbRest.OmOperation( data, {
            'success': function(){
               if( clusterIndex == $scope.CurrentCluster )
               {
                  $scope.CurrentCluster = 0 ;
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
         if( $scope.ClusterList.length == 0 )
         {
            return ;
         }
         $scope.RemoveClusterWindow['config'] = {
            'inputList': [
               {
                  "name": 'ClusterName',
                  "webName": $scope.autoLanguage( '集群名' ),
                  "type": "select",
                  "value": $scope.CurrentCluster,
                  "valid": []
               }
            ]
         } ;
         $.each( $scope.ClusterList, function( index ){
            $scope.RemoveClusterWindow['config']['inputList'][0]['valid'].push( { 'key': $scope.ClusterList[index]['ClusterName'], 'value': index } ) ;
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
            $.each( $scope.ClusterList, function( index, clusterInfo ){
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
                        'regex': '^[0-9a-zA-Z_-]+$'
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
                     "desc": $scope.autoLanguage( '用户名和密码需要与安装 SequoiaDB 的用户名和密码一致' ),
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
                     "desc": $scope.autoLanguage( '用户名和密码需要与安装 SequoiaDB 的用户名和密码一致' ),
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
                     "webName": $scope.autoLanguage( '存储集群名' ),
                     "type": "string",
                     "required": true,
                     "value": "myService",
                     "valid": {
                        "min": 1,
                        "max": 127,
                        'regex': '^[0-9a-zA-Z_-]+$'
                     }
                  },
                  {
                     "name": 'moduleType',
                     "webName": $scope.autoLanguage( '类型' ),
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

         $scope.DeployModuleWindow['config'][1]['inputList'][0]['value'] = SdbSwap.generateModuleName( 'SequoiaDB' ) ; ;
         $.each( SdbSwap.moduleType, function( index, typeInfo ){
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
               $.each( $scope.ClusterList, function( index, clusterInfo ){
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
               $.each( $scope.ModuleList, function( index, moduleInfo ){
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
                  rv.push( { 'name': 'moduleName', 'error': $scope.autoLanguage( '存储集群名已经存在' ) } ) ;
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
                  $rootScope.tempData( 'Deploy', 'Module', SdbSwap.moduleType[ formVal1['moduleType'] ]['BusinessType'] ) ;
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
      function setGrant( conf )
      {
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
      var chooseCluster = $scope.CurrentCluster ;

      //资源授权 弹窗
      $scope.ResourceGrantWindow = {
         'config': {},
         'callback': {}
      }

      //打开 资源授权
      function showResourceGrant()
      {
         $scope.ResourceGrantWindow['config'] = {
            'inputList': [
               {
                  "name": 'ClusterName',
                  "webName": $scope.autoLanguage( '集群名' ),
                  "type": "normal",
                  "value": $scope.ClusterList[chooseCluster]['ClusterName']
               },
               {
                  "name": 'HostFile',
                  "webName": 'HostFile',
                  "type": "switch",
                  "value": $scope.ClusterList[chooseCluster]['GrantConf'][0]['Privilege'],
                  "desc": $scope.autoLanguage( '是否授权om对系统hosts文件的修改' ),
                  "onChange": function( name, key ){
                     $scope.ResourceGrantWindow['config']['inputList'][1]['value'] = !key ;
                     setGrant( { 'name': 'HostFile', 'privilege': !key, 'clustername': $scope.ClusterList[chooseCluster]['ClusterName'] } ) ;
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

      //集群操作下拉菜单
      $scope.ClusterDropdown = {
         'config': [
            { 'key': $scope.autoLanguage( '资源授权' ) },
            { 'key': $scope.autoLanguage( '删除集群' ) }
         ],
         'OnClick': function( index ){
            if( index == 0 )
            {
               showResourceGrant() ;
               $scope.ResourceGrantWindow['config']['inputList'][0]['disabled'] = true ;
               $scope.ResourceGrantWindow['config']['inputList'][1]['value'] = $scope.ClusterList[chooseCluster]['GrantConf'][0]['Privilege'] ;
            }
            else
            {
               $scope.Components.Confirm.type = 3 ;
               $scope.Components.Confirm.context = sprintf( $scope.autoLanguage( '是否确定删除集群：?？' ), $scope.ClusterList[chooseCluster]['ClusterName'] ) ;
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