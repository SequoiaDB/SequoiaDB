//@ sourceURL=Index.js
//"use strict" ;
(function(){
   var sacApp = window.SdbSacManagerModule ;
   //主控制器
   sacApp.controllerProvider.register( 'Deploy.MariaDB.Sync.Ctrl', function( $scope, $location, $rootScope, Loading, SdbRest, SdbSignal, SdbPromise ){

      $scope.ContainerBox = [ { offsetY: -106 }, { offsetY: -40 } ] ;
      $scope.ClusterName  = $rootScope.tempData( 'Deploy', 'ClusterName' ) ;
      $scope.ModuleName   = $rootScope.tempData( 'Deploy', 'ModuleName' ) ;
      $scope.SyncConf     = $rootScope.tempData( 'Deploy', 'SyncConf' ) ;
      var moduleType      = $rootScope.tempData( 'Deploy', 'ModuleType' ) ;

      if( $scope.ModuleName == null || $scope.ClusterName == null || moduleType != 'sequoiasql-mariadb' )
      {
         $location.path( '/Transfer' ).search( { 'r': new Date().getTime() } ) ;
         return ;
      }

      //如果从安装主机进来的，有步骤条
      if( $scope.SyncConf != null )
      {
         $scope.stepList = _Deploy.BuildSyncStep( $scope, $location, $scope['Url']['Action'], 'sequoiasql-mysql' ) ;
      }

      //同步状态
      $scope.SyncStatus = 0 ;

      var afterSyncDefer  = SdbPromise.init( 2 ) ;

      //同步服务
      function syncBusinessConf()
      {
         var data = { 'cmd': 'sync business configure', 'ClusterName': $scope.ClusterName, 'BusinessName': $scope.ModuleName } ;
         SdbRest.OmOperation( data, {
            'success': function(){
               getBusinessConf() ;
            },
            'failed': function( errorInfo ){
               $scope.SyncStatus = 2 ;
               Loading.cancel() ;
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  syncBusinessConf() ;
                  return true ;
               } ) ;
            }
         } ) ;
      }

      var isFirst = true ;
      //获取服务配置
      function getBusinessConf()
      {
         var data = { 'cmd': 'query node configure', 'filter': JSON.stringify( { 'ClusterName': $scope.ClusterName, 'BusinessName': $scope.ModuleName } ) } ;
         SdbRest.OmOperation( data, {
            'success': function( result ){
               var tempBusinessConfig = [] ;
               $.each( result, function( index, hostInfo ){
                  $.each( hostInfo['Config'], function( index2, nodeInfo ){
                     nodeInfo['hostName'] = hostInfo['HostName'] ;
                     tempBusinessConfig.push( nodeInfo ) ;
                  } ) ;
               } ) ;
               if( isFirst == true )
               {
                  afterSyncDefer.resolve( 'BeforeNodeConf', tempBusinessConfig ) ;
                  syncBusinessConf() ;
               }
               else
               {
                  afterSyncDefer.resolve( 'AfterNodeConf', tempBusinessConfig ) ;
               }
               isFirst = false ;
            },
            'failed': function( errorInfo ){
               $scope.SyncStatus = 2 ;
               Loading.cancel() ;
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  getBusinessConf() ;
                  return true ;
               } ) ;
            }
         } ) ;
      }

      //同步后节点配置
      afterSyncDefer.then( function( result ){
         generateResult( result ) ;
         $scope.SyncStatus = 1 ;
         Loading.cancel() ;
      } );

      $scope.ConfigTable = {
         'title': {
            'key':      $scope.autoLanguage( '配置项' ),
            'value.0':  $scope.autoLanguage( '同步前' ),
            'value.1':  $scope.autoLanguage( '同步后' )
         },
         'body': [],
         'options': {
            'width': {
               'key':     '20%',
               'value.0': '40%',
               'value.1': '40%',
            },
            'sort': {
               'key': true
            },
            'autoSort': { 'key': 'key', 'asc': true },
            'max': 10000,
            'tools': false
         },
         'callback': {}
      };

      //两次配置获取完之后执行
      function generateResult( result )
      {
         var configList = [];
         var beforeConfig = result['BeforeNodeConf'][0] ;
         var afterConfig = result['AfterNodeConf'][0] ;

         //原节点的所有配置项
         var keyMap = {};
         $.each( beforeConfig, function ( key, value ) {
            keyMap[key] = configList.length;
            configList.push( { 'type': 1, 'key': key, 'value': [ value, null ] } );
         } );

         $.each( keyMap, function ( key, index ) {
            if ( isUndefined( afterConfig[key] ) )
            {
               //删除的配置项
               configList[index]['type'] = 1;
            }
            else
            {
               if ( beforeConfig[key] == afterConfig[key] )
               {
                  //无变化的配置项
                  configList[index]['type'] = 0;
               }
               else
               {
                  //有变化的配置项
                  configList[index]['type'] = 3;
               }
               configList[index]['value'][1] = afterConfig[key];
            }
         } );

         $.each( afterConfig, function ( key, value ) {
            if ( isNaN( keyMap[key] ) == true )
            {
               //新配置有的项
               configList.push( { 'type': 2, 'key': key, 'value': [null, value] } );
            }
         } );

         $scope.ConfigTable['body'] = configList;
      }

      //返回
      $scope.GotoDeploy = function(){
         $location.path( '/Deploy/Index' ).search( { 'r': new Date().getTime() } ) ;
      }

      //创建loading
      Loading.cancel();
      Loading.create();

      getBusinessConf();

   } );
}());