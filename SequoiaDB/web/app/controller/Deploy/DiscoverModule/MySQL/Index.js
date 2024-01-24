//@ sourceURL=Index.js
(function(){
   var sacApp = window.SdbSacManagerModule ;
   //主控制器
   sacApp.controllerProvider.register( 'Deploy.MySQL.Discover.Ctrl', function( $scope, $location, $rootScope, SdbRest ){
      $scope.ContainerBox = [ { offsetY: -106 }, { offsetY: -40 } ] ;
      $scope.ClusterName  = $rootScope.tempData( 'Deploy', 'ClusterName' ) ;
      $scope.DiscoverConf = $rootScope.tempData( 'Deploy', 'DiscoverConf' ) ;
      var moduleHostName  = $rootScope.tempData( 'Deploy', 'ModuleHostName' ) ;
      var modulePort      = $rootScope.tempData( 'Deploy', 'ModulePort' ) ;

      if( moduleHostName == null || modulePort == null || $scope.ClusterName == null )
      {
         $location.path( '/Transfer' ).search( { 'r': new Date().getTime() } ) ;
         return ;
      }

      $scope.ConfigTable = {
         'title': {
            'key':   $scope.autoLanguage( '配置项' ),
            'value': $scope.autoLanguage( '值' )
         },
         'body': [],
         'options': {
            'width': {
               'key':   '40%',
               'value': '60%',
            },
            'sort': {
               'key': true
            },
            'autoSort': { 'key': 'key', 'asc': true },
            'max': 10000,
            'tools': false
         },
         'callback': {}
      } ;

      //如果从安装主机进来的，有步骤条
      if( $scope.DiscoverConf != null )
      {
         $scope.stepList = _Deploy.BuildDiscoverStep( $scope, $location, $scope['Url']['Action'], 'sequoiasql-mysql' ) ;
      }

      //获取服务配置
      function getBusinessConf()
      {
         var data = { 'cmd': 'query node configure', 'filter': JSON.stringify( { 'ClusterName': $scope.ClusterName, 'HostName': moduleHostName, 'Config.port': modulePort } ) } ;
         SdbRest.OmOperation( data, {
            'success': function( result ){
               if ( result.length > 0 && result[0]['Config'].length > 0 )
               {
                  $scope.ModuleName = result[0]['BusinessName'] ;
                  $.each( result[0]['Config'][0], function( key, value ){
                     $scope.ConfigTable['body'].push( { 'key': key, 'value': value } ) ;
                  } ) ;
               }
            },
            'failed': function( errorInfo ){
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  getBusinessConf() ;
                  return true ;
               } ) ;
            }
         } ) ;
      }

      //返回
      $scope.GotoDeploy = function(){
         $location.path( '/Deploy/Index' ).search( { 'r': new Date().getTime() } ) ;
      }

      getBusinessConf();

   } );
}());