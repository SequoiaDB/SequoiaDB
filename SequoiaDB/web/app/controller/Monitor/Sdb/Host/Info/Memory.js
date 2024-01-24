//@ sourceURL=Memory.js
(function(){
   var sacApp = window.SdbSacManagerModule ;
   //控制器
   sacApp.controllerProvider.register( 'Performance.Memory.Index.Ctrl', function( $scope, $compile, $location, SdbRest, SdbFunction ){
      
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
      $scope.hostName = hostName ;
      //主机状态
      $scope.HostStatus = null ;
      //内存信息
      $scope.memoryInfo = { 'size': '-', 'use': '-', 'free': '-' } ;
      //图表配置
      $scope.charts = {}; 
      $scope.charts['Memory'] = {} ;
      $scope.charts['Memory']['options'] = window.SdbSacManagerConf.MemoryEchart ;
      $scope.charts['Memory']['options']['title']['text'] = $scope.autoLanguage( '内存利用率' ) ;
      $scope.charts['Memory']['value'] = [ [ 0, 0, true, false ], [ 0, 0, true, false ] ] ;

      $scope.charts['MemoryBar'] = {} ;
      $scope.charts['MemoryBar']['options'] = window.SdbSacManagerConf.MemoryLessEchart ;

      //获取主机状态信息
      var getMemoryInfo = function(){
         var data = {
            'cmd':'query host status',
            'HostInfo': JSON.stringify( { "HostInfo": [ { 'HostName': hostName } ] } )
         } ;
         SdbRest.OmOperation( data, {
            'success': function( hostList ){
               if( hostList.length > 0 )
               {
                  $.each( hostList[0]['HostInfo'], function( Index, hostInfo ){
                     var freePencent = 0 ;
                     var usePencent  = 0 ;
                     if( isNaN( hostInfo['errno'] ) == false && hostInfo['errno'] != 0 )
                     {
                        $scope.charts['Memory']['value'] = [ [ 0, 0, true, false ] ] ;
                        $scope.HostStatus = false ;
                        return true ;
                     }
                     $scope.HostStatus = true ;

                     var memoryInfo = hostInfo['Memory'] ;

                     usePencent = fixedNumber( memoryInfo['Used'] / memoryInfo['Size'] * 100, 2 ) ;
                     freePencent = fixedNumber( 100 - usePencent, 2 ) ;

                     $scope.memoryInfo['size'] = memoryInfo['Size'] ;
                     $scope.memoryInfo['use']  = memoryInfo['Used'] ;
                     $scope.memoryInfo['free'] = memoryInfo['Free'] ;
                     $scope.charts['Memory']['value'] = [ [ 0, usePencent, true, false ] ] ;
                     $scope.charts['MemoryBar']['value'] = [
                        [ 0, usePencent, true, false ],
                        [ 1, freePencent, true, false ]
                     ] ;
                  } ) ;
               }
            }
         }, {
            'showLoading': false,
            'delay': 5000,
            'loop': true
         } ) ;
      }

      getMemoryInfo() ;

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