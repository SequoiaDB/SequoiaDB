//@ sourceURL=web.js
(function(){
   var sacApp = window.SdbSacManagerModule ;
   //控制器
   sacApp.controllerProvider.register( 'Data.Other.Index.Ctrl', function( $scope, $location, $compile, SdbFunction, SdbRest ){
      var clusterName = SdbFunction.LocalData( 'SdbClusterName' ) ;
      var moduleType = SdbFunction.LocalData( 'SdbModuleType' ) ;
      var moduleName = SdbFunction.LocalData( 'SdbModuleName' ) ;
      if( clusterName == null || ( moduleType != 'hdfs' && moduleType != 'spark' && moduleType != 'yarn' )  || moduleName == null )
      {
         window.location.href = '/deployment/index.html' ;
         return;
      }

      printfDebug( 'Cluster: ' + clusterName + ', Type: ' + moduleType + ', Module: ' + moduleName ) ;

      $scope.url = '' ;

      var data = { 'cmd': 'query business', 'filter': JSON.stringify( { 'BusinessName': moduleName, 'ClusterName': clusterName } ) } ;
      SdbRest.OmOperation( data, {
         'success': function( moduleInfo ){
            if( moduleInfo.length > 0 )
            {
               moduleInfo = moduleInfo[0]['BusinessInfo'] ;
               $scope.url = 'http://' + moduleInfo['HostName'] + ':' + moduleInfo['WebServicePort'] ;
            }
         },
         'failed': function( errorInfo, retryFun ){
            _IndexPublic.createRetryModel( $scope, errorInfo, function(){
               retryFun() ;
               return true ;
            } ) ;
         }
      } ) ;
   } ) ;
}());