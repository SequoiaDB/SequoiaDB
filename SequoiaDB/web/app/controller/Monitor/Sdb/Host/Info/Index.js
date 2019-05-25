//@ sourceURL=Index.js
//"use strict" ;
(function(){
   var sacApp = window.SdbSacManagerModule ;
   //控制器
   sacApp.controllerProvider.register( 'Host.Performance.Index.Ctrl', function( $scope, SdbRest, $location, $compile, SdbFunction ){
      
      _IndexPublic.checkMonitorEdition( $location ) ; //检测是不是企业版

      var clusterName = SdbFunction.LocalData( 'SdbClusterName' ) ;
      var moduleType = SdbFunction.LocalData( 'SdbModuleType' ) ;
      var moduleName = SdbFunction.LocalData( 'SdbModuleName' ) ;
      var moduleMode = SdbFunction.LocalData( 'SdbModuleMode' ) ;
      var hostName = SdbFunction.LocalData( 'SdbHostName' ) ;
      if( clusterName == null || moduleType != 'sequoiadb' || moduleMode == null || moduleName == null )
      {
         $location.path( '/Transfer' ).search( { 'r': new Date().getTime() } ) ;
         return ;
      }

      if( hostName == null )
      {
         $location.path( '/Monitor/SDB-Host/List/Index' ).search( { 'r': new Date().getTime() } ) ;
         return ;
      }

      //初始化
      $scope.NodeList = { 'coord': [], 'catalog': [], 'data': [], 'standalone': [] } ;
      //主机状态
      $scope.HostStatus = null ;
      //磁盘总空间
      var diskSize = 0 ;
      //磁盘使用
      var diskUsed = 0 ;
      //磁盘使用率
      var diskPercent = 0 ;
      //图表信息
      var chartDetail = {} ;
      //最后一次的cpu数据
      var lastCPU = null ;
      //计算网卡使用率
      var networkIn = [] ;
      var networkOut = [] ;
      var networkInValue = [] ;
      var networkOutValue = [] ;
      var netInChart = 0 ;
      var netOutChart = 0 ;

      //初始化主机信息
      $scope.hostInfo = {
         'HostName': hostName,
         'IP': '-',
         'OS': '-',
         'AgentService': '-',
         'CPU': '-',
         'Processes': '-',
         'Memory': '-',
         'Disk': '-'
      } ;

      //获取节点列表
      var getNodeList = function(){
         var data = { 'cmd': 'list nodes', 'BusinessName': moduleName } ;
         SdbRest.OmOperation( data, {
            'success': function( nodeList ){
               $.each( nodeList, function( index, nodeInfo ){
                  if( nodeInfo['HostName'] == hostName )
                  {
                     $scope.NodeList[ nodeInfo['Role'] ].push( { 'hostName': nodeInfo['HostName'], 'serviceName': nodeInfo['ServiceName'] } ) ;
                  }
               } ) ;
            },
            'failed': function( errorInfo ){
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  getNodeList() ;
                  return true ;
               } ) ;
            }
         },{
            'showLoading': false
         } ) ;
      }
      
      //获取主机性能
      var getChartInfo = function(){
         var data = {
            'cmd': 'query host status',
            'HostInfo': JSON.stringify( { "HostInfo": [ { 'HostName': hostName } ] } )
         } ;
         SdbRest.OmOperation( data, {
            'success': function( hostList ){
               if( hostList.length > 0 )
               {
                  diskUsed = 0 ;
                  diskSize = 0 ;
                  $.each( hostList[0]['HostInfo'], function( Index, hostInfo ){
                     if( isNaN( hostInfo['errno'] ) == false && hostInfo['errno'] != 0 )
                     {
                        $scope.HostStatus = false ;
                        return true ;
                     }
                     $scope.HostStatus = true ;
                     //当前磁盘占用率
                     if( isArray( hostInfo['Disk'] ) )
                     {
                        $.each( hostInfo['Disk'], function( index, diskInfo ){
                           diskUsed += diskInfo['Size'] - diskInfo['Free'] ;
                           diskSize += diskInfo['Size'] ;
                        } ) ;
                     }
                     diskPercent = ( ( diskUsed/diskSize )*100 ).toFixed(2) ;

                     //网卡
                     netInChart = 0 ;
                     netOutChart = 0 ;
                     $.each( hostInfo['Net']['Net'], function( index, netInfo){
                        if( typeof( networkIn[index] ) == 'undefined' &&  typeof( networkOut[index] ) == 'undefined' )
                        {
                           networkIn[index] = netInfo['RXBytes']['Megabit'] * 1024  + netInfo['RXBytes']['Unit'] / 1024 ;
                           networkOut[index] = netInfo['TXBytes']['Megabit'] * 1024  + netInfo['TXBytes']['Unit'] / 1024 ;
                        }
                        else
                        {
                           networkInValue[index] = netInfo['RXBytes']['Megabit'] * 1024 + netInfo['RXBytes']['Unit'] / 1024 - networkIn[index]  ;
                           networkOutValue[index] = netInfo['TXBytes']['Megabit'] * 1024 + netInfo['TXBytes']['Unit'] / 1024 - networkOut[index]  ;

                           networkIn[index] = netInfo['RXBytes']['Megabit'] * 1024  + netInfo['RXBytes']['Unit'] / 1024 ;
                           networkOut[index] = netInfo['TXBytes']['Megabit'] * 1024  + netInfo['TXBytes']['Unit'] / 1024 ;
                        }

                     } ) ;
                     $.each( networkInValue, function( inIndex, inValue ){
                        netInChart += inValue ;
                     } ) ;
                     $.each( networkOutValue, function( outIndex, outValue ){
                        netOutChart += outValue ;
                     } ) ;

                     netInChart = fixedNumber( netInChart, 2 ) ;
                     netOutChart = fixedNumber( netOutChart, 2 ) ;

                     $scope.charts['network']['value'] = [ [ 0, netInChart, true, false ],[ 1, netOutChart, true, false ] ] ;
               

                     //计算CPU
                     if( lastCPU === null )
                     {
                        lastCPU = getCpuUsePercent( [ hostInfo ] ) ;
                        $scope.charts['Cpu']['value'] = [ [ 0, 0, true, false ] ] ;
                     }
                     else
                     {
                        $scope.charts['Cpu']['value'] = [ [ 0, getCpuUsePercent( [ hostInfo ], lastCPU ), true, false ] ] ;
                     }

                     //内存占用率
                     memoryPencent = fixedNumber( hostInfo['Memory']['Used'] / hostInfo['Memory']['Size'] * 100, 2 ) ;
                     $scope.charts['memory']['value'] = [ [ 0, memoryPencent, true, false ] ] ;

                     //磁盘利用率
                     $scope.charts['Storage']['value'] = [ [ 0, diskPercent, true, false ] ] ;
                  } ) ;

               }
            }
         }, {
            'showLoading': false,
            'delay': 5000,
            'loop': true
         } ) ;
         
      }

      //获取主机信息
      var getHostInfo = function(){
         var data = {
            'cmd': 'query host',
            'filter': JSON.stringify( { 'HostName': hostName } )
         } ;
         SdbRest.OmOperation( data, {
            'success': function( hostInfo ){
               if( isArray( hostInfo[0]['Disk'] ) )
               {
                  $scope.hostInfo['AgentService'] = hostInfo[0]['AgentService'] ;
                  $.each( hostInfo[0]['Disk'], function( index3, diskInfo ){
                     diskSize += diskInfo['Size'] ;
                  } ) ;
                  hostInfo[0]['DiskSize'] = (  diskSize/1024 ).toFixed(2) + 'GB' ;
                  hostInfo[0]['MemorySize'] = (  hostInfo[0]['Memory']['Size']/1024 ).toFixed(2) + 'GB' ;
                  $scope.hostInfo['IP'] = hostInfo[0]['IP'] ;
                  $scope.hostInfo['OS'] = hostInfo[0]['OS']['Description'] ;
                  $scope.hostInfo['CPU'] = hostInfo[0]['CPU'][0]['Model'] ;
                  $scope.hostInfo['Memory'] = hostInfo[0]['MemorySize'] ;
                  $scope.hostInfo['Disk'] = hostInfo[0]['DiskSize'] ;
                  getChartInfo() ;
               }
            },
            'failed': function( errorInfo ){
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  getHostInfo() ;
                  return true ;
               } ) ;
            } 
         },{
            'showLoading': false
         } ) ;
      }
      
      getHostInfo();
      getNodeList() ;

      $scope.charts = {};
      $scope.charts['Storage'] = {} ;
      $scope.charts['Storage']['options'] = window.SdbSacManagerConf.DiskStorageEchart ;
      $scope.charts['Storage']['options']['title']['text'] = $scope.autoLanguage( '磁盘利用率' ) ;

      $scope.charts['network'] = {} ;
      $scope.charts['network']['options'] = window.SdbSacManagerConf.NetwordIOEchart ;
      $scope.charts['network']['options']['title']['text'] = $scope.autoLanguage( '网络流量' ) ;

      $scope.charts['memory'] = {} ;
      $scope.charts['memory']['options'] = window.SdbSacManagerConf.MemoryEchart ;
      $scope.charts['memory']['options']['title']['text'] = $scope.autoLanguage( '内存利用率' ) ;
      
      $scope.charts['Cpu'] = {} ;
      $scope.charts['Cpu']['options'] = window.SdbSacManagerConf.CpuEchart ;
      $scope.charts['Cpu']['options']['title']['text'] = $scope.autoLanguage( 'CPU利用率' ) ;
      


      //跳转至节点信息页面
      $scope.GotoNode = function( serviceName ){
         SdbFunction.LocalData( 'SdbServiceName', serviceName ) ;
         $location.path( '/Monitor/SDB-Nodes/Node/Index' ).search( { 'r': new Date().getTime() } ) ;
      } ;

      //跳转至资源
      $scope.GotoResources = function(){
         $location.path( '/Monitor/SDB-Resources/Session' ).search( { 'r': new Date().getTime() } ) ;
      } ;

      //跳转至主机列表
      $scope.GotoHosts = function(){
         $location.path( '/Monitor/SDB-Host/List/Index' ).search( { 'r': new Date().getTime() } ) ;
      } ;
      
      
      //跳转至节点列表
      $scope.GotoNodes = function(){
         if( moduleMode == 'distribution' )
         {
            $location.path( '/Monitor/SDB-Nodes/Nodes' ).search( { 'r': new Date().getTime() } ) ;
         }
         else
         {
            $location.path( '/Monitor/SDB-Nodes/Node/Index' ).search( { 'r': new Date().getTime() } ) ;
         }
      } ;

   } ) ;
}());