//@ sourceURL=Index.js
//"use strict" ;
(function(){
   var sacApp = window.SdbSacManagerModule ;
   //控制器
   sacApp.controllerProvider.register( 'Monitor.HostList.Index.Ctrl', function( $scope, $location, SdbRest, SdbFunction ){
      
      var clusterName = SdbFunction.LocalData( 'SdbClusterName' ) ;
      var moduleType = SdbFunction.LocalData( 'SdbModuleType' ) ;
      var moduleMode = SdbFunction.LocalData( 'SdbModuleMode' ) ;
      var moduleName = SdbFunction.LocalData( 'SdbModuleName' ) ;
      if( clusterName == null || moduleType != 'sequoiadb' || moduleMode == null || moduleName == null )
      {
         $location.path( '/Transfer' ).search( { 'r': new Date().getTime() } ) ;
         return;
      }

      //初始化
      //主机列表的表格
      $scope.HostTable = {
         'title': {
            'HostName':       $scope.autoLanguage( '主机名' ),
            'IP':             'IP',
            'AgentService':   $scope.autoLanguage( '代理端口' ),
            'CPU':            'CPU',
            'MemorySize':     $scope.autoLanguage( '内存大小' ),
            'DiskSize':       $scope.autoLanguage( '磁盘容量' ),
            'OS.Description': $scope.autoLanguage( '操作系统' ),
            'Net.length':     $scope.autoLanguage( '网卡数' )
         },
         'body': [],
         'options': {
            'width': {},
            'sort': {
               'HostName':       true,
               'IP':             true,
               'AgentService':   true,
               'CPU':            true,
               'MemorySize':     true,
               'DiskSize':       true,
               'OS.Description': true,
               'Net.length':     true
            },
            'max': 50,
            'filter': {
               'HostName':       'indexof',
               'IP':             'indexof',
               'AgentService':   'indexof',
               'CPU':            'indexof',
               'MemorySize':     'number',
               'DiskSize':       'number',
               'OS.Description': 'indexof',
               'Net.length':     'number'
            }
         },
         'callback': {}
      } ;
     
      //获取主机列表
      var getHostList = function( hostnameList ){
         var data = { 'cmd': 'query host', 'filter': JSON.stringify( { '$or': hostnameList } ), 'sort': JSON.stringify( { 'HostName': 1 } ) } ;
         SdbRest.OmOperation( data, {
            'success': function( hostList ){
               var newHostList = [] ;  
               $.each( hostList, function( index, hostInfo ){
                  var DiskSize = 0 ;
                  $.each( hostInfo['Disk'], function( index2, diskInfo ){
                     DiskSize += diskInfo['Size'] ;
                  } ) ;
                  hostInfo['CPU'] = hostInfo['CPU'][0]['Model'] ;
                  hostInfo['DiskSize'] = fixedNumber(  DiskSize / 1024, 2 ) ;
                  hostInfo['MemorySize'] = fixedNumber(  hostInfo['Memory']['Size'] / 1024, 2 ) ;
                  newHostList.push( hostInfo ) ;
               } ) ;
               $scope.HostTable['body'] = newHostList ;
            },
            'failed': function( errorInfo ){
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  getHostList( hostnameList ) ;
                  return true ;
               } ) ;
            }
         },{
            'showLoading': false
         } ) ;
      }

      //查询业务信息
      var getModuleInfo = function(){
         var data = { 'cmd': 'query business', 'filter' : JSON.stringify( { 'BusinessName': moduleName } ) } ;
         SdbRest.OmOperation( data, {
            'success': function( moduleInfo ){
               var hostnameList = [] ;
               $.each( moduleInfo[0]['Location'], function( index, hostInfo ){
                  hostnameList.push( { 'HostName': hostInfo['HostName'] } ) ;
               } ) ;
               getHostList( hostnameList ) ;
            }, 
            'failed': function( errorInfo ){
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  getModuleInfo() ;
                  return true ;
               } ) ;
            }
         },{
            'showLoading': false
         } ) ;
      }

      getModuleInfo() ;
      
      //跳转事件
      $scope.GotoHost = function( HostName ){
         SdbFunction.LocalData( 'SdbHostName', HostName ) ;
         $location.path( '/Monitor/SDB-Host/Info/Index' ).search( { 'r': new Date().getTime() } ) ;
      }

      $scope.GotoDisk = function( HostName ){
         SdbFunction.LocalData( 'SdbHostName', HostName ) ;
         $location.path( '/Monitor/SDB-Host/Info/Disk' ).search( { 'r': new Date().getTime() } ) ;
      }

      $scope.GotoNet = function( HostName ){
         SdbFunction.LocalData( 'SdbHostName', HostName ) ;
         $location.path( '/Monitor/SDB-Host/Info/Network' ).search( { 'r': new Date().getTime() } ) ;
      }

      $scope.GotoCPU = function( HostName ){
         SdbFunction.LocalData( 'SdbHostName', HostName ) ;
         $location.path( '/Monitor/SDB-Host/Info/CPU' ).search( { 'r': new Date().getTime() } ) ;
      }

      $scope.GotoMemory = function( HostName ){
         SdbFunction.LocalData( 'SdbHostName', HostName ) ;
         $location.path( '/Monitor/SDB-Host/Info/Memory' ).search( { 'r': new Date().getTime() } ) ;
      }

      //跳转至资源
      $scope.GotoResources = function(){
         $location.path( '/Monitor/SDB-Resources/Session' ).search( { 'r': new Date().getTime() } ) ;
      } ;

      //跳转至主机列表
      $scope.GotoHostList = function(){
         $location.path( '/Monitor/SDB-Host/List/Index' ).search( { 'r': new Date().getTime() } ) ;
      } ;
      
      //跳转至节点列表
      $scope.GotoNodeList = function(){
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