//@ sourceURL=Index.js
(function(){
   var sacApp = window.SdbSacManagerModule ;
   //控制器
   sacApp.controllerProvider.register( 'Config.PG.Index.Ctrl', function( $scope, $location, $rootScope, SdbFunction, SdbRest, SdbSignal, SdbSwap, SdbPromise ){
      var clusterName = SdbFunction.LocalData( 'SdbClusterName' ) ;
      var moduleType = SdbFunction.LocalData( 'SdbModuleType' ) ;
      var moduleName = SdbFunction.LocalData( 'SdbModuleName' ) ;
      var moduleMode = SdbFunction.LocalData( 'SdbModuleMode' ) ;
      if( clusterName == null || moduleType != 'sequoiasql-postgresql' || moduleName == null || moduleMode == null )
      {
         $location.path( '/Transfer' ).search( { 'r': new Date().getTime() } ) ;
         return;
      }

      var template = [] ;
      var currentConfigs = {} ;

      $scope.ModuleName = moduleName ;
      $scope.Expand = false ;

      $scope.SettingTable = {
         'table': {
            'width': {
               'context': '120px'
            },
            'tools': false,
            'max': 10000
         },
         'title': {
            'name':        $scope.pAutoLanguage( '配置项' ),
            'setting':     $scope.pAutoLanguage( '值' ),
            'context':     $scope.pAutoLanguage( '生效类型' ),
            'short_desc':  $scope.pAutoLanguage( '描述' ),
            'extra_desc':  $scope.pAutoLanguage( '详细描述' )
         },
         'content': [],
         'callback': {}
      } ;

      //修改配置弹窗
      $scope.ModifyConfigWindow = {
         'config': {
            'keyWidth': '160px',
            'inputList': []
         },
         'callback': {}
      } ;

      $scope.Refresh = function(){
         querySetting( $scope.Expand ) ;
      }

      $scope.SwitchExpandConfigs = function(){
         $scope.Expand = !$scope.Expand ;

         clearArray( $scope.SettingTable['content'] ) ;
         clearObject( currentConfigs ) ;

         $.each( template, function( index, info ){
            if ( $scope.Expand || info['source'] == 'configuration file' )
            {
               $scope.SettingTable['content'].push( info ) ;

               currentConfigs[info['name']] = info['setting'] ;
            }
         } ) ;
      }

      //把弹窗的表单生成rest请求格式
      function ModifyConfigRequest()
      {
         //要修改的配置项
         this.updateConfig = {} ;

         //要删除的配置项
         this.deleteConfig = {} ;

         this.restartConfig = [] ;

         this.getTemplateInfo = function( template, name )
         {
            for( var i in template )
            {
               if ( template[i]['name'] == name )
               {
                  return template[i] ;
               }
            }
            return null ;
         }

         this.loadUpdateConfig = function( nodeConfigs, template, config )
         {
            for( var key in config )
            {
               var tmpValue = config[key] ;
               var tmpType = typeof( tmpValue ) ;

               if ( ( tmpType == 'string' && tmpValue.length == 0 ) || 
                    ( tmpType == 'number' && isNaN( tmpValue ) ) )
               {
                  //没有填值
                  continue ;
               }

               //根据模板做类型转换
               var isIntEqual = false ;
               var templateInfo = this.getTemplateInfo( template, key ) ;
               if ( templateInfo )
               {
                  switch( templateInfo['vartype'] )
                  {
                  case 'integer':
                     if ( isNaN( tmpValue ) )
                     {
                        continue ;
                     }
                     isIntEqual = integerEqual( nodeConfigs[key], config[key] ) ;
                     break ;
                  case 'real':
                     if ( isNaN( tmpValue ) )
                     {
                        continue ;
                     }
                     break ;
                  }
               }

               this.updateConfig[key] = tmpValue ;
               if ( templateInfo && templateInfo['context'] == 'postmaster' &&
                    ( isIntEqual == false && nodeConfigs[key] != config[key] ) )
               {
                  this.restartConfig.push( key ) ;
               }
            }
         }

         this.loadDeleteConfigs = function( nodeConfigs, template, config )
         {
            for( var key in config )
            {
               var value = config[key] ;
               var type = typeof( value ) ;

               if ( ( type == 'string' && value.length == 0 ) ||
                    ( type == 'number' && isNaN( value ) ) )
               {
                  //值是空的
                  if ( hasKey( nodeConfigs, key ) )
                  {
                     //看看节点原来有没有这个值
                     this.deleteConfig[key] = 1 ;

                     var templateInfo = this.getTemplateInfo( template, key ) ;
                     if ( templateInfo && templateInfo['context'] == 'postmaster' )
                     {
                        this.restartConfig.push( key ) ;
                     }
                  }
               }
            }
         }

         this.parse = function( nodeConfigs, template, config )
         {
            this.updateConfig = {} ; ;
            this.deleteConfig = {} ;

            this.loadUpdateConfig( nodeConfigs, template, config ) ;

            this.loadDeleteConfigs( nodeConfigs, template, config ) ;
         }

         this.getUpdateConfig = function()
         {
            if( getObjectSize( this.updateConfig ) > 0 )
            {
               return { 'property': this.updateConfig } ;
            }
            else
            {
               return null ;
            }
         }

         this.getDeleteConfig = function()
         {
            if( getObjectSize( this.deleteConfig ) > 0 )
            {
               return { 'property': this.deleteConfig } ;
            }
            else
            {
               return null ;
            }
         }

         this.isRestart = function()
         {
            return this.restartConfig.length > 0 ;
         }

         this.getRestartDesc = function()
         {
            var str = '' ;
            for ( var i in this.restartConfig )
            {
               if ( i >= 3 )
               {
                  break ;
               }
               else if ( i > 0 )
               {
                  str = str + ', ' ;
               }

               str = str + this.restartConfig[i] ;
            }

            if ( this.restartConfig.length > 3 )
            {
               str = str + ' ...' ;
            }
            return str ;
         }
      }

      function requestModifyConfig( cmd, config, func )
      {
         if ( config == null )
         {
            if ( isFunction( func ) )
            {
               func( false ) ;
            }
            return ;
         }

         var configInfo = {
            'ClusterName': clusterName,
            'BusinessName': moduleName,
            'Config': config
         } ;

         var data = {
            'cmd': cmd,
            'ConfigInfo': JSON.stringify( configInfo )
         } ;

         SdbRest.OmOperation( data, {
            'success': function(){
               if ( isFunction( func ) )
               {
                  func( true ) ;
               }
            }, 
            'failed': function( errorInfo ){
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  requestModifyConfig( cmd, config ) ;
                  return true ;
               } ) ;
            }
         } ) ;
      }

      $scope.OpenModifyConfigWindow = function(){

         $scope.ModifyConfigWindow['config']['inputList'] = pgSettingConvertTemplate( template, $scope.Expand ) ;
         
         $scope.ModifyConfigWindow['callback']['SetTitle']( $scope.pAutoLanguage( '修改配置项' ) ) ;
         $scope.ModifyConfigWindow['callback']['SetOkButton']( $scope.pAutoLanguage( '确定' ), function(){
            var isAllClear = $scope.ModifyConfigWindow['config'].check() ;
            if( isAllClear )
            {
               var configs = $scope.ModifyConfigWindow['config'].getValue() ;
               var request = new ModifyConfigRequest() ;

               request.parse( currentConfigs, template, configs ) ;
               var updateConfig = request.getUpdateConfig() ;
               var deleteConfig = request.getDeleteConfig() ;

               requestModifyConfig( 'update business config', updateConfig, function(){
                  requestModifyConfig( 'delete business config', deleteConfig, function(){
                     querySetting( $scope.Expand ) ;
                     if ( request.isRestart() )
                     {
                        $scope.Components.Confirm.type = 2 ;
                        $scope.Components.Confirm.context = sprintf( $scope.pAutoLanguage( '? 配置需要重启生效。' ), request.getRestartDesc() ) ;
                        $scope.Components.Confirm.isShow = true ;
                        $scope.Components.Confirm.noClose = true ;
                        $scope.Components.Confirm.okText = $scope.pAutoLanguage( '确定' ) ;
                        $scope.Components.Confirm.ok = function(){
                           $scope.Components.Confirm.isShow = false ;
                           $scope.Components.Confirm.noClose = false ;
                        }
                     }
                  } ) ;
               } ) ;
            }
            else
            {
               $scope.ModifyConfigWindow['config'].scrollToError( null ) ;
            }
            return isAllClear ;
         } ) ;
         $scope.ModifyConfigWindow['callback']['Open']() ;
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
               $rootScope.tempData( 'Deploy', 'Module', 'sequoiasql-postgresql' ) ;
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

      function querySetting( extend )
      {
         var data = { 'Sql': "select * from pg_settings", 'DbName': '', 'IsAll': 'true' } ;
         SdbRest.DataOperationV2( '/sql', data, {
            'success': function( settings ){
               template = settings ;

               clearArray( $scope.SettingTable['content'] ) ;

               $.each( settings, function( index, info ){
                  if ( extend || info['source'] == 'configuration file' )
                  {
                     $scope.SettingTable['content'].push( info ) ;

                     currentConfigs[info['name']] = info['setting'] ;
                  }
               } ) ;
            },
            'failed': function( errorInfo ){
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  querySetting( extend ) ;
                  return true ;
               } ) ;
            }
         }, {
            'showLoading': true
         } ) ;
      }

      querySetting( $scope.Expand ) ;

   } ) ;

}());