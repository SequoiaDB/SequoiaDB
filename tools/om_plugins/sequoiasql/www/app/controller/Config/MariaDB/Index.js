//@ sourceURL=Config.MariaDB.Index.Ctrl.js
(function(){
   var sacApp = window.SdbSacManagerModule ;
   //控制器
   sacApp.controllerProvider.register( 'Config.MariaDB.Index.Ctrl', function( $scope, $location, $rootScope, SdbFunction, SdbRest, SdbSignal, SdbSwap, SdbPromise ){

      var clusterName = SdbFunction.LocalData( 'SdbClusterName' ) ;
      var moduleType = SdbFunction.LocalData( 'SdbModuleType' ) ;
      var moduleName = SdbFunction.LocalData( 'SdbModuleName' ) ;
      var moduleMode = SdbFunction.LocalData( 'SdbModuleMode' ) ;

      if( clusterName == null || moduleType != 'sequoiasql-mariadb' || moduleName == null || moduleMode == null )
      {
         $location.path( '/Transfer' ).search( { 'r': new Date().getTime() } ) ;
         return;
      }

      var template = {} ;

      $scope.ModuleName = moduleName ;

      $scope.SettingTable = {
         'table': {
            'tools': false,
            'max': 10000,
            'filter': {
               'VARIABLE_NAME': 'indexof',
               'VARIABLE_VALUE': 'indexof'
            }
         },
         'title': {
            'VARIABLE_NAME':  $scope.pAutoLanguage( '配置项' ),
            'VARIABLE_VALUE': $scope.pAutoLanguage( '值' )
         },
         'content': [],
         'callback': {}
      } ;

      //修改多个配置弹窗
      $scope.ConfigsWindow = {
         'config': {
            'keyWidth': '200px',
            'inputList': []
         },
         'callback': {}
      } ;

      //修改单个配置弹窗
      $scope.ConfigWindow = {
         'config': {
            'keyWidth': '200px',
            'inputList': [
               {
                  'name':     'value',
                  'value':    '',
                  'webName':  '',
                  'type':     'string'
               }
            ]
         },
         'callback': {}
      } ;

      $scope.Refresh = function(){
         queryGlobalVar() ;
      }

      function deleteConfig( configs, success )
      {
         if ( isEmpty( configs ) )
         {
            success() ;
            return ;
         }

         var configInfo = {
            'ClusterName': clusterName,
            'BusinessName': moduleName,
            'Config': {
               'property': configs
            }
         } ;

         var data = {
            'cmd': 'delete business config',
            'ConfigInfo': JSON.stringify( configInfo )
         } ;

         SdbRest.OmOperation( data, {
            'success': success,
            'failed': function( errorInfo ){
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  deleteConfig( configs, success ) ;
                  return true ;
               } ) ;
            }
         } ) ;
      }

      function updateConfig( configs, success )
      {
         if ( isEmpty( configs ) )
         {
            success() ;
            return ;
         }

         var configInfo = {
            'ClusterName': clusterName,
            'BusinessName': moduleName,
            'Config': {
               'property': configs
            }
         } ;

         var data = {
            'cmd': 'update business config',
            'ConfigInfo': JSON.stringify( configInfo )
         } ;

         SdbRest.OmOperation( data, {
            'success': success,
            'failed': function( errorInfo ){
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  updateConfig( configs, success ) ;
                  return true ;
               } ) ;
            }
         } ) ;
      }

      function saveServiceConfig( configs )
      {
         if ( isEmpty( configs ) )
         {
            queryGlobalVar() ;
            return ;
         }

         var updateConfigs = {} ;
         var deleteConfigs = [] ;

         for( var key in configs )
         {
            if( isString( configs[key] ) && configs[key].length == 0 )
            {
               deleteConfigs.push( key ) ;
            }
            else
            {
               updateConfigs[key] = configs[key] ;
            }
         }

         updateConfig( updateConfigs, function(){
            deleteConfig( deleteConfigs, function(){
               queryGlobalVar() ;
            } ) ;
         } ) ;
      }

      function updateServiceConfig( updator )
      {
         var sql = 'set global ' ;
         var isFirst = true ;

         for( var key in updator )
         {
            if ( !isFirst )
            {
               sql += ', ' ;
            }
            if ( isNaN( updator[key] ) || ( isString( updator[key] ) && updator[key].length == 0 ) )
            {
               sql += key + "='" + updator[key] + "'" ;
            }
            else
            {
               sql += key + "=" + updator[key] ;
            }
            isFirst = false ;
         }

         var data = { 'Sql': sql, 'DbName': '', 'Type': 'mysql' } ;
         SdbRest.DataOperationV2( '/sql', data, {
            'success': function( result ){
               saveServiceConfig( updator ) ;
            },
            'failed': function( errorInfo ){
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  updateServiceConfig( updator ) ;
                  return true ;
               } ) ;
            }
         }, {
            'showLoading': true
         } ) ;
      }

      //批量
      $scope.OpenConfigsWindow = function(){
     
         $scope.ConfigsWindow['callback']['SetTitle']( $scope.pAutoLanguage( '修改配置项' ) ) ;
         $scope.ConfigsWindow['callback']['SetOkButton']( $scope.pAutoLanguage( '确定' ), function(){
            var isAllClear = $scope.ConfigsWindow['config'].check() ;
            if( isAllClear )
            {
               var configs = $scope.ConfigsWindow['config'].getValue() ;
               var newConfigs = {} ;

               for( var key in configs )
               {
                  if( template[key] != configs[key] )
                  {
                     newConfigs[key] = configs[key] ;
                  }
               }

               if( isEmpty( newConfigs ) )
               {
                  queryGlobalVar() ;
               }
               else
               {
                  updateServiceConfig( newConfigs ) ;
               }
            }
            else
            {
               $scope.ConfigsWindow['config'].scrollToError( null ) ;
            }
            return isAllClear ;
         } ) ;
         $scope.ConfigsWindow['callback']['Open']() ;
      }

      //单个
      $scope.OpenConfigWindow = function( key, value ){

         $scope.ConfigWindow['config']['inputList'][0]['webName'] = key ;
         $scope.ConfigWindow['config']['inputList'][0]['name'] = key ;
         $scope.ConfigWindow['config']['inputList'][0]['value'] = value ;
      
         $scope.ConfigWindow['callback']['SetTitle']( $scope.pAutoLanguage( '修改配置项' ) ) ;
         $scope.ConfigWindow['callback']['SetOkButton']( $scope.pAutoLanguage( '确定' ), function(){
            var isAllClear = $scope.ConfigWindow['config'].check() ;
            if( isAllClear )
            {
               var configs = $scope.ConfigWindow['config'].getValue() ;

               updateServiceConfig( configs ) ;
            }
            return isAllClear ;
         } ) ;
         $scope.ConfigWindow['callback']['Open']() ;
      }

      function restartModule()
      {
         var data = {
            'cmd': 'restart business',
            'ClusterName': clusterName,
            'BusinessName': moduleName,
            'Options': {}
         } ;

         SdbRest.OmOperation( data, {
            'success': function( taskInfo ){
               $rootScope.tempData( 'Deploy', 'Model', 'Update Config' ) ;
               $rootScope.tempData( 'Deploy', 'Module', 'sequoiasql-mariadb' ) ;
               $rootScope.tempData( 'Deploy', 'ModuleTaskID', taskInfo[0]['TaskID'] ) ;
               $location.path( '/Deploy/Task/Restart' ).search( { 'r': new Date().getTime() } ) ;
            }, 
            'failed': function( errorInfo ){
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  restartModule() ;
                  return true ;
               } ) ;
            }
         } ) ;
      }

      $scope.RestartModule = function(){
         $scope.Components.Confirm.type = 2 ;
         $scope.Components.Confirm.context = $scope.pAutoLanguage( '该操作将重启实例，是否确定继续？' ) ;
         $scope.Components.Confirm.isShow = true ;
         $scope.Components.Confirm.okText = $scope.pAutoLanguage( '确定' ) ;
         $scope.Components.Confirm.ok = function(){
            restartModule() ;
            $scope.Components.Confirm.isShow = false ;
         }
      }

      function queryGlobalVar()
      {
         var data = { 'Sql': "select * from global_variables", 'DbName': 'information_schema', 'Type': 'mysql', 'IsAll': 'true' } ;
         SdbRest.DataOperationV2( '/sql', data, {
            'success': function( result ){

               clearObject( template ) ;
               clearArray( $scope.ConfigsWindow['config']['inputList'] ) ;

               $.each( result, function( index, info ){
                  info['VARIABLE_NAME'] = info['VARIABLE_NAME'].toLowerCase() ;
                  template[info['VARIABLE_NAME']] = info['VARIABLE_VALUE'] ;

                  $scope.ConfigsWindow['config']['inputList'].push( {
                     'name':     info['VARIABLE_NAME'],
                     'value':    info['VARIABLE_VALUE'],
                     'webName':  info['VARIABLE_NAME'],
                     'type':     'string'
                  } ) ;
               } ) ;

               $scope.SettingTable['content'] = result ;

            },
            'failed': function( errorInfo ){
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  queryGlobalVar() ;
                  return true ;
               } ) ;
            }
         }, {
            'showLoading': true
         } ) ;
      }

      queryGlobalVar() ;
   } ) ;

}());