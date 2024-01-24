//@ sourceURL=Log.js
(function(){
   var sacApp = window.SdbSacManagerModule ;
   //控制器
   sacApp.controllerProvider.register( 'Monitor.SdbNode.Log.Ctrl', function( $scope, $compile, $timeout, $location, SdbRest, SdbFunction ){
      
      var clusterName = SdbFunction.LocalData( 'SdbClusterName' ) ;
      var moduleType = SdbFunction.LocalData( 'SdbModuleType' ) ;
      var moduleMode = SdbFunction.LocalData( 'SdbModuleMode' ) ;
      var moduleName = SdbFunction.LocalData( 'SdbModuleName' ) ;
      var hostname = SdbFunction.LocalData( 'SdbHostName' ) ;
      var svcname = SdbFunction.LocalData( 'SdbServiceName' ) ;
      if( clusterName == null || moduleType != 'sequoiadb' || moduleMode == null || moduleName == null )
      {
         $location.path( '/Transfer' ).search( { 'r': new Date().getTime() } ) ;
         return;
      }

      if( ( moduleMode == 'distribution' && ( hostname == null || svcname == null ) ) || moduleMode == 'standalone' )
      {
         $location.path( '/Monitor/SDB/Index' ).search( { 'r': new Date().getTime() } ) ;
         return;
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
         $location.path( '/Monitor/SDB-Nodes/Nodes' ).search( { 'r': new Date().getTime() } ) ;
      } ;

      function getNodeLog()
      {
         var data = { 'cmd': 'get node log', 'BusinessName': moduleName, 'HostName': hostname, 'ServiceName': svcname } ;
         SdbRest.OmOperation( data, {
            'success': function( result ){
               $scope.LogContext = result[0]['Log'] ;
            },
            'failed': function( errorInfo ){
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  getNodeLog() ;
                  return true ;
               } ) ;
            }
         } ) ;
      }

      $scope.Refresh = function(){
         getNodeLog() ;
      } ;

      getNodeLog() ;
   } ) ;
}());