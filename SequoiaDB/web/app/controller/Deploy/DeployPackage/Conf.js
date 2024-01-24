//@ sourceURL=Conf.js
//"use strict" ;
(function(){
   var sacApp = window.SdbSacManagerModule ;
   //控制器
   sacApp.controllerProvider.register( 'Deploy.Package.Conf.Ctrl', function( $scope, $location, $rootScope, SdbRest, SdbSwap, SdbSignal ){
      
      var deployModel  = $rootScope.tempData( 'Deploy', 'Model' ) ;
      var deplpyModule = $rootScope.tempData( 'Deploy', 'Module' ) ;
      SdbSwap.clusterName  = $rootScope.tempData( 'Deploy', 'ClusterName' ) ;
      SdbSwap.hostList = $rootScope.tempData( 'Deploy', 'HostList' ) ;
      if( deployModel == null || SdbSwap.clusterName == null || deplpyModule == null || SdbSwap.hostList == null )
      {
         $location.path( '/Deploy/Index' ).search( { 'r': new Date().getTime() } ) ;
         return ;
      }

      $scope.stepList = _Deploy.BuildDeployPackageStep( $scope, $location, $scope['Url']['Action'], deployModel ) ;

      if( $scope.stepList['info'].length == 0 )
      {
         $location.path( '/Deploy/Index' ).search( { 'r': new Date().getTime() } ) ;
         return ;
      }

      $scope.Form = {
         'inputList': [
            {
               "name": "ClusterName",
               "webName": $scope.autoLanguage( '集群名' ),
               "type": "normal",
               "required": true,
               "value": SdbSwap.clusterName,
               "disabled": true
            },
            {
               "name": "PackageName",
               "webName": $scope.autoLanguage( '安装包名' ),
               "type": "select",
               "required": true,
               "value": 'sequoiasql-mysql',
               "valid": [
                  { 'key': 'SequoiaSQL-MySQL', 'value': 'sequoiasql-mysql' },
                  { 'key': 'SequoiaSQL-MariaDB', 'value': 'sequoiasql-mariadb' },
                  { 'key': 'SequoiaSQL-PostgreSQL', 'value': 'sequoiasql-postgresql' }
               ],
               "onChange": function( name, key, value ){
                  if( value == 'sequoiasql-mysql' )
                  {
                     $scope.Form['inputList'][2]['value'] = '/opt/sequoiasql/mysql/' ;
                  }
                  else if( value == 'sequoiasql-mariadb' )
                  {
                     $scope.Form['inputList'][2]['value'] = '/opt/sequoiasql/mariadb/' ;
                  }
                  else
                  {
                     $scope.Form['inputList'][2]['value'] = '/opt/sequoiasql/postgresql/' ;
                  }

                  SdbSignal.commit( "GetCheck", [  $scope.Form['inputList'][5]['value'], value ] ) ;
               }
            },
            {
               "name": "InstallPath",
               "webName": $scope.autoLanguage( '安装路径' ),
               "type": "string",
               "required": true,
               "value": '/opt/sequoiasql/mysql/',
               "valid": {
                  "min": 1
               }
            },
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
               "value": ""
            },
            {
               "name": "Enforced",
               "webName": $scope.autoLanguage( '强制安装' ),
               "type": "select",
               "required": true,
               "value": false,
               "valid": [
                  { 'key': false, 'value': false },
                  { 'key': true, 'value': true }
               ],
               "onChange": function( name, key, value ){
                  //切换强制安装修改选中主机
                  SdbSignal.commit( "GetCheck", [ value, $scope.Form['inputList'][1]['value'] ] ) ;
               }
            }
         ]
      } ;

      //开始安装
      var deployPackage = function( packageInfo ){
         var data = {
            'cmd': 'deploy package',
            'ClusterName': packageInfo['ClusterName'],
            'PackageName': packageInfo['PackageName'],
            'InstallPath': packageInfo['InstallPath'],
            'HostInfo': JSON.stringify( packageInfo['HostInfo'] ),
            'User': packageInfo['User'],
            'Passwd': packageInfo['Passwd'],
            'Enforced': packageInfo['Enforced']
         }
         SdbRest.OmOperation( data, {
            'success': function( taskInfo ){
               $rootScope.tempData( 'Deploy', 'HostTaskID', taskInfo[0]['TaskID'] ) ;
               $location.path( '/Deploy/Task/Host' ).search( { 'r': new Date().getTime() } ) ;
            },
            'failed': function( errorInfo ){
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  deployPackage( packageInfo ) ;
                  return true ;
               } ) ;
            }
         } ) ;
      }
      
      //上一步
      $scope.GotoDeploy = function(){
         $location.path( '/Deploy/Index' ).search( { 'r': new Date().getTime() } ) ;
      }

      //下一步
      $scope.GotoAddHost = function(){
         var isAllClear = $scope.Form.check( function( thisValue ){
            if( thisValue['Passwd'].length == 0 )
            {
               var num = 0 ;
               $.each( SdbSwap.hostList, function( index, hostInfo ){
                  if( typeof( hostInfo['Passwd'] ) == 'undefined' && hostInfo['Checked'] == true )
                  {
                     ++num ;
                  }
               } ) ;
               if( num > 0 )
               {
                  return [ { 'name': 'Passwd', 'error': sprintf( $scope.autoLanguage( '?长度不能小于?。' ), $scope.autoLanguage( '密码' ), 1 ) } ] ;
               }
            }
            return [] ;
         } ) ;
         if( isAllClear )
         {
            var packageInfo = $scope.Form.getValue() ;
            packageInfo['HostInfo'] = { 'HostInfo': [] } ;
            $.each( SdbSwap.hostList, function( index, hostInfo ){
               if( hostInfo['Checked'] == true )
               {
                  var info = {} ;
                  info['HostName'] = hostInfo['HostName'] ;

                  //有单独填写主机账号密码时
                  if( typeof( hostInfo['Passwd'] ) != 'undefined' )
                  {
                     info['User'] = hostInfo['User'] ;
                     info['Passwd'] = hostInfo['Passwd'] ;
                  }
                  else
                  {
                     info['User'] = packageInfo['User'] ;
                     info['Passwd'] = packageInfo['Passwd'] ;
                  }
                  packageInfo['HostInfo']['HostInfo'].push( info ) ;
               }
            } ) ;
            packageInfo['Passwd'] = '-' ;
            if( packageInfo['HostInfo']['HostInfo'].length == 0 )
            {
               $scope.Components.Confirm.type = 3 ;
               $scope.Components.Confirm.context = $scope.autoLanguage( '至少选择一台主机，才可以进入下一步操作。' ) ;
               $scope.Components.Confirm.isShow = true ;
               $scope.Components.Confirm.noClose = true ;
               return ;
            }
            deployPackage( packageInfo ) ;
         }
      } ;
   } ) ;

   //right-table controller
   sacApp.controllerProvider.register( 'Deploy.Package.Conf.Right.Ctrl', function( $scope, $rootScope, SdbRest, SdbSwap, SdbSignal ){
      $scope.CheckStatus = false ;
      //主机列表表格
      $scope.HostListTable = {
         'title': {
            'Checked':   '',
            'HostName':   $scope.autoLanguage( '主机名' ),
            'IP':         $scope.autoLanguage( 'IP地址' ),
            'User':       $scope.autoLanguage( '用户名' ),
            'Passwd':     $scope.autoLanguage( '密码' ),
            'Package':    $scope.autoLanguage( '已部署' )
         },
         'body': [],
         'options': {
            'width': {
               'Checked':         '25px',
               'HostName':         '18%',
               'IP':               '18%',
               'User':             '18%',
               'Passwd':           '18%',
               'Package':          '28%'
            },
            'max': 50
         },
         'callback': {}
      } ;

      if( SdbSwap.hostList != null )
      {
         $.each( SdbSwap.hostList, function( index, hostInfo ){
            hostInfo['Package'] = '' ;
            hostInfo['Checked'] = true ;
            $.each( hostInfo['Packages'], function( index2, packageInfo ){
               if( hostInfo['Package'].length == 0 )
               {
                  hostInfo['Package'] = packageInfo['Name'] ;
               }
               else
               {
                  hostInfo['Package'] = hostInfo['Package'] + ',' + packageInfo['Name'] ;
               }
            } ) ;
            if( hostInfo['Package'].indexOf( 'sequoiasql-postgresql' ) > 0 )
            {
               hostInfo['Checked'] = false ;
            }
         } ) ;

         $scope.HostListTable['body'] = SdbSwap.hostList ;

      }

      //检测主机是否可以部署包
      SdbSignal.on( 'GetCheck', function( result ){
         if( result[0] == true )
         {
            $.each( SdbSwap.hostList, function( index, hostInfo ){
               hostInfo['Checked'] = true ;
            } ) ;
         }
         else
         {
            $.each( SdbSwap.hostList, function( index, hostInfo ){
               if( hostInfo['Package'].indexOf( result[1] ) < 0 )
               {
                  hostInfo['Checked'] = true ;
               }
               else
               {
                  hostInfo['Checked'] = false ;
               }
            } ) ;
         }
      } ) ;
      
      //修改主机信息 弹窗
      $scope.ChangeHostInfoWindow = {
         'config': {},
         'callback': {}
      } ;

      //打开 修改主机信息 弹窗
      $scope.ShowChangeHostInfo = function( index ){
         if( SdbSwap.hostList[index]['Checked'] == false )
         {
            return ;
         }
         var hostInfo = SdbSwap.hostList[index] ;
         $scope.ChangeHostInfoWindow['config'] = {
            'inputList': [
               {
                  "name": "HostName",
                  "webName": $scope.autoLanguage( 'IP地址/主机名' ),
                  "type": "string",
                  "value": hostInfo['HostName'] ? hostInfo['HostName'] : hostInfo['IP'],
                  "valid": {
                     "min": 1
                  },
                  'disabled': true
               },
               {
                  "name": "User",
                  "webName": $scope.autoLanguage( '用户名' ),
                  "type": "string",
                  "value": hostInfo['User'],
                  "valid": {
                     "min": 1
                  }
               },
               {
                  "name": "Passwd",
                  "webName": $scope.autoLanguage( '密码' ),
                  "type": "password",
                  "value": hostInfo['Passwd'],
                  "valid": {}
               }
            ]
         } ;
         $scope.ChangeHostInfoWindow['callback']['SetOkButton']( $scope.autoLanguage( '确定' ), function(){
            var isAllClear = $scope.ChangeHostInfoWindow['config'].check() ;
            if( isAllClear )
            {
               var formVal = $scope.ChangeHostInfoWindow['config'].getValue() ;
               hostInfo['User'] = formVal['User'] ;
               if( typeof( formVal['Passwd'] ) != 'undefined' )
               {
                  hostInfo['Passwd'] = formVal['Passwd'] ;
               }
               else
               {
                  SdbSwap.hostList[index] = deleteJson( hostInfo, 'Passwd' ) ;
               }
            }
            return isAllClear ;
         } ) ;
         $scope.ChangeHostInfoWindow['callback']['SetTitle']( $scope.autoLanguage( '修改主机信息' ) ) ;
         $scope.ChangeHostInfoWindow['callback']['Open']() ;
      }
   } ) ;
}());