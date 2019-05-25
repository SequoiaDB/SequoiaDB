//@ sourceURL=Mod.js
//"use strict" ;
(function(){
   var sacApp = window.SdbSacManagerModule ;
   //控制器
   sacApp.controllerProvider.register( 'Deploy.Ssql.Mod.Ctrl', function( $scope, $compile, $location, $rootScope, $interval, SdbRest, SdbFunction, Loading ){
      
      var configure      = $rootScope.tempData( 'Deploy', 'ModuleConfig' ) ;
      $scope.ModuleName  = $rootScope.tempData( 'Deploy', 'ModuleName' ) ;
      var deployType     = $rootScope.tempData( 'Deploy', 'Model' ) ;
      var clusterName    = $rootScope.tempData( 'Deploy', 'ClusterName' ) ;
      if( deployType == null || clusterName == null || $scope.ModuleName == null || configure == null )
      {
         $location.path( '/Deploy/Index' ).search( { 'r': new Date().getTime() } ) ;
         return ;
      }
      $scope.stepList = _Deploy.BuildSdbOltpStep( $scope, $location, $scope['Url']['Action'] ) ;
      if( $scope.stepList['info'].length == 0 )
      {
         $location.path( '/Deploy/Index' ).search( { 'r': new Date().getTime() } ) ;
         return ;
      }

      $scope.ShowType  = 'normal' ;
      
      var hostSelectList = [] ;
      var installConfig = [] ;
      var buildConf = [] ;
      //切换分类
      $scope.SwitchParam = function( type ){
         $scope.ShowType = type ;
      }

      //生成form表单
      var buildConfForm= function( config ){
         installConfig = config[0] ;
         buildConf = config[0]['Config'] ;
         $scope.Template = config[0]['Property'] ;
         $scope.AllForm = {} ;
         $scope.AllForm['normal'] = {
            'keyWidth': '200px',
            'inputList': _Deploy.ConvertTemplate( $scope.Template, 0 )
         } ;
         $scope.AllForm['normal']['inputList'].splice( 0, 0, {
            "name": "HostName",
            "webName": 'hostname',
            "type": "select",
            "value": hostSelectList[0]['key'],
            "valid": hostSelectList
         } ) ;
         $.each( $scope.AllForm['normal']['inputList'], function( index ){
            var name = $scope.AllForm['normal']['inputList'][index]['name'] ;
            $scope.AllForm['normal']['inputList'][index]['value'] = buildConf[0][name] ;
         } ) ;

         var otherForm =  _Deploy.ConvertTemplate( $scope.Template, 1, true, true ) ;
         $.each( otherForm, function( index, formInfo ){
            formInfo['confType'] = formInfo['confType'].toLowerCase() ;
            if( typeof( $scope.AllForm[formInfo['confType']] ) == 'undefined' )
            {
               $scope.AllForm[formInfo['confType']] = {
                  'keyWidth': '200px',
                  'inputList': [ formInfo ]
               } ;
            }
            else
            {
               $scope.AllForm[formInfo['confType']]['inputList'].push( formInfo ) ;
            }
         } ) ;
      }

      //获取配置
      var getModuleConfig = function(){
         var data = { 'cmd': 'get business config', 'TemplateInfo': JSON.stringify( configure ) } ;
         SdbRest.OmOperation( data, {
            'success': function( config ){
               buildConfForm( config ) ;
            },
            'failed': function( errorInfo ){
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  getModuleConfig() ;
                  return true ;
               } ) ;
            }
         } ) ;
      }

      //获取主机列表
      var getHostList = function(){
         var data = { 'cmd': 'query host' } ;
         SdbRest.OmOperation( data, {
            'success': function( hostList ){
               $.each( hostList, function( index, hostInfo ){
                  if( hostInfo['ClusterName'] == clusterName )
                  {
                     $.each( hostInfo['Packages'], function( packageIndex, packageInfo ){
                        if( packageInfo['Name'] == 'sequoiasql-oltp' )
                        {
                           hostSelectList.push( { 'key': hostInfo['HostName'], 'value': hostInfo['HostName'] } ) ;
                        }
                     } ) ;
                  }
               } ) ;
               configure['HostInfo'] = [ { 'HostName': hostSelectList[0]['value'] } ] ;
               getModuleConfig() ;
            },
            'failed': function( errorInfo ){
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  getHostList() ;
                  return true ;
               } ) ;
            }
         } ) ;
      }
      getHostList() ;
      
      $scope.GotoDeploy = function(){
         $location.path( '/Deploy/Index' ).search( { 'r': new Date().getTime() } ) ;
      }

      var installOltp = function( installConfig ){
         var data = { 'cmd': 'add business', 'ConfigInfo': JSON.stringify( installConfig ) } ;
         SdbRest.OmOperation( data, {
            'success': function( taskInfo ){
               $rootScope.tempData( 'Deploy', 'ModuleTaskID', taskInfo[0]['TaskID'] ) ;
               $location.path( '/Deploy/InstallModule' ).search( { 'r': new Date().getTime() } ) ;
            },
            'failed': function( errorInfo ){
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  installOltp( installConfig ) ;
                  return true ;
               } ) ;
            }
         } ) ;
      }

      var convertConfig = function(){
         var config = {} ;
         config['ClusterName']  = installConfig['ClusterName'] ;
         config['BusinessType'] = installConfig['BusinessType'] ;
         config['BusinessName'] = installConfig['BusinessName'] ;
         config['DeployMod']    = installConfig['DeployMod'] ;

         var allFormVal = {} ;
         var checkErrNum = 0 ;
         $.each( $scope.AllForm, function( key, formInfo ){
            //检查普通页port、dbpath配置
            if( key == 'normal' )
            {
               var isAllClear = $scope.AllForm['normal'].check( function( formVal ){
                  var error = [] ;
                  if( checkPort( formVal['port'] ) == false )
                  {
                     error.push( { 'name': 'port', 'error': sprintf( $scope.autoLanguage( '?格式错误。' ), $scope.autoLanguage( '服务名' ) ) } ) ;
                  }
                  if( formVal['dbpath'].length == 0 )
                  {
                     error.push( { 'name': 'dbpath', 'error': sprintf( $scope.autoLanguage( '?长度不能小于?。' ), $scope.autoLanguage( '数据路径' ), 1 ) } ) ;
                  }
                  return error ;
               } ) ;
               if( isAllClear == false )
               {
                  ++checkErrNum ;
               }
            }
            else
            {
               if( formInfo.check() == false )
               {
                  ++checkErrNum ;
               }
            }
            
         } ) ;

         if ( checkErrNum > 0 )
         {
            return false ;
         }

         $.each( $scope.AllForm, function( key, formInfo ){
            var formVal = formInfo.getValue() ;

            $.each( formVal, function( key2, value ){
               if(  value.length > 0 || ( typeof( value ) == 'number' && isNaN( value ) == false ) )
               {
                  allFormVal[key2] = value ;
               }
            } ) ;
         } ) ;

         config['Config'] = [ {} ] ;
         $.each( allFormVal, function( key, value ){
            config['Config'][0][key] = value ;
         } ) ;

         return config ;
      }

      $scope.GotoInstall = function(){
         var oldConfigure = convertConfig() ;
         if( oldConfigure == false )
         {
            return ;
         }
         var configure = {} ;
         $.each( oldConfigure, function( key, value ){
            configure[key] = value ;
         } ) ;
         configure['Config'] = [] ;
         $.each( oldConfigure['Config'], function( nodeIndex, nodeInfo ){
            var nodeConfig = {} ;
            $.each( nodeInfo, function( key, value ){
               if( value.length > 0 )
               {
                   nodeConfig[key] = value ;
               }
            } ) ;
            configure['Config'].push( nodeConfig ) ;
         } ) ;
         if( configure )
            installOltp( configure ) ;
      }

   } ) ;
}());