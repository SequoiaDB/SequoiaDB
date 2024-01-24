//@ sourceURL=Deploy.MariaDB.Mod.Ctrl.js
//"use strict" ;
(function(){
   var sacApp = window.SdbSacManagerModule ;
   //控制器
   sacApp.controllerProvider.register( 'Deploy.MariaDB.Mod.Ctrl', function( $scope, $compile, $location, $rootScope, $interval, SdbRest, SdbFunction, Loading ){
      
      var configure      = $rootScope.tempData( 'Deploy', 'ModuleConfig' ) ;
      $scope.ModuleName  = $rootScope.tempData( 'Deploy', 'ModuleName' ) ;
      var deployType     = $rootScope.tempData( 'Deploy', 'Model' ) ;
      var clusterName    = $rootScope.tempData( 'Deploy', 'ClusterName' ) ;
      if( deployType == null || clusterName == null || $scope.ModuleName == null || configure == null )
      {
         $location.path( '/Deploy/Index' ).search( { 'r': new Date().getTime() } ) ;
         return ;
      }
      $scope.stepList = _Deploy.BuildSdbMysqlStep( $scope, $location, $scope['Url']['Action'] ) ;
      if( $scope.stepList['info'].length == 0 )
      {
         $location.path( '/Deploy/Index' ).search( { 'r': new Date().getTime() } ) ;
         return ;
      }

      $scope.ShowType  = 1 ;
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
         $scope.Form1['inputList'][1]['enable'] = isDeployPackage ;
         $scope.Form1['inputList'][2]['enable'] = isDeployPackage ;

         if( hasPackage )
         {
            $.each( $scope.Form1['inputList'], function( index ){
               var name = $scope.Form1['inputList'][index]['name'] ;

               if ( name == 'HostName' )
               {
                  return true ;
               }
               else if ( hasKey( buildConf[0], name ) )
               {
                  var tmp = buildConf[0][name] ;

                  $scope.Form1['inputList'][index]['value'] = tmp ;
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
               rootPath = '/opt/sequoiasql/mariadb' ;
            }
            else if( rootPath.indexOf( 'sequoiasql/mariadb' ) < 0 )
            {
               rootPath = catPath( rootPath, 'sequoiasql/mariadb' ) ;
            }

            $.each( $scope.Form1['inputList'], function( index ){
               var name = $scope.Form1['inputList'][index]['name'] ;

               if ( name == 'HostName' )
               {
                  return true ;
               }
               else if ( hasKey( buildConf[0], name ) )
               {
                  var tmp = buildConf[0][name] ;

                  if ( name == 'dbpath' )
                  {
                     tmp = catPath( rootPath, 'database/3306' ) ;
                  }

                  $scope.Form1['inputList'][index]['value'] = tmp ;
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
         $scope.Form1 = {
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

         $scope.Form1['inputList'].splice( 0, 0, {
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

         $scope.Form1['inputList'].splice( 1, 0, {
            "name": "InstallPath",
            "webName": $scope.autoLanguage( '安装路径' ),
            "desc": $scope.autoLanguage( 'MariaDB 的安装路径' ),
            "type": "string",
            "enable": true,
            "required": true,
            "value": '/opt/sequoiasql/mariadb/',
            "valid": {
               "min": 1
            }
         } ) ;

         $scope.Form1['inputList'].splice( 2, 0, {
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

         $scope.Form1['inputList'].splice( 3, 0, {
            "name": "DatabaseAuth",
            "webName": $scope.autoLanguage( 'MariaDB 账号' ),
            "desc": $scope.autoLanguage( '创建用于访问 MariaDB 实例的账号' ),
            "type": "group",
            "child": [
               {
                  "name": "AuthUser",
                  "webName": $scope.autoLanguage( '用户名' ),
                  "type": "string",
                  "required": true,
                  "value": 'root',
                  "valid": {
                     "min": 1,
                     "max": 32,
                     "regex": '^[0-9a-zA-Z]+$'
                  }
               },
               {
                  "name": "AuthPasswd",
                  "webName": $scope.autoLanguage( '密码' ),
                  "type": "password",
                  "required": false,
                  "value": '',
                  "valid": {}
               }
            ]
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
                        if( packageInfo['Name'] == 'sequoiasql-mariadb' )
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

      function installMysql( installConfig )
      {
         var data = { 'cmd': 'add business', 'Force': true, 'ConfigInfo': JSON.stringify( installConfig ) } ;
         SdbRest.OmOperation( data, {
            'success': function( taskInfo ){
               $rootScope.tempData( 'Deploy', 'ModuleTaskID', taskInfo[0]['TaskID'] ) ;
               $location.path( '/Deploy/Task/Module' ).search( { 'r': new Date().getTime() } ) ;
            },
            'failed': function( errorInfo ){
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  installMysql( installConfig ) ;
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

         var allFormVal = {} ;

         var isAllClear1 = $scope.Form1.check( function( formVal ){
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

         if( isAllClear1 == false )
         {
            return false ;
         }

         var filterDeployKey = [ 'InstallPath', 'SystemAdmin' ] ;

         var formVal1 = $scope.Form1.getValue() ;

         $.each( formVal1, function( key2, value ){
            if ( isDeployPackage == true && filterDeployKey.indexOf( key2 ) >= 0 )
            {
               return true ;
            }
            if( key2 == 'DatabaseAuth' )
            {
               allFormVal['AuthUser'] = value['AuthUser'] ;
               allFormVal['AuthPasswd'] = value['AuthPasswd'] ;
            }
            else if( value.length > 0 || ( typeof( value ) == 'number' && isNaN( value ) == false ) )
            {
               allFormVal[key2] = value ;
            }
         } ) ;

         config['Config'] = [ {} ] ;
         $.each( allFormVal, function( key, value ){
            config['Config'][0][key] = value ;
         } ) ;

         return config ;
      }

      function convertDeployConfig()
      {
         var allFormVal = {} ;

         var formVal = $scope.Form1.getValue() ;
         $.each( formVal, function( key2, value ){
            if( key2 == 'SystemAdmin' )
            {
               allFormVal['User'] = value['User'] ;
               allFormVal['Passwd'] = value['Passwd'] ;
            }
            else if(  value.length > 0 || ( typeof( value ) == 'number' && isNaN( value ) == false ) )
            {
               allFormVal[key2] = value ;
            }
         } ) ;

         var config = {} ;
         config['ClusterName'] = installConfig['ClusterName'] ;
         config['PackageName'] = 'sequoiasql-mariadb' ;
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
               if( value.length > 0 )
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
               installMysql( configure ) ;
            }
         }
      }

   } ) ;
}());