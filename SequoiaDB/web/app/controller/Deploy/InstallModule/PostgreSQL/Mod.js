//@ sourceURL=Deploy.PG.Mod.Ctrl.js
//"use strict" ;
(function(){
   var sacApp = window.SdbSacManagerModule ;
   //控制器
   sacApp.controllerProvider.register( 'Deploy.PG.Mod.Ctrl', function( $scope, $compile, $location, $rootScope, $interval, SdbRest, SdbFunction, Loading ){
      
      var configure      = $rootScope.tempData( 'Deploy', 'ModuleConfig' ) ;
      $scope.ModuleName  = $rootScope.tempData( 'Deploy', 'ModuleName' ) ;
      var deployType     = $rootScope.tempData( 'Deploy', 'Model' ) ;
      var clusterName    = $rootScope.tempData( 'Deploy', 'ClusterName' ) ;
      if( deployType == null || clusterName == null || $scope.ModuleName == null || configure == null )
      {
         $location.path( '/Deploy/Index' ).search( { 'r': new Date().getTime() } ) ;
         return ;
      }
      $scope.stepList = _Deploy.BuildSdbPgsqlStep( $scope, $location, $scope['Url']['Action'] ) ;
      if( $scope.stepList['info'].length == 0 )
      {
         $location.path( '/Deploy/Index' ).search( { 'r': new Date().getTime() } ) ;
         return ;
      }

      $scope.InputErrNum = {} ;
      $scope.ShowType = 'normal' ;

      var isDeployPackage = true ;
      var hostSelectList = [] ;
      var installConfig = [] ;
      var buildConf = [] ;
      //切换分类
      $scope.SwitchParam = function( type ){
         $scope.ShowType = type ;
      }

      function resetFormValue( defaultHostIndex, hasPackage )
      {
         isDeployPackage = !hasPackage ;
         $scope.AllForm['normal']['inputList'][1]['enable'] = isDeployPackage ;
         $scope.AllForm['normal']['inputList'][2]['enable'] = isDeployPackage ;

         if( hasPackage )
         {
            $.each( $scope.AllForm['normal']['inputList'], function( index ){
               var name = $scope.AllForm['normal']['inputList'][index]['name'] ;

               if ( name == 'HostName' )
               {
                  return true ;
               }
               else if ( hasKey( buildConf[0], name ) )
               {
                  var tmp = buildConf[0][name] ;

                  $scope.AllForm['normal']['inputList'][index]['value'] = tmp ;
               }
            } ) ;
         }
         else
         {
            var defaultMount = '/' ;

            $.each( hostSelectList[defaultHostIndex]['hostInfo']['Disk'], function( index, info ){
               if( info['IsLocal'] == true )
               {
                  defaultMount = info['Mount'] ;
                  return false ;
               }
            } ) ;

            var rootPath = defaultMount ;

            if( rootPath == '/' )
            {
               rootPath = '/opt/sequoiasql/postgresql' ;
            }
            else if( rootPath.indexOf( 'sequoiasql/postgresql' ) < 0 )
            {
               rootPath = catPath( rootPath, 'sequoiasql/postgresql' ) ;
            }

            $.each( $scope.AllForm['normal']['inputList'], function( index ){
               var name = $scope.AllForm['normal']['inputList'][index]['name'] ;

               if ( name == 'HostName' )
               {
                  return true ;
               }
               else if ( hasKey( buildConf[0], name ) )
               {
                  var tmp = buildConf[0][name] ;

                  if ( name == 'dbpath' )
                  {
                     tmp = catPath( rootPath, 'database/5432' ) ;
                  }

                  $scope.AllForm['normal']['inputList'][index]['value'] = tmp ;
               }
            } ) ;
         }
      }

      //生成form表单
      function buildConfForm( config )
      {
         var defaultHostIndex = 0 ;
         var defaultHostName = '' ;
         var hasPackage = false ;

         $scope.Template = config[0]['Property'] ;
         $scope.AllForm = {} ;
         $scope.AllForm['normal'] = {
            'keyWidth': '200px',
            'inputList': _Deploy.ConvertTemplate( $scope.Template, 0 )
         } ;

         $.each( hostSelectList, function( index, info ){
            if( buildConf[0]['HostName'] == info['hostInfo']['HostName'] )
            {
               defaultHostIndex = index ;
               defaultHostName = info['hostInfo']['HostName'] ;
               hasPackage = info['hasPackage'] ;
               return false ;
            }
         } ) ;

         $scope.AllForm['normal']['inputList'].splice( 0, 0, {
            "name": "HostName",
            "webName": $scope.autoLanguage( '主机名' ),
            "type": "select",
            "required": true,
            "value": defaultHostName,
            "valid": hostSelectList,
            "onChange": function( name, key, value ){
               var tmpIndex = 0 ;
               var tmpHasPKG = false ;

               $.each( hostSelectList, function( index, info ){
                  if( value == info['hostInfo']['HostName'] )
                  {
                     tmpIndex = index ;
                     tmpHasPKG = info['hasPackage'] ;
                     return false ;
                  }
               } ) ;

               configure['HostInfo'] = [ { 'HostName': value } ] ;

               getModuleConfig( configure, function(){
                  resetFormValue( tmpIndex, tmpHasPKG ) ;
               } ) ;
            }
         } ) ;

         $scope.AllForm['normal']['inputList'].splice( 1, 0, {
            "name": "InstallPath",
            "webName": $scope.autoLanguage( '安装路径' ),
            "desc": $scope.autoLanguage( 'PostgreSQL 的安装路径' ),
            "type": "string",
            "enable": true,
            "required": true,
            "value": '/opt/sequoiasql/postgresql/',
            "valid": {
               "min": 1
            }
         } ) ;

         $scope.AllForm['normal']['inputList'].splice( 2, 0, {
            "name": "SystemAdmin",
            "webName": $scope.autoLanguage( '系统管理员' ),
            "desc": $scope.autoLanguage( '操作系统的管理员账号' ),
            "type": "group",
            "enable": true,
            "child": [
               {
                  "name": "User",
                  "webName": $scope.autoLanguage( '用户名' ),
                  "type": "string",
                  "required": true,
                  "value": "root",
                  "valid": {
                     "min": 1
                  }
               },
               {
                  "name": "Passwd",
                  "webName": $scope.autoLanguage( '密码' ),
                  "type": "password",
                  "required": true,
                  "value": "",
                  "valid": {
                     "min": 1
                  }
               } 
            ]
         } ) ;

         var otherForm = _Deploy.ConvertTemplate( $scope.Template, 1, true, true ) ;
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

         resetFormValue( defaultHostIndex, hasPackage ) ;
      }

      //获取配置
      function getModuleConfig( configure, func )
      {
         var data = { 'cmd': 'get business config', 'TemplateInfo': JSON.stringify( configure ) } ;
         SdbRest.OmOperation( data, {
            'success': function( config ){
               installConfig = config[0] ;
               buildConf = config[0]['Config'] ;
               func( config ) ;
            },
            'failed': function( errorInfo ){
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  getModuleConfig( configure, func ) ;
                  return true ;
               } ) ;
            }
         } ) ;
      }

      //获取主机列表
      function getHostList()
      {
         var data = { 'cmd': 'query host' } ;
         SdbRest.OmOperation( data, {
            'success': function( hostList ){
               configure['HostInfo'] = [] ;
               $.each( hostList, function( index, hostInfo ){
                  if( hostInfo['ClusterName'] == clusterName )
                  {
                     var hasPackage = false ;
                     $.each( hostInfo['Packages'], function( packageIndex, packageInfo ){
                        if( packageInfo['Name'] == 'sequoiasql-postgresql' )
                        {
                           hasPackage = true ;
                           return false ;
                        }
                     } ) ;

                     hostSelectList.push( {
                        'key':   hostInfo['HostName'] + ' [' + hostInfo['IP'] + ']',
                        'value': hostInfo['HostName'],
                        'hostInfo':  hostInfo,
                        'hasPackage': hasPackage
                     } ) ;

                     if( hasPackage )
                     {
                        configure['HostInfo'].push( { 'HostName': hostInfo['HostName'] } ) ;
                     }
                  }
               } ) ;

               hostSelectList.sort( function( a, b ){
                  return a.key > b.key ? 1 : a.key < b.key ? -1 : 0 ;
               } ) ;

               getModuleConfig( configure, buildConfForm ) ;
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

      function installPgsql( installConfig )
      {
         var data = { 'cmd': 'add business', 'ConfigInfo': JSON.stringify( installConfig ) } ;
         SdbRest.OmOperation( data, {
            'success': function( taskInfo ){
               $rootScope.tempData( 'Deploy', 'ModuleTaskID', taskInfo[0]['TaskID'] ) ;
               $location.path( '/Deploy/Task/Module' ).search( { 'r': new Date().getTime() } ) ;
            },
            'failed': function( errorInfo ){
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  installPgsql( installConfig ) ;
                  return true ;
               } ) ;
            }
         } ) ;
      }

      function convertConfig()
      {
         var config = {} ;
         config['ClusterName']  = installConfig['ClusterName'] ;
         config['BusinessType'] = installConfig['BusinessType'] ;
         config['BusinessName'] = installConfig['BusinessName'] ;
         config['DeployMod']    = installConfig['DeployMod'] ;

         var firstErrFormName = null ;
         var allFormVal = {} ;
         var isAllClear = true ;
         $.each( $scope.AllForm, function( key, formInfo ){
            var inputErrNum = 0 ;
            if( key == 'normal' )
            {
               inputErrNum = formInfo.getErrNum( function( formVal ){
                  //检查普通页port、dbpath配置
                  var error = [] ;
                  if( checkPort( formVal['port'] ) == false )
                  {
                     error.push( { 'name': 'port', 'error': sprintf( $scope.autoLanguage( '?格式错误。' ), $scope.autoLanguage( '端口' ) ) } ) ;
                  }
                  if( formVal['dbpath'].length == 0 )
                  {
                     error.push( { 'name': 'dbpath', 'error': sprintf( $scope.autoLanguage( '?长度不能小于?。' ), $scope.autoLanguage( '数据路径' ), 1 ) } ) ;
                  }
                  return error ;
               } ) ;
               if( inputErrNum > 0 )
               {
                  if( isNull( firstErrFormName ) )
                  {
                     firstErrFormName = key ;
                  }
                  isAllClear = false ;
               }
            }
            else
            {
               inputErrNum = formInfo.getErrNum() ;
               if( inputErrNum > 0 )
               {
                  if( isNull( firstErrFormName ) )
                  {
                     firstErrFormName = key ;
                  }
                  isAllClear = false ;
               }
            }
            $scope.InputErrNum[key] = inputErrNum ;
         } ) ;

         if ( isAllClear == false )
         {
            $scope.SwitchParam( firstErrFormName ) ;
            $scope.AllForm[firstErrFormName].scrollToError( null, -10 ) ;
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

         var filterDeployKey = [ 'InstallPath', 'SystemAdmin' ] ;

         config['Config'] = [ {} ] ;
         $.each( allFormVal, function( key, value ){
            if ( isDeployPackage == true && filterDeployKey.indexOf( key ) >= 0 )
            {
               return true ;
            }
            config['Config'][0][key] = value ;
         } ) ;

         return config ;
      }

      function convertDeployConfig()
      {
         var allFormVal = {} ;

         $.each( $scope.AllForm, function( key, formInfo ){
            var formVal = formInfo.getValue() ;
            $.each( formVal, function( key2, value ){
               if( key == 'normal' && key2 == 'SystemAdmin' )
               {
                  allFormVal['User'] = value['User'] ;
                  allFormVal['Passwd'] = value['Passwd'] ;
               }
               else if(  value.length > 0 || ( typeof( value ) == 'number' && isNaN( value ) == false ) )
               {
                  allFormVal[key2] = value ;
               }
            } ) ;
         } ) ;

         var config = {} ;
         config['ClusterName'] = installConfig['ClusterName'] ;
         config['PackageName'] = 'sequoiasql-postgresql' ;
         config['InstallPath'] = allFormVal['InstallPath'] ;
         config['HostInfo'] = JSON.stringify( { "HostInfo" : [ {
            "HostName": allFormVal['HostName'],
            "User":     allFormVal['User'],
            "Passwd":   allFormVal['Passwd']
         } ] } ) ;
         config['User']       = allFormVal['User'] ;
         config['Passwd']     = '-' ;
         config['Enforced']   = false;

         return config ;
      }

      function isFilterDefaultItem( key, value ) {
         var result = false ;
         var unSkipList = [ 'HostName', 'dbpath', 'port' ] ;
         if ( unSkipList.indexOf( key ) >= 0 )
         {
            return false ;
         }
         $.each( $scope.Template, function( index, item ){
            if ( item['Name'] == key )
            {
               result = ( item['Default'] == value ) ;
               return false ;
            }
         } ) ;
         return result ;
      }

      function deployPackage( addBuzConfig )
      {
         var config = convertDeployConfig() ;



         var data = {
            'cmd': 'deploy package',
            'ClusterName': config['ClusterName'],
            'PackageName': config['PackageName'],
            'InstallPath': config['InstallPath'],
            'HostInfo':    config['HostInfo'],
            'User':        config['User'],
            'Passwd':      config['Passwd'],
            'Enforced':    config['Enforced']
         } ;
         SdbRest.OmOperation( data, {
            'success': function( taskInfo ){
               $rootScope.tempData( 'Deploy', 'SecondTask', addBuzConfig ) ;
               $rootScope.tempData( 'Deploy', 'ModuleTaskID', taskInfo[0]['TaskID'] ) ;
               $location.path( '/Deploy/Task/Module' ).search( { 'r': new Date().getTime() } ) ;
            },
            'failed': function( errorInfo ){
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  deployPackage( addBuzConfig ) ;
                  return true ;
               } ) ;
            }
         } ) ;
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
               if( value.length > 0 && isFilterDefaultItem( key, value ) == false )
               {
                   nodeConfig[key] = value ;
               }
            } ) ;
            configure['Config'].push( nodeConfig ) ;
         } ) ;

         if( configure )
         {
            if ( isDeployPackage )
            {
               deployPackage( configure ) ;
            }
            else
            {
               installPgsql( configure ) ;
            }
         }
      }

   } ) ;
}());