//@ sourceURL=Scan.js
//"use strict" ;
(function(){
   var sacApp = window.SdbSacManagerModule ;
   //控制器
   sacApp.controllerProvider.register( 'Deploy.Scan.Ctrl', function( $scope, $compile, $location, $rootScope, SdbRest ){

      //初始化
      var scanHostTmp   = $rootScope.tempData( 'Deploy', 'ScanHost' ) ;
      var successNum    = 0 ;
      var hostList      = scanHostTmp == null ? [] : scanHostTmp ;
      $scope.checkedHostNum  = 0 ;
      //主机列表表格
      $scope.HostListTable = {
         'title': {
            'Check':      '',
            'Errno':      $scope.autoLanguage( '状态' ),
            'HostName':   $scope.autoLanguage( '主机名' ),
            'IP':         $scope.autoLanguage( 'IP地址' ),
            'User':       $scope.autoLanguage( '用户名' ),
            'Password':   $scope.autoLanguage( '密码' ),
            'SSH':        $scope.autoLanguage( 'SSH端口' ),
            'Proxy':      $scope.autoLanguage( '代理端口' )
         },
         'body': hostList,
         'options': {
            'width': {
               'Check':            '30px',
               'Errno':            16,
               'HostName':         20,
               'IP':               16,
               'User':             12,
               'Password':         12,
               'SSH':              12,
               'Proxy':            12
            },
            'max': 50,
            
            'text': {
               'default': sprintf( $scope.autoLanguage( '已扫描 ? 台主机, 共 ? 台主机连接成功。' ), hostList.length, successNum )
            }
         },
         'callback': {}
      } ;

      $scope.ScanForm = {
         'inputList': [
            {
               "name": "address",
               "webName": $scope.autoLanguage( 'IP地址/主机名' ),
               "type": "text",
               "value": "",
               "valid": {
                  "min": 1
               }
            },
            {
               "name": "user",
               "webName": $scope.autoLanguage( '用户名' ),
               "type": "string",
               "value": "root",
               "valid": {
                  "min": 1
               },
               'disabled': true
            },
            {
               "name": "password",
               "webName": $scope.autoLanguage( '密码' ),
               "type": "password",
               "value": "",
               "valid": {
                  "min": 1
               },
               'onKeypress': function( event ){
                  $scope.ScanHost( event ) ;
               }
            },
            {
               "name": "ssh",
               "webName": $scope.autoLanguage( 'SSH端口' ),
               "type": "string",
               "value": "22",
               "valid": {
                  "min": 1
               },
               'onKeypress': function( event ){
                  $scope.ScanHost( event ) ;
               }
            },
            {
               "name": "proxy",
               "webName": $scope.autoLanguage( '代理端口' ),
               "type": "string",
               "value": "11790",
               "valid": {
                  "min": 1
               },
               'onKeypress': function( event ){
                  $scope.ScanHost( event ) ;
               }
            }
         ]
      } ;

      //帮助窗口
      $scope.Helper = {
         'config': {},
         'callback': {}
      } ;

      var deployModel  = $rootScope.tempData( 'Deploy', 'Model' ) ;
      var deplpyModule = $rootScope.tempData( 'Deploy', 'Module' ) ;
      var clusterName  = $rootScope.tempData( 'Deploy', 'ClusterName' ) ;
      var discoverConf = $rootScope.tempData( 'Deploy', 'DiscoverConf' ) ;
      var syncConf     = $rootScope.tempData( 'Deploy', 'SyncConf' ) ;

      if( deployModel == null || clusterName == null || deplpyModule == null )
      {
         $location.path( '/Deploy/Index' ).search( { 'r': new Date().getTime() } ) ;
         return ;
      }

      if( discoverConf != null )
      {
         $scope.stepList = _Deploy.BuildDiscoverStep( $scope, $location, $scope['Url']['Action'], deplpyModule ) ;
         var discoverHostList  = $rootScope.tempData( 'Deploy', 'DiscoverHostList' ) ;
         $.each( discoverHostList, function( index, value ){
            if( $scope.ScanForm['inputList'][0]['value'] == '' )
            {
               $scope.ScanForm['inputList'][0]['value'] = value ;
            }
            else
            {
               $scope.ScanForm['inputList'][0]['value'] = $scope.ScanForm['inputList'][0]['value'] + ',' + value ;
            }
         } ) ;
      }
      else if( syncConf != null )
      {
         $scope.stepList = _Deploy.BuildSyncStep( $scope, $location, $scope['Url']['Action'], deplpyModule ) ;
         var syncConf  = $rootScope.tempData( 'Deploy', 'SyncConf' ) ;
         $.each( syncConf, function( index, value ){
            if( $scope.ScanForm['inputList'][0]['value'] == '' )
            {
               $scope.ScanForm['inputList'][0]['value'] = value ;
            }
            else
            {
               $scope.ScanForm['inputList'][0]['value'] = $scope.ScanForm['inputList'][0]['value'] + ',' + value ;
            }
         } ) ;
      }
      else
      {
         $scope.stepList = _Deploy.BuildSdbStep( $scope, $location, deployModel, $scope['Url']['Action'], deplpyModule ) ;
      }

      if( $scope.stepList['info'].length == 0 )
      {
         $location.path( '/Deploy/Index' ).search( { 'r': new Date().getTime() } ) ;
         return ;
      }

      $scope.ClearInput = function(){
         $scope.ScanForm['inputList'][0]['value'] = '' ;
         $scope.ScanForm['inputList'][2]['value'] = '' ;
      }

      $scope.CountCheckedHostNum = function(){
         setTimeout( function(){
            $scope.checkedHostNum = 0 ;
            $.each( hostList, function( index, hostInfo ){
               if( hostInfo['checked'] == true )
               {
                  ++$scope.checkedHostNum ;
               }
            } ) ;
            $scope.$apply() ;
         } ) ;
      }

      var countSuccessHostNum = function(){
         successNum = 0 ;
         $.each( hostList, function( index2, hostInfo ){
            if( hostInfo['Errno'] == 0 )
            {
               ++successNum ;
            }
         } ) ;
      }

      countSuccessHostNum() ;
      $scope.CountCheckedHostNum() ;

      $scope.SelectAll = function(){
         $.each( hostList, function( index ){
            if( hostList[index]['Errno'] == 0 )
            {
               hostList[index]['checked'] = true ;
            }
         } ) ;
      }

      $scope.Unselected = function(){
         $.each( hostList, function( index ){
            if( hostList[index]['Errno'] == 0 )
            {
               hostList[index]['checked'] = !hostList[index]['checked'] ;
            }
         } ) ;
      }

      $scope.ClearErrorHost = function(){
         var newHostList = [] ;
         $.each( hostList, function( index, hostInfo ){
            if( hostInfo['Errno'] == 0 )
            {
               newHostList.push( hostInfo ) ;
            }
         } ) ;
         hostList = newHostList ;
         $scope.HostListTable['body'] = newHostList ;
         $scope.HostListTable['options']['text']['default'] = sprintf( $scope.autoLanguage( '已扫描 ? 台主机, 共 ? 台主机连接成功。' ), $scope.HostListTable['body'].length, $scope.HostListTable['body'].length ) ;
         $rootScope.tempData( 'Deploy', 'ScanHost', hostList ) ;
      }

      //打开帮助窗口
      $scope.ShowHelper = function(){
         //设置确定按钮
         $scope.Helper['callback']['SetOkButton']( $scope.autoLanguage( '确定' ), function(){
            $scope.Helper['callback']['Close']() ;
         } ) ;
         //设置标题
         $scope.Helper['callback']['SetTitle']( $scope.autoLanguage( '帮助' ) ) ;
         //设置图标
         $scope.Helper['callback']['SetIcon']( '' ) ;
         //打开窗口
         $scope.Helper['callback']['Open']() ;
      }

      function ScanAllHost( formVal )
      {
         var ipList = parseHostString( formVal['address'] ) ;
         var index = 0 ;
         var scanHostOnce = function(){
            if( index < ipList.length )
            {
               var hostInfo = { "ClusterName": clusterName, "HostInfo": ipList[index], "User": formVal['user'], "Passwd": formVal['password'], "SshPort": formVal['ssh'],"AgentService": formVal['proxy'] } ;
               var data = { 'cmd': 'scan host', 'HostInfo': JSON.stringify( hostInfo ) } ;
               SdbRest.OmOperation( data, {
                  'success': function( scanHostList ){
                     $.each( scanHostList, function( index, hostInfo ){
                        var isExists = checkHostIsExist( hostList, hostInfo['HostName'], hostInfo['IP'] ) ;
                        var newHostInfo = { 'checked': ( hostInfo['errno'] == 0 ? true : false ), 'Errno': hostInfo['errno'], 'Detail': hostInfo['detail'], 'HostName': hostInfo['HostName'], 'IP': hostInfo['IP'], 'User': formVal['user'], 'Password': formVal['password'], 'SSH': formVal['ssh'], 'Proxy': formVal['proxy']  } ;
                        if( isExists == -1 || isExists == -2 )
                        {
                           hostList.push( newHostInfo ) ;
                        }
                        else
                        {
                           hostList[isExists] = newHostInfo ;
                        }
                     } ) ;
                     $scope.HostListTable['body'] = hostList ;
                     $rootScope.tempData( 'Deploy', 'ScanHost', $scope.HostListTable['body'] ) ;
                     countSuccessHostNum() ;
                     $scope.HostListTable['options']['text']['default'] = sprintf( $scope.autoLanguage( '已扫描 ? 台主机, 共 ? 台主机连接成功。' ), hostList.length, successNum ) ;
                     $scope.CountCheckedHostNum() ;
                     $rootScope.bindResize() ;
                     $scope.$apply() ;
                     ++index ;
                     scanHostOnce() ;
                  },
                  'failed': function( errorInfo ){
                     _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                        $scope.ScanHost() ;
                        return true ;
                     } ) ;
                  }
               } ) ;
            }
         }
         scanHostOnce() ;
      }

      //修改主机信息 弹窗
      $scope.ChangeHostInfoWindow = {
         'config': {},
         'callback': {}
      } ;

      //打开 修改主机信息 弹窗
      $scope.ShowChangeHostInfo = function( index ){
         var hostInfo = hostList[index] ;
         $scope.ChangeHostInfoWindow['config'] = {
            'inputList': [
               {
                  "name": "address",
                  "webName": $scope.autoLanguage( 'IP地址/主机名' ),
                  "type": "string",
                  "value": hostInfo['HostName'] ? hostInfo['HostName'] : hostInfo['IP'],
                  "valid": {
                     "min": 1
                  },
                  'disabled': true
               },
               {
                  "name": "user",
                  "webName": $scope.autoLanguage( '用户名' ),
                  "type": "string",
                  "value": hostInfo['User'],
                  "valid": {
                     "min": 1
                  },
                  'disabled': true
               },
               {
                  "name": "password",
                  "webName": $scope.autoLanguage( '密码' ),
                  "type": "password",
                  "value": hostInfo['Password'],
                  "valid": {
                     "min": 1
                  }
               },
               {
                  "name": "ssh",
                  "webName": $scope.autoLanguage( 'SSH端口' ),
                  "type": "string",
                  "value": hostInfo['SSH'],
                  "valid": {
                     "min": 1
                  }
               },
               {
                  "name": "proxy",
                  "webName": $scope.autoLanguage( '代理端口' ),
                  "type": "string",
                  "value": hostInfo['Proxy'],
                  "valid": {
                     "min": 1
                  }
               }
            ]
         } ;
         $scope.ChangeHostInfoWindow['callback']['SetOkButton']( $scope.autoLanguage( '确定' ), function(){
            var isAllClear = $scope.ChangeHostInfoWindow['config'].check( function( val ){
               var errInfo = [] ;
               if( !checkPort( val['ssh'] ) )
               {
                  errInfo.push( { 'name': 'ssh', 'error': $scope.autoLanguage( 'SSH端口格式错误' ) } ) ;
               }
               if( !checkPort( val['proxy'] ) )
               {
                  errInfo.push( { 'name': 'proxy', 'error': $scope.autoLanguage( '代理端口格式错误' ) } ) ;
               }
               return errInfo ;
            } ) ;
            if( isAllClear )
            {
               var formVal = $scope.ChangeHostInfoWindow['config'].getValue() ;
               ScanAllHost( formVal ) ;
            }
            return isAllClear ;
         } ) ;
         $scope.ChangeHostInfoWindow['callback']['SetTitle']( $scope.autoLanguage( '修改主机信息' ) ) ;
         $scope.ChangeHostInfoWindow['callback']['Open']() ;
      }

      $scope.ScanHost = function( event ){
         if( typeof( event ) == 'undefined' || event['keyCode'] == 13 )
         {
            var isAllClear = $scope.ScanForm.check( function( val ){
               var errInfo = [] ;
               if( !checkPort( val['ssh'] ) )
               {
                  errInfo.push( { 'name': 'ssh', 'error': $scope.autoLanguage( 'SSH端口格式错误' ) } ) ;
               }
               if( !checkPort( val['proxy'] ) )
               {
                  errInfo.push( { 'name': 'proxy', 'error': $scope.autoLanguage( '代理端口格式错误' ) } ) ;
               }
               return errInfo ;
            } ) ;
            if( isAllClear )
            {
               var formVal = $scope.ScanForm.getValue() ;
               ScanAllHost( formVal ) ;
            }
         }
      }

      //上一步
      $scope.GotoDeploy = function(){
         if( deployModel == 'Host' )
         {
            $rootScope.tempData( 'Deploy', 'Index', 'host' ) ;
         }
         $location.path( '/Deploy/Index' ).search( { 'r': new Date().getTime() } ) ;
      }

      //下一步
      $scope.GotoAddHost = function(){
         if( $scope.checkedHostNum > 0 )
         {
            var newHostList = {
               'ClusterName': clusterName,
               'HostInfo': [],
               'User': '-',
               'Passwd': '-',
               'SshPort': '-',
               'AgentService': '-'
            } ;
            $.each( hostList, function( index, hostInfo ){
               if( hostInfo['checked'] == true )
               {
                  newHostList['HostInfo'].push( {
                     "HostName": hostInfo['HostName'],
                     "IP":       hostInfo['IP'],
                     "User":     hostInfo['User'],
                     "Passwd":   hostInfo['Password'],
                     "SshPort":  hostInfo['SSH'],
                     "AgentService": hostInfo['Proxy']
                  } ) ;
               }
            } ) ;
            $rootScope.tempData( 'Deploy', 'AddHost', newHostList ) ;
            $location.path( '/Deploy/AddHost' ).search( { 'r': new Date().getTime() } ) ;
         }
         else
         {
            $scope.Components.Confirm.type = 3 ;
            $scope.Components.Confirm.context = $scope.autoLanguage( '至少选择一台主机，才可以进入下一步操作。' ) ;
            $scope.Components.Confirm.isShow = true ;
            $scope.Components.Confirm.noClose = true ;
         }
      } ;
   } ) ;
}());