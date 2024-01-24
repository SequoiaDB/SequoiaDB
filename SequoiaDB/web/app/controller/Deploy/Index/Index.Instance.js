//@ sourceURL=Deploy.Index.Instance.Ctrl.js
//"use strict" ;
(function(){
   var sacApp = window.SdbSacManagerModule ;
   //实例控制器
   sacApp.controllerProvider.register( 'Deploy.Index.Instance.Ctrl', function( $scope, $rootScope, $location, SdbFunction, SdbRest, SdbSignal, SdbSwap ){

      $scope.UninstallTips = '' ;

      //重启实例 弹窗
      $scope.RestartInstanceWindow = {
         'config': {
            'inputList': [
               {
                  "name": 'moduleName',
                  "webName": $scope.autoLanguage( '实例名' ),
                  "type": "select",
                  "value": null,
                  "valid": []
               }
            ]
         },
         'callback': {}
      }

      //添加实例下拉菜单
      $scope.AddInstanceDropdown = {
         'config': [
            { 'key': $scope.autoLanguage( '创建实例' ) },
            { 'key': $scope.autoLanguage( '添加已有的实例' ) }
         ],
         'OnClick': function ( index ) {
            if ( index == 0 )
            {
               $scope.ShowInstallInstance();
            }
            else
            {
               $scope.ShowAppendInstance();
            }
            $scope.AddInstanceDropdown['callback']['Close']();
         },
         'callback': {}
      }

      //打开 添加服务下拉菜单
      $scope.OpenAddInstanceDropdown = function ( event ) {
         if ( $scope.ClusterList.length > 0 ) {
            $scope.AddInstanceDropdown['callback']['Open']( event.currentTarget );
         }
      }

      //重启实例
      function restartInstance( clusterName, moduleName )
      {
         var data = { 'cmd': 'restart business', 'ClusterName': clusterName, 'BusinessName': moduleName } ;
         SdbRest.OmOperation( data, {
            'success': function( taskInfo ){
               $rootScope.tempData( 'Deploy', 'Model', 'Task' ) ;
               $rootScope.tempData( 'Deploy', 'Module', 'None' ) ;
               $rootScope.tempData( 'Deploy', 'ModuleTaskID', taskInfo[0]['TaskID'] ) ;
               $location.path( '/Deploy/Task/Restart' ).search( { 'r': new Date().getTime() } ) ;
            }, 
            'failed': function( errorInfo ){
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  restartInstance( clusterName, moduleName ) ;
                  return true ;
               } ) ;
            }
         } ) ;
      }

      //打开 重启实例 弹窗
      function showRestartInstance()
      {
         if ( $scope.InstanceNum == 0 ) {
            return;
         }
         $scope.RestartInstanceWindow['config']['inputList'][0]['value'] = null;
         $scope.RestartInstanceWindow['config']['inputList'][0]['valid'] = [];

         var clusterName = $scope.ClusterList[$scope.CurrentCluster]['ClusterName'];
         $.each( $scope.InstanceList, function ( index, moduleInfo ) {
            if ( clusterName == moduleInfo['ClusterName'] &&
                ( moduleInfo['BusinessType'] == 'sequoiasql-postgresql' ||
                  moduleInfo['BusinessType'] == 'sequoiasql-mysql' ||
                  moduleInfo['BusinessType'] == 'sequoiasql-mariadb' ) ) {
               if ( $scope.RestartInstanceWindow['config']['inputList'][0]['value'] == null ) {
                  $scope.RestartInstanceWindow['config']['inputList'][0]['value'] = index;
               }
               $scope.RestartInstanceWindow['config']['inputList'][0]['valid'].push( { 'key': moduleInfo['BusinessName'], 'value': index } )
            }
         } );

         $scope.RestartInstanceWindow['callback']['SetOkButton']( $scope.autoLanguage( '确定' ), function () {
            var isAllClear = $scope.RestartInstanceWindow['config'].check();
            if ( isAllClear ) {
               var formVal = $scope.RestartInstanceWindow['config'].getValue();
               $scope.Components.Confirm.type = 2;
               $scope.Components.Confirm.context = $scope.autoLanguage( '该操作将重启该服务所有节点，是否确定继续？' );
               $scope.Components.Confirm.isShow = true;
               $scope.Components.Confirm.okText = $scope.autoLanguage( '确定' );
               $scope.Components.Confirm.ok = function () {
                  restartInstance( $scope.ClusterList[$scope.CurrentCluster]['ClusterName'], $scope.InstanceList[formVal['moduleName']]['BusinessName'] );
                  $scope.Components.Confirm.isShow = false;
               }
            }
            return isAllClear;
         } );
         $scope.RestartInstanceWindow['callback']['SetTitle']( $scope.autoLanguage( '重启实例' ) );
         $scope.RestartInstanceWindow['callback']['SetIcon']( '' );
         $scope.RestartInstanceWindow['callback']['Open']();
      }

      //同步服务 弹窗
      $scope.SyncWindow = {
         'config': {
            'inputList': [
               {
                  "name": 'moduleName',
                  "webName": $scope.autoLanguage( '实例名' ),
                  "type": "select",
                  "value": null,
                  "valid": []
               }
            ]
         },
         'callback': {}
      }

      //打开 同步服务 弹窗
      function showSyncWindow()
      {
         if ( $scope.InstanceNum == 0 )
         {
            return;
         }

         clearArray( $scope.SyncWindow['config']['inputList'][0]['valid'] );

         var clusterName = $scope.ClusterList[$scope.CurrentCluster]['ClusterName'];
         $.each( $scope.InstanceList, function ( index, moduleInfo ) {
            if ( clusterName == moduleInfo['ClusterName'] )
            {
               if ( $scope.SyncWindow['config']['inputList'][0]['value'] == null )
               {
                  $scope.SyncWindow['config']['inputList'][0]['value'] = index;
               }
               $scope.SyncWindow['config']['inputList'][0]['valid'].push( { 'key': moduleInfo['BusinessName'], 'value': index } )
            }
         } );
         $scope.SyncWindow['callback']['SetOkButton']( $scope.autoLanguage( '确定' ), function () {
            var isAllClear = $scope.SyncWindow['config'].check();
            if ( isAllClear ) {
               var formVal = $scope.SyncWindow['config'].getValue();
               $rootScope.tempData( 'Deploy', 'ModuleName', $scope.InstanceList[formVal['moduleName']]['BusinessName'] );
               $rootScope.tempData( 'Deploy', 'ModuleType', $scope.InstanceList[formVal['moduleName']]['BusinessType'] );
               $rootScope.tempData( 'Deploy', 'ClusterName', $scope.ClusterList[$scope.CurrentCluster]['ClusterName'] );
               $rootScope.tempData( 'Deploy', 'InstallPath', $scope.ClusterList[$scope.CurrentCluster]['InstallPath'] );
               if ( $scope.InstanceList[formVal['moduleName']]['BusinessType'] == 'sequoiasql-mysql' )
               {
                  $location.path( '/Deploy/MySQL-Sync' ).search( { 'r': new Date().getTime() } );
               }
               else if ( $scope.InstanceList[formVal['moduleName']]['BusinessType'] == 'sequoiasql-mariadb' )
               {
                  $location.path( '/Deploy/MariaDB-Sync' ).search( { 'r': new Date().getTime() } );
               }
               else if ( $scope.InstanceList[formVal['moduleName']]['BusinessType'] == 'sequoiasql-postgresql' )
               {
                  $location.path( '/Deploy/PostgreSQL-Sync' ).search( { 'r': new Date().getTime() } );
               }
            }
            return isAllClear;
         } );
         $scope.SyncWindow['callback']['SetTitle']( $scope.autoLanguage( '同步配置' ) );
         $scope.SyncWindow['callback']['SetIcon']( '' );
         $scope.SyncWindow['callback']['Open']();
      }

      //发现实例 弹窗
      $scope.AppendInstance = {
         'config': {
            'inputList': [
               {
                  "name": 'instanceType',
                  "webName": $scope.autoLanguage( '实例类型' ),
                  "type": "select",
                  "value": 'mysql',
                  "valid": [
                     { 'key': 'MySQL', 'value': 'sequoiasql-mysql' },
                     { 'key': 'MariaDB', 'value': 'sequoiasql-mariadb' },
                     { 'key': 'PostgreSQL', 'value': 'sequoiasql-postgresql' }
                  ],
                  "onChange": function( name, key, value ){
                     if( value == 'sequoiasql-mysql' || value == 'sequoiasql-mariadb' )
                     {
                        $scope.AppendInstance['config']['inputList'][3]['required'] = true ;
                        $scope.AppendInstance['config']['inputList'][3]['valid'] = { 'min': 1, 'max': 32 } ;
                     }
                     else if( value == 'sequoiasql-postgresql' )
                     {
                        $scope.AppendInstance['config']['inputList'][3]['required'] = false ;
                        $scope.AppendInstance['config']['inputList'][3]['valid'] = {} ;
                     }
                  }
               },
               {
                  "name": 'HostName',
                  "webName": $scope.autoLanguage( '地址' ),
                  "type": "string",
                  "required": true,
                  "desc": $scope.autoLanguage( '数据库实例的主机名或者IP地址' ),
                  "value": "",
                  "valid": {
                     "min": 1
                  }
               },
               {
                  "name": 'ServiceName',
                  "webName": $scope.autoLanguage( '端口' ),
                  "type": "port",
                  "required": true,
                  "value": '',
                  "valid": {}
               },
               {
                  "name": 'User',
                  "webName": $scope.autoLanguage( '数据库用户名' ),
                  "type": "string",
                  "required": true,
                  "value": '',
                  "valid": {
                     "min": 1,
                     "max": 32
                  }
               },
               {
                  "name": 'Passwd',
                  "webName": $scope.autoLanguage( '数据库密码' ),
                  "type": "password",
                  "value": ""
               }
            ]
         },
         'callback': {}
      } ;

      //打开 发现实例 弹窗
      $scope.ShowAppendInstance = function()
      {
         if ( $scope.ClusterList.length == 0 )
         {
            return;
         }

         $scope.AppendInstance['config']['inputList'][0]['value'] = 'sequoiasql-mysql';
         $scope.AppendInstance['config']['inputList'][1]['value'] = '';
         $scope.AppendInstance['config']['inputList'][2]['value'] = '';
         $scope.AppendInstance['config']['inputList'][3]['value'] = '';
         $scope.AppendInstance['config']['inputList'][4]['value'] = '';

         $scope.AppendInstance['callback']['SetOkButton']( $scope.autoLanguage( '确定' ), function () {
            var isAllClear = $scope.AppendInstance['config'].check();
            if ( isAllClear ) {
               var formVal = $scope.AppendInstance['config'].getValue();
               var configure = {};
               configure['ClusterName'] = $scope.ClusterList[$scope.CurrentCluster]['ClusterName'];
               configure['BusinessType'] = formVal['instanceType'];
               configure['BusinessName'] = '' ;
               configure['BusinessInfo'] = formVal;
               discoverInstance( configure );
            }
            return isAllClear;
         } );
         $scope.AppendInstance['callback']['SetTitle']( $scope.autoLanguage( '添加已有的实例' ) );
         $scope.AppendInstance['callback']['SetIcon']( '' );
         $scope.AppendInstance['callback']['Open']();
      }

      //从发现前往添加主机
      function gotoAddHost( configure )
      {
         $rootScope.tempData( 'Deploy', 'ModuleHostName', configure['BusinessInfo']['HostName'] );
         $rootScope.tempData( 'Deploy', 'ModulePort', configure['BusinessInfo']['ServiceName'] );

         $rootScope.tempData( 'Deploy', 'Model', 'Deploy' );
         $rootScope.tempData( 'Deploy', 'Module', configure['BusinessType'] );
         $rootScope.tempData( 'Deploy', 'ClusterName', $scope.ClusterList[$scope.CurrentCluster]['ClusterName'] );
         $rootScope.tempData( 'Deploy', 'InstallPath', $scope.ClusterList[$scope.CurrentCluster]['InstallPath'] );
         $rootScope.tempData( 'Deploy', 'DiscoverConf', configure );
         $location.path( '/Deploy/ScanHost' ).search( { 'r': new Date().getTime() } );
      }

      //发现实例
      function discoverInstance( configure )
      {
         var data = { 'cmd': 'discover business', 'ConfigInfo': JSON.stringify( configure ) };
         SdbRest.OmOperation( data, {
            'success': function () {
               var hostName = configure['BusinessInfo']['HostName'] ;

               $.each( SdbSwap.hostList, function( index, info ){
                  if( hostName == info['IP'] )
                  {
                     hostName = info['HostName'] ;
                     return false ;
                  }
               } ) ;

               $rootScope.tempData( 'Deploy', 'ModuleName', '' );
               $rootScope.tempData( 'Deploy', 'ModuleHostName', hostName );
               $rootScope.tempData( 'Deploy', 'ModulePort', configure['BusinessInfo']['ServiceName'] );
               $rootScope.tempData( 'Deploy', 'ClusterName', configure['ClusterName'] );

               if ( configure['BusinessType'] == 'sequoiasql-mysql' )
               {
                  $location.path( '/Deploy/MYSQL-Discover' ).search( { 'r': new Date().getTime() } );
               }
               else if ( configure['BusinessType'] == 'sequoiasql-mariadb' )
               {
                  $location.path( '/Deploy/MARIADB-Discover' ).search( { 'r': new Date().getTime() } );
               }
               else if ( configure['BusinessType'] == 'sequoiasql-postgresql' )
               {
                  $location.path( '/Deploy/PostgreSQL-Discover' ).search( { 'r': new Date().getTime() } );
               }
            },
            'failed': function ( errorInfo ) {
               if ( isArray( errorInfo['hosts'] ) && errorInfo['hosts'].length > 0 )
               {
                  $scope.Components.Confirm.type = 3;
                  $scope.Components.Confirm.context = $scope.autoLanguage( '需要先在集群中添加该示例的主机。是否前往添加主机？' );
                  $scope.Components.Confirm.isShow = true;
                  $scope.Components.Confirm.okText = $scope.autoLanguage( '是' );
                  $scope.Components.Confirm.ok = function () {
                     $rootScope.tempData( 'Deploy', 'DiscoverHostList', errorInfo['hosts'] );
                     gotoAddHost( configure );
                  }
               }
               else {
                  _IndexPublic.createRetryModel( $scope, errorInfo, function () {
                     discoverInstance( configure );
                     return true;
                  } );
               }

            }
         } );
      }

      //实例操作
      $scope.EditInstanceDropdown = {
         'config': [],
         'OnClick': function ( index ) {
            if ( index == 0 ) {
               showRestartInstance();
            }
            else if ( index == 1 ) {
               showSyncWindow();
            }
            $scope.EditInstanceDropdown['callback']['Close']();
         },
         'callback': {}
      } ;

      //打开 存储集群操作
      $scope.OpenEditModuleDropdown = function ( event ) {
         if( $scope.ClusterList.length > 0 )
         {
            $scope.EditInstanceDropdown['config'] = [];
            $scope.EditInstanceDropdown['config'].push( { 'key': $scope.autoLanguage( '重启实例' ) } );
            $scope.EditInstanceDropdown['config'].push( { 'key': $scope.autoLanguage( '同步配置' ) } );
            $scope.EditInstanceDropdown['callback']['Open']( event.currentTarget );
         }
      }

      //添加服务 弹窗
      $scope.InstallModule = {
         'config': {},
         'callback': {}
      };

      //打开 创建实例 弹窗
      $scope.ShowInstallInstance = function()
      {
         if( $scope.ClusterList.length > 0 )
         {
            if( $scope.HostNum == 0 )
            {
               $scope.Components.Confirm.type = 3 ;
               $scope.Components.Confirm.context = $scope.autoLanguage( '集群还没有安装主机。' ) ;
               $scope.Components.Confirm.isShow = true ;
               $scope.Components.Confirm.okText = $scope.autoLanguage( '安装主机' ) ;
               $scope.Components.Confirm.ok = function(){
                  SdbSignal.commit( 'addHost' ) ;
               }
               return ;
            }
            $scope.InstallModule['config'] = {
               inputList: [
                  {
                     "name": 'moduleName',
                     "webName": $scope.autoLanguage( '实例名' ),
                     "type": "string",
                     "required": true,
                     "value": "",
                     "valid": {
                        "min": 1,
                        "max": 127,
                        'regex': '^[0-9a-zA-Z_-]+$'
                     }
                  },
                  {
                     "name": 'moduleType',
                     "webName": $scope.autoLanguage( '实例类型' ),
                     "type": "select",
                     "value": null,
                     "valid": [],
                     "onChange": function( name, key, value ){
                        $scope.InstallModule['config']['inputList'][0]['value'] = SdbSwap.generateModuleName( SdbSwap.moduleType[value]['BusinessDesc'] + 'Instance' ) ;
                     }
                  }
               ]
            } ;
            
            $.each( SdbSwap.moduleType, function( index, typeInfo ){
               if( typeInfo['BusinessType'] != 'sequoiadb' )
               {
                  if( isNull( $scope.InstallModule['config']['inputList'][1]['value'] ) )
                  {
                     $scope.InstallModule['config']['inputList'][1]['value'] = index ;

                     $scope.InstallModule['config']['inputList'][0]['value'] = SdbSwap.generateModuleName( typeInfo['BusinessDesc'] + 'Instance' ) ;
                  }
                  $scope.InstallModule['config']['inputList'][1]['valid'].push( { 'key': typeInfo['BusinessDesc'], 'value': index } ) ;
               }
            } ) ;

            $scope.InstallModule['callback']['SetOkButton']( $scope.autoLanguage( '确定' ), function(){
               var isAllClear = $scope.InstallModule['config'].check( function( formVal ){
                  var isFind = false ;
                  $.each( $scope.ModuleList, function( index, moduleInfo ){
                     if( formVal['moduleName'] == moduleInfo['BusinessName'] )
                     {
                        isFind = true ;
                        return false ;
                     }
                  } ) ;
                  if( isFind == false )
                  {
                     $.each( $rootScope.OmTaskList, function( index, taskInfo ){
                        if( taskInfo['Status'] != 4 && formVal['moduleName'] == taskInfo['Info']['BusinessName'] )
                        {
                           isFind = true ;
                           return false ;
                        }
                     } ) ;
                  }
                  if( isFind == true )
                  {
                     return [ { 'name': 'moduleName', 'error': $scope.autoLanguage( '实例名已经存在' ) } ]
                  }
                  else
                  {
                     return [] ;
                  }
               } ) ;
               if( isAllClear )
               {
                  var formVal = $scope.InstallModule['config'].getValue() ;
                  $rootScope.tempData( 'Deploy', 'Model', 'Module' ) ;
                  $rootScope.tempData( 'Deploy', 'Module', SdbSwap.moduleType[ formVal['moduleType'] ]['BusinessType'] ) ;
                  $rootScope.tempData( 'Deploy', 'ModuleName', formVal['moduleName'] ) ;
                  $rootScope.tempData( 'Deploy', 'ClusterName', $scope.ClusterList[ $scope.CurrentCluster ]['ClusterName'] ) ;

                  //当服务类型是postgresql时
                  if( SdbSwap.moduleType[ formVal['moduleType'] ]['BusinessType'] == 'sequoiasql-postgresql' )
                  {
                     var checkSqlHost = 0 ;
                     $.each( SdbSwap.hostList, function( index, hostInfo ){
                        if( hostInfo['ClusterName'] == $scope.ClusterList[ $scope.CurrentCluster ]['ClusterName'] )
                        {
                           $.each( hostInfo['Packages'], function( packIndex, packInfo ){
                              if( packInfo['Name'] == 'sequoiasql-postgresql' )
                              {
                                 ++checkSqlHost ;
                              }
                           } ) ;
                        }
                     } ) ;
                     {
                        var businessConf = {} ;
                        businessConf['ClusterName'] = $scope.ClusterList[ $scope.CurrentCluster ]['ClusterName'] ;
                        businessConf['BusinessName'] = formVal['moduleName'] ;
                        businessConf['BusinessType'] = SdbSwap.moduleType[ formVal['moduleType'] ]['BusinessType'] ;
                        businessConf['DeployMod'] = '' ;
                        businessConf['Property'] = [] ;
                        $rootScope.tempData( 'Deploy', 'ModuleConfig', businessConf ) ;
                        $location.path( '/Deploy/PostgreSQL-Mod' ).search( { 'r': new Date().getTime() } ) ;
                     }
                  }
                  else if( SdbSwap.moduleType[ formVal['moduleType'] ]['BusinessType'] == 'sequoiasql-mysql' )
                  {
                     var checkSqlHost = 0 ;
                     $.each( SdbSwap.hostList, function( index, hostInfo ){
                        if( hostInfo['ClusterName'] == $scope.ClusterList[ $scope.CurrentCluster ]['ClusterName'] )
                        {
                           $.each( hostInfo['Packages'], function( packIndex, packInfo ){
                              if( packInfo['Name'] == 'sequoiasql-mysql' )
                              {
                                 ++checkSqlHost ;
                              }
                           } ) ;
                        }
                     } ) ;
                     {
                        var businessConf = {} ;
                        businessConf['ClusterName'] = $scope.ClusterList[ $scope.CurrentCluster ]['ClusterName'] ;
                        businessConf['BusinessName'] = formVal['moduleName'] ;
                        businessConf['BusinessType'] = SdbSwap.moduleType[ formVal['moduleType'] ]['BusinessType'] ;
                        businessConf['DeployMod'] = '' ;
                        businessConf['Property'] = [] ;
                        $rootScope.tempData( 'Deploy', 'ModuleConfig', businessConf ) ;
                        $location.path( '/Deploy/MySQL-Mod' ).search( { 'r': new Date().getTime() } ) ;
                     }
                  }
                  else if( SdbSwap.moduleType[ formVal['moduleType'] ]['BusinessType'] == 'sequoiasql-mariadb' )
                  {
                     var checkSqlHost = 0 ;
                     $.each( SdbSwap.hostList, function( index, hostInfo ){
                        if( hostInfo['ClusterName'] == $scope.ClusterList[ $scope.CurrentCluster ]['ClusterName'] )
                        {
                           $.each( hostInfo['Packages'], function( packIndex, packInfo ){
                              if( packInfo['Name'] == 'sequoiasql-mariadb' )
                              {
                                 ++checkSqlHost ;
                              }
                           } ) ;
                        }
                     } ) ;
                     {
                        var businessConf = {} ;
                        businessConf['ClusterName'] = $scope.ClusterList[ $scope.CurrentCluster ]['ClusterName'] ;
                        businessConf['BusinessName'] = formVal['moduleName'] ;
                        businessConf['BusinessType'] = SdbSwap.moduleType[ formVal['moduleType'] ]['BusinessType'] ;
                        businessConf['DeployMod'] = '' ;
                        businessConf['Property'] = [] ;
                        $rootScope.tempData( 'Deploy', 'ModuleConfig', businessConf ) ;
                        $location.path( '/Deploy/MariaDB-Mod' ).search( { 'r': new Date().getTime() } ) ;
                     }
                  }
               }
               return isAllClear ;
            } ) ;
            $scope.InstallModule['callback']['SetTitle']( $scope.autoLanguage( '创建实例' ) ) ;
            $scope.InstallModule['callback']['SetIcon']( '' ) ;
            $scope.InstallModule['callback']['Open']() ;
         }
      }

      
      //删除实例下拉菜单
      $scope.DeleteInstanceDropdown = {
         'config': [
            { 'key': $scope.autoLanguage( '删除实例' ) },
            { 'key': $scope.autoLanguage( '移除实例' ) }
         ],
         'OnClick': function( index ){
            if( index == 0 )
            {
               showUninstallInstance() ;
            }
            else
            {
               showUnbindInstance() ;
            }
            $scope.DeleteInstanceDropdown['callback']['Close']() ;
         },
         'callback': {}
      }

      //打开 删除实例下拉菜单
      $scope.OpenDeleteInstanceDropdown = function( event ){
         if( $scope.ClusterList.length > 0 )
         {
            $scope.DeleteInstanceDropdown['callback']['Open']( event.currentTarget ) ;
         }
      }

      //解绑实例
      var unbindInstance = function( clusterName, businessName ){
         var data = {
            'cmd': 'unbind business', 'ClusterName': clusterName, 'BusinessName': businessName
         } ;
         SdbRest.OmOperation( data, {
            'success': function(){
               $scope.Components.Confirm.type = 4 ;
               $scope.Components.Confirm.context = sprintf( $scope.autoLanguage( '实例：? 移除成功。' ), businessName ) ;
               $scope.Components.Confirm.isShow = true ;
               $scope.Components.Confirm.noClose = true ;
               $scope.Components.Confirm.normalOK = true ;
               $scope.Components.Confirm.okText = $scope.autoLanguage( '确定' ) ;
               $scope.Components.Confirm.ok = function(){
                  $scope.Components.Confirm.isShow = false ;
                  $scope.Components.Confirm.noClose = false ;
                  $scope.Components.Confirm.normalOK = false ;
                  $location.path( '/Deploy/Index' ).search( { 'r': new Date().getTime() }  ) ;
               }
            },
            'failed': function( errorInfo ){
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  unbindInstance( clusterName, businessName ) ;
                  return true ;
               } ) ;
            }
         } ) ;
      }

      //解绑实例 弹窗
      $scope.UnbindInstanceWindow = {
         'config': {},
         'callback': {}
      }

      //打开 解绑实例 弹窗
      var showUnbindInstance = function(){
         if( $scope.ClusterList.length > 0 && $scope.ModuleNum != 0 )
         {
            var clusterName = $scope.ClusterList[ $scope.CurrentCluster ]['ClusterName'] ;
            $scope.UnbindInstanceWindow['config'] = {
               'inputList': [
                  {
                     "name": 'moduleIndex',
                     "webName": $scope.autoLanguage( '实例名' ),
                     "type": "select",
                     "value": null,
                     "valid": []
                  }
               ]
            }
            $.each( $scope.InstanceList, function( index, moduleInfo ){
               if( clusterName == moduleInfo['ClusterName'] )
               {
                  if( $scope.UnbindInstanceWindow['config']['inputList'][0]['value'] == null )
                  {
                     $scope.UnbindInstanceWindow['config']['inputList'][0]['value'] = index ;
                  }
                  $scope.UnbindInstanceWindow['config']['inputList'][0]['valid'].push( { 'key': moduleInfo['BusinessName'], 'value': index } )
               }
            } ) ;
            $scope.UnbindInstanceWindow['callback']['SetOkButton']( $scope.autoLanguage( '确定' ), function(){
               var isAllClear = $scope.UnbindInstanceWindow['config'].check() ;
               if( isAllClear )
               {
                  var formVal = $scope.UnbindInstanceWindow['config'].getValue() ;
                  var businessName = $scope.InstanceList[ formVal['moduleIndex'] ]['BusinessName'] ;
                  var clusterName = $scope.ClusterList[ $scope.CurrentCluster ]['ClusterName'] ;
                  unbindInstance( clusterName, businessName ) ;
               }
               return isAllClear ;
            } ) ;
            $scope.UnbindInstanceWindow['callback']['SetTitle']( $scope.autoLanguage( '移除实例' ) ) ;
            $scope.UnbindInstanceWindow['callback']['Open']() ;
         }
      }

      //删除实例
      function uninstallModule( index, isForce )
      {
         if( typeof( $scope.InstanceList[index]['AddtionType'] ) == 'undefined' || $scope.InstanceList[index]['AddtionType'] != 1 )
         {
            var data = { 'cmd': 'remove business', 'BusinessName': $scope.InstanceList[index]['BusinessName'], 'force': isForce } ;
            SdbRest.OmOperation( data, {
               'success': function( taskInfo ){
                  $rootScope.tempData( 'Deploy', 'Model', 'Task' ) ;
                  $rootScope.tempData( 'Deploy', 'Module', 'None' ) ;
                  $rootScope.tempData( 'Deploy', 'ModuleTaskID', taskInfo[0]['TaskID'] ) ;
                  $location.path( '/Deploy/Task/Module' ).search( { 'r': new Date().getTime() } ) ;
               },
               'failed': function( errorInfo ){
                  _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                     uninstallModule( index ) ;
                     return true ;
                  } ) ;
               }
            } ) ;
         }
         else
         {
            var data = { 'cmd': 'undiscover business', 'ClusterName': $scope.InstanceList[index]['ClusterName'], 'BusinessName': $scope.InstanceList[index]['BusinessName'] } ;
            SdbRest.OmOperation( data, {
               'success': function(){
                  $location.path( '/Deploy/Index' ).search( { 'r': new Date().getTime() }  ) ;
               },
               'failed': function( errorInfo ){
                  _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                     uninstallModule( index ) ;
                     return true ;
                  } ) ;
               }
            } ) ;
         }
      }

      //删除实例 弹窗
      $scope.RemoveInstanceWindow = {
         'config': {
            'inputList': [
               {
                  "name": 'moduleIndex',
                  "webName": $scope.autoLanguage( '实例名' ),
                  "type": "select",
                  "value": null,
                  "valid": []
               },
               {
                  "name": 'force',
                  "webName": $scope.autoLanguage( '删除所有数据' ),
                  "type": "select",
                  "value": false,
                  "valid": [
                     { 'key': $scope.autoLanguage( '否' ),  'value': false },
                     { 'key': $scope.autoLanguage( '是' ),  'value': true  }
                  ],
                  "onChange": function( name, key, value ){
                     if( value == true )
                     {
                        $scope.UninstallTips = $scope.autoLanguage( '提示：选择删除所有数据将无法恢复！' ) ;
                     }
                     else
                     {
                        $scope.UninstallTips = '' ;
                     }
                  }
               }
            ]
         },
         'callback': {}
      } ;

      //打开 删除实例 弹窗
      var showUninstallInstance = function(){
         if( $scope.ClusterList.length == 0 )
         {
            return ;
         }
         if( $scope.ModuleNum == 0 )
         {
            $scope.Components.Confirm.type = 3 ;
            $scope.Components.Confirm.context = $scope.autoLanguage( '已经没有实例了。' ) ;
            $scope.Components.Confirm.isShow = true ;
            $scope.Components.Confirm.noClose = true ;
            return ;
         }
         $scope.UninstallTips = '' ;
         var clusterName = $scope.ClusterList[ $scope.CurrentCluster ]['ClusterName'] ;

         clearArray( $scope.RemoveInstanceWindow['config']['inputList'][0]['valid'] ) ;
         $.each( $scope.InstanceList, function( index, moduleInfo ){
            if( clusterName == moduleInfo['ClusterName'] )
            {
               if( isNull( $scope.RemoveInstanceWindow['config']['inputList'][0]['value'] ) )
               {
                  $scope.RemoveInstanceWindow['config']['inputList'][0]['value'] = index ;
               }
               $scope.RemoveInstanceWindow['config']['inputList'][0]['valid'].push( { 'key': moduleInfo['BusinessName'], 'value': index } )
            }
         } ) ;
         $scope.RemoveInstanceWindow['callback']['SetOkButton']( $scope.autoLanguage( '确定' ), function(){
            var isAllClear = $scope.RemoveInstanceWindow['config'].check() ;
            if( isAllClear )
            {
               var formVal = $scope.RemoveInstanceWindow['config'].getValue() ;
               if( formVal['force'] == true )
               {
                  $scope.Components.Confirm.type = 2 ;
                  $scope.Components.Confirm.context = $scope.autoLanguage( '该操作将删除该实例所有数据，是否确定继续？' ) ;
                  $scope.Components.Confirm.isShow = true ;
                  $scope.Components.Confirm.okText = $scope.autoLanguage( '确定' ) ;
                  $scope.Components.Confirm.ok = function(){
                     uninstallModule( formVal['moduleIndex'], formVal['force'] ) ;
                     $scope.Components.Confirm.isShow = false ;
                  }
               }
               else
               {
                  uninstallModule( formVal['moduleIndex'], formVal['force'] ) ;
               }
            }
            return isAllClear ;
         } ) ;
         $scope.RemoveInstanceWindow['callback']['SetTitle']( $scope.autoLanguage( '删除实例' ) ) ;
         $scope.RemoveInstanceWindow['callback']['Open']() ;
      }


      //设置鉴权 弹窗
      $scope.SetAuthority = {
         'config': {},
         'callback': {}
      } ;

      //表单
      var authorityform = {
         'inputList': [
            {
               "name": "BusinessName",
               "webName": $scope.autoLanguage( '实例名' ),
               "type": "string",
               "disabled": true,
               "value": ''
            },
            {
               "name": "User",
               "webName": $scope.autoLanguage( '用户名' ),
               "type": "string",
               "required": true,
               "value": "",
               "valid": {
                  "min": 1,
                  "max": 127,
                  "regex": '^[0-9a-zA-Z]+$'
               }
            },
            {
               "name": "Password",
               "webName": $scope.autoLanguage( '密码' ),
               "type": "password",
               "value": ""
            }
         ]
      } ;

      //保存当前选中的服务名
      var chooseBusinessName = '' ;
      $scope.SaveBsName = function( businessName )
      {
         chooseBusinessName = businessName ;
      }

      //设置鉴权
      var setAuthority = function( data, businessName ){
         SdbRest.OmOperation( data, {
            'success': function(){
               SdbSwap.queryAuth( businessName ) ;
            },
            'failed': function( errorInfo ){
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  setAuthority( data, businessName ) ;
                  return true ;
               } ) ;
            }
         }, {
            'showLoading': true
         } ) ;
      }

      //打开 设置鉴权 弹窗
      $scope.ShowSetAuthority = function( businessName, index ){
         if( typeof( businessName ) == 'undefined' )
         {
            businessName = chooseBusinessName ;
            index = chooseBusinessIndex ;
         }
         
         var user = '' ;
         if( typeof( $scope.InstanceList[index]['authority'][0]['User'] ) != 'undefined' )
         {
            user = $scope.InstanceList[index]['authority'][0]['User'] ;
         }

         authorityform['inputList'][1]['value'] = '' ;
         authorityform['inputList'][2]['value'] = '' ;

         //关闭鉴权下拉菜单
         $scope.AuthorityDropdown['callback']['Close']() ;

         var form = authorityform ;
         form['inputList'][0]['value'] = businessName ;
         form['inputList'][1]['value'] = user ;
         $scope.SetAuthority['config'] = form ;
         $scope.SetAuthority['callback']['SetOkButton']( $scope.autoLanguage( '确定' ), function(){
            var isAllClear = form.check() ;
               if( isAllClear )
               {
                  var formVal = form.getValue() ;
                  var data = {
                     'cmd': 'set business authority',
                     'BusinessName': businessName,
                     'User': formVal['User'],
                     'Passwd': formVal['Password']
                  } ;
                  setAuthority( data, businessName ) ;
                  $scope.SetAuthority['callback']['Close']() ;
               }
         } ) ;
         $scope.SetAuthority['callback']['SetTitle']( $scope.autoLanguage( '设置鉴权' ) ) ;
         $scope.SetAuthority['callback']['SetIcon']( '' ) ;
         $scope.SetAuthority['callback']['Open']() ;

      }

      //删除鉴权
      $scope.DropAuthorityModel = function( businessName, index ){
         if( typeof( businessName ) == 'undefined' )
         {
            businessName = chooseBusinessName ;
            index = chooseBusinessIndex ;
         }
         //关闭下拉菜单
         $scope.AuthorityDropdown['callback']['Close']() ;
         $scope.Components.Confirm.isShow = true ;
         $scope.Components.Confirm.type = 1 ;
         $scope.Components.Confirm.okText = $scope.autoLanguage( '确定' ) ;
         $scope.Components.Confirm.closeText = $scope.autoLanguage( '取消' ) ;
         $scope.Components.Confirm.title = $scope.autoLanguage( '要删除该实例的鉴权吗？' ) ;
         $scope.Components.Confirm.context = $scope.autoLanguage( '实例名' ) + ': ' + businessName ;
         $scope.Components.Confirm.ok = function(){
            var data = {
               'cmd': 'remove business authority',
               'BusinessName': businessName
            }
            SdbRest.OmOperation( data, {
               'success': function(){
                  SdbSwap.queryAuth( businessName ) ;
               },
               'failed': function( errorInfo, retryRun ){
                  _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                     retryRun() ;
                     return true ;
                  } ) ;
               }
            }, {
               'showLoading': true
            } ) ;
            return true ;
         }
      }

      //鉴权下拉菜单
      $scope.AuthorityDropdown = {
         'config': [
            { 'field': $scope.autoLanguage( '修改鉴权' ), 'value': "edit" },
            { 'field': $scope.autoLanguage( '删除鉴权' ), 'value': "delete" }
         ],
         'callback': {}
      } ;

      //打开鉴权下拉菜单
      $scope.OpenShowAuthorityDropdown = function( event, businessName, index ){
         chooseBusinessName = businessName ;
         chooseBusinessIndex = index ;
         $scope.AuthorityDropdown['callback']['Open']( event.currentTarget ) ;
      }

      $scope.SetDefaultDbWindow = {
         'config': {},
         'callback': {}
      } ;

      $scope.ShowSetDefaultDb = function( businessName, index ){
         var dbName = '' ;
         if( typeof( $scope.ModuleList[index]['authority'][0]['DbName'] ) != 'undefined' )
         {
            dbName = $scope.ModuleList[index]['authority'][0]['DbName'] ;
         }
         var form = {
            'inputList': [
               {
                  "name": "DbName",
                  "webName": $scope.autoLanguage( '默认数据库' ),
                  "type": "string",
                  "required": true,
                  "value": dbName,
                  "valid": {
                     "min": 1
                  } 
               }
            ]
         } ;
         $scope.SetDefaultDbWindow['config'] = form ;
         $scope.SetDefaultDbWindow['callback']['SetOkButton']( $scope.autoLanguage( '确定' ), function(){
            var isAllClear = $scope.SetDefaultDbWindow['config'].check() ;
            if( isAllClear )
            {
               var formVal = form.getValue() ;
               var data = {
                  'cmd': 'set business authority',
                  'BusinessName': businessName,
                  'DbName' : formVal['DbName']
               } ;
               setAuthority( data, businessName ) ;
               $scope.SetDefaultDbWindow['callback']['Close']() ;
            }
         } ) ;
         $scope.SetDefaultDbWindow['callback']['SetTitle']( $scope.autoLanguage( '设置默认数据库' ) ) ;
         $scope.SetDefaultDbWindow['callback']['SetIcon']( '' ) ;
         $scope.SetDefaultDbWindow['callback']['Open']() ;
      }

      //判断类型
      $scope.Typeof = function( value ){
         return typeof( value ) ;
      }

      $scope.GotoExpansion = function(){
         $location.path( '/Deploy/SDB-ExtendConf' ).search( { 'r': new Date().getTime() } ) ;
      }

      //创建关联
      function createRelation( name, from, to, options )
      {
         var data = {
            'cmd'    : 'create relationship',
            'Name'   : name,
            'From'   : from,
            'To'     : to,
            'Options': JSON.stringify( options )
         } ;

         SdbRest.OmOperation( data, {
            'success': function( result ){
               $scope.Components.Confirm.type = 4 ;
               $scope.Components.Confirm.context = sprintf( $scope.autoLanguage( '创建成功：?' ), name ) ;
               $scope.Components.Confirm.isShow = true ;
               $scope.Components.Confirm.noClose = true ;
               $scope.Components.Confirm.normalOK = true ;
               $scope.Components.Confirm.okText = $scope.autoLanguage( '确定' ) ;
               $scope.Components.Confirm.ok = function(){
                  $scope.Components.Confirm.isShow = false ;
                  $scope.Components.Confirm.noClose = false ;
                  $scope.Components.Confirm.normalOK = false ;
                  SdbSwap.getRelationship() ;
               }
            },
            'failed': function( errorInfo ){
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  createRelation( name, from, to, options ) ;
                  return true ;
               } ) ;
            }
         }, {
            'showLoading': true
         } ) ;
      }

      //解除关联
      function removeRelation( name )
      {
         var data = {
            'cmd' : 'remove relationship',
            'Name'  : name
         } ;

         SdbRest.OmOperation( data, {
            'success': function(){
               $scope.Components.Confirm.type = 4 ;
               $scope.Components.Confirm.context = sprintf( $scope.autoLanguage( '移除成功：?' ), name ) ;
               $scope.Components.Confirm.isShow = true ;
               $scope.Components.Confirm.noClose = true ;
               $scope.Components.Confirm.normalOK = true ;
               $scope.Components.Confirm.okText = $scope.autoLanguage( '确定' ) ;
               $scope.Components.Confirm.ok = function(){
                  $scope.Components.Confirm.isShow = false ;
                  $scope.Components.Confirm.noClose = false ;
                  $scope.Components.Confirm.normalOK = false ;
                  SdbSwap.getRelationship() ;
               }
            },
            'failed': function( errorInfo ){
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  removeRelation( name ) ;
                  return true ;
               } ) ;
            }
         }, {
            'showLoading': true
         } ) ;
      }

      //关联操作 下拉菜单
      $scope.RelationDropdown = {
         'config': [],
         'OnClick': function( index ){
            if( index == 0 )
            {
               $scope.OpenCreateRelation() ;
            }
            else if( index == 1 )                
            {
               $scope.OpenRemoveRelation() ;
            }
            $scope.RelationDropdown['callback']['Close']() ;
         },
         'callback': {}
      } ;

      //是否禁用创建关联按钮
      var createRelationDisabled = false ;

      //打开 关联操作 下拉菜单
      $scope.OpenRelationDropdown = function( event ){
         var disabled = false ;
         var pgsqlModule = 0 ;
         var mysqlModule = 0 ;
         var mariadbModule = 0 ;
         var sdbModule = 0 ;
         $.each( $scope.ModuleList, function( index, moduleInfo ){
            if( moduleInfo['BusinessType'] == 'sequoiadb' )
            {
               ++sdbModule ;
            }
            else if( moduleInfo['BusinessType'] == 'sequoiasql-postgresql' )
            {
               ++pgsqlModule ;
            }
            else if( moduleInfo['BusinessType'] == 'sequoiasql-mysql' )
            {
               ++mysqlModule ;
            }
            else if( moduleInfo['BusinessType'] == 'sequoiasql-mariadb' )
            {
               ++mariadbModule ;
            }
         } ) ;
         if( ( pgsqlModule == 0 && mysqlModule == 0 && mariadbModule == 0 ) || sdbModule == 0 )
         {
            createRelationDisabled = true ;
         }
         else
         {
            createRelationDisabled = false ;
         }
         $scope.RelationDropdown['config'] = [] ;
         $scope.RelationDropdown['config'].push( { 'key': $scope.autoLanguage( '添加实例存储' ), 'disabled': createRelationDisabled } ) ;

         if( SdbSwap.relationshipList.length == 0 )
         {
            disabled = true ;
         }
         $scope.RelationDropdown['config'].push( { 'key': $scope.autoLanguage( '移除实例存储' ), 'disabled': disabled } ) ;
         $scope.RelationDropdown['callback']['Open']( event.currentTarget ) ;
      }

      //创建关联 弹窗
      $scope.CreateRelationWindow = {
         'config': {
            'ShowType': 1,
            'inputErrNum1': 0,
            'inputErrNum2': 0,
            'normal': {
               'inputList': [
                  {
                     "name": 'Name',
                     "webName": $scope.autoLanguage( '关联名' ),
                     "type": "string",
                     "required": true,
                     "value": '',
                     "valid": {
                        "min": 1,
                        "max": 63,
                        "regex": "^[a-zA-Z]+[0-9a-zA-Z_-]*$"
                     }
                  },
                  {
                     "name": "Type",
                     "webName": $scope.autoLanguage( '关联类型' ),
                     "required": true,
                     "type": "select",
                     "value": 'sdb-pg',
                     "valid": []
                  },
                  {
                     "name": "From",
                     "webName": $scope.autoLanguage( '实例名' ),
                     "required": true,
                     "type": "select",
                     "value": '',
                     "valid": []
                  },
                  {
                     "name": 'DbName',
                     "webName": $scope.autoLanguage( '实例的数据库' ),
                     "type": "select",
                     "required": true,
                     "value": "",
                     "valid": []
                  },
                  {
                     "name": "To",
                     "webName": $scope.autoLanguage( '分布式存储' ),
                     "required": true,
                     "type": "select",
                     "value": '',
                     "valid":[]
                  }
               ]
            },
            'advance': {
               'keyWidth': '220px',
               'inputList': [
                  {
                     "name": "preferedinstance",
                     "webName": "preferedinstance",
                     "type": "select",
                     "required": true,
                     "value": 'a',
                     "valid":[
                        { 'key': 's', 'value': 's' },
                        { 'key': 'm', 'value': 'm' },
                        { 'key': 'a', 'value': 'a' },
                        { 'key': '1', 'value': '1' },
                        { 'key': '2', 'value': '2' },
                        { 'key': '3', 'value': '3' },
                        { 'key': '4', 'value': '4' },
                        { 'key': '5', 'value': '5' },
                        { 'key': '6', 'value': '6' },
                        { 'key': '7', 'value': '7' }
                     ]
                  },
                  {
                     "name": "transaction",
                     "webName": "transaction",
                     "type": "select",
                     "required": true,
                     "value": "OFF",
                     "valid":[
                        { 'key': 'ON', 'value': 'ON' },
                        { 'key': 'OFF', 'value': 'OFF' }
                     ]
                  },
                  {
                     "name": "sequoiadb_auto_partition",
                     "webName": "sequoiadb_auto_partition",
                     "desc": $scope.autoLanguage( "自动分区" ),
                     "type": "select",
                     "value": "ON",
                     "valid":[
                        { 'key': 'ON', 'value': 'ON' },
                        { 'key': 'OFF', 'value': 'OFF' }
                     ]
                  },
                  {
                     "name": "sequoiadb_replica_size",
                     "webName": "sequoiadb_replica_size",
                     "desc": $scope.autoLanguage( "一致性写副本数" ),
                     "type": "int",
                     "value": 1,
                     "valid": {
                        'min': -1,
                        'max': 7
                     }
                  },
                  {
                     "name": "sequoiadb_use_bulk_insert",
                     "webName": "sequoiadb_use_bulk_insert",
                     "desc": $scope.autoLanguage( "批量插入" ),
                     "type": "select",
                     "value": "ON",
                     "valid":[
                        { 'key': 'ON', 'value': 'ON' },
                        { 'key': 'OFF', 'value': 'OFF' }
                     ]
                  },
                  {
                     "name": "sequoiadb_bulk_insert_size",
                     "webName": "sequoiadb_bulk_insert_size",
                     "desc": $scope.autoLanguage( "批量插入的最大记录数" ),
                     "type": "int",
                     "value": 2000,
                     "valid": {
                        'min': 1,
                        'max': 100000
                     }
                  },
                  {
                     "name": "sequoiadb_selector_pushdown_threshold",
                     "webName": "sequoiadb_selector_pushdown_threshold",
                     "desc": $scope.autoLanguage( "查询字段下压触发阈值" ),
                     "type": "int",
                     "value": 30,
                     "valid": {
                        'min': 0,
                        'max': 100
                     }
                  },
                  {
                     "name": "sequoiadb_optimizer_options",
                     "webName": "sequoiadb_optimizer_options",
                     "desc": $scope.autoLanguage( "SequoiaDB 优化选项开关，以决定是否优化计数、更新、删除操作" ),
                     "type": "string",
                     "value": "direct_count,direct_delete,direct_update"
                  },
                  {
                     "name": "sequoiadb_rollback_on_timeout",
                     "webName": "sequoiadb_rollback_on_timeout",
                     "desc": $scope.autoLanguage( "配置记录锁超时是否中断并回滚整个事务" ),
                     "type": "select",
                     "value": "OFF",
                     "valid":[
                        { 'key': 'ON', 'value': 'ON' },
                        { 'key': 'OFF', 'value': 'OFF' }
                     ]
                  },
                  {
                     "name": "sequoiadb_alter_table_overhead_threshold",
                     "webName": "sequoiadb_alter_table_overhead_threshold",
                     "desc": $scope.autoLanguage( "更改表开销阈值" ),
                     "type": "int",
                     "value": 10000000,
                     "valid": {
                        'min': 0
                     }
                  },
                  {
                     "name": "sequoiadb_execute_only_in_mysql",
                     "webName": "sequoiadb_execute_only_in_mysql",
                     "desc": $scope.autoLanguage( "DDL 命令只在 MySQL 执行，不下压到 SequoiaDB 执行" ),
                     "type": "select",
                     "value": "OFF",
                     "valid":[
                        { 'key': 'ON', 'value': 'ON' },
                        { 'key': 'OFF', 'value': 'OFF' }
                     ]
                  },
                  {
                     "name": "sequoiadb_debug_log",
                     "webName": "sequoiadb_debug_log",
                     "desc": $scope.autoLanguage( "打印debug日志" ),
                     "type": "select",
                     "value": "OFF",
                     "valid":[
                        { 'key': 'ON', 'value': 'ON' },
                        { 'key': 'OFF', 'value': 'OFF' }
                     ]
                  },
                  {
                     "name": "sequoiadb_error_level",
                     "webName": "sequoiadb_error_level",
                     "desc": $scope.autoLanguage( "错误级别控制" ),
                     "type": "select",
                     "value": "error",
                     "valid":[
                        { 'key': 'warning', 'value': 'warning' },
                        { 'key': 'error', 'value': 'error' }
                     ]
                  },
                  {
                     "name": "address",
                     "webName": "address",
                     "desc": $scope.autoLanguage( '存储集群的节点' ),
                     "type": "multiple",
                     "required": true,
                     "value": [],
                     "valid": {}
                  }
               ]
            }
         },
         'callback': {}
      } ;

      //获取postgresql数据库列表
      function getPostgresqlDB( clusterName, serviceName, func )
      {
         var sql = 'SELECT datname FROM pg_database WHERE datname NOT LIKE \'template0\' AND datname NOT LIKE \'template1\'' ;
         var data = { 'Sql': sql, 'IsAll': 'true' } ;

         SdbRest.DataOperationV21( clusterName, serviceName, '/sql', data, {
            'success': func,
            'failed': function( errorInfo ){
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  getPostgresqlDB( clusterName, serviceName, func ) ;
                  return true ;
               } ) ;
            }
         } ) ;
      }

      //设置关联窗口的节点地址
      function setCreateRelationAddress( serviceName, advanceInput )
      {
         var serviceInfo = getArrayItem( $scope.ModuleList, 'BusinessName', serviceName ) ;

         if( serviceInfo && isObject( serviceInfo['BusinessInfo'] ) && isArray( serviceInfo['BusinessInfo']['NodeList'] ) )
         {
            var nodeInputList = [] ;

            $.each( serviceInfo['BusinessInfo']['NodeList'], function( index, nodeInfo ){
               if( nodeInfo['Role'] == 'coord' || nodeInfo['Role'] == 'standalone' )
               {
                  nodeInputList.push( {
                     'key': nodeInfo['HostName'] + ':' + nodeInfo['ServiceName'],
                     'value': nodeInfo['HostName'] + ':' + nodeInfo['ServiceName'],
                     'checked': true
                  } ) ;
               }
            } ) ;

            setArrayItemValue( advanceInput, 'name', 'address', { 'valid': { 'min': 0, 'list': nodeInputList } } ) ;
         }
      }

      //打开 创建关联 弹窗
      $scope.OpenCreateRelation = function() {
         if( createRelationDisabled == true )
         {
            return ;
         }

         $scope.CreateRelationWindow['config'].ShowType = 1 ;
         $scope.CreateRelationWindow['config'].inputErrNum1 = 0 ;
         $scope.CreateRelationWindow['config'].inputErrNum2 = 0 ;

         if ( isFunction( $scope.CreateRelationWindow['config']['normal'].ResetDefault ) &&
              isFunction( $scope.CreateRelationWindow['config']['advance'].ResetDefault ) )
         {
            $scope.CreateRelationWindow['config']['normal'].ResetDefault() ;
            $scope.CreateRelationWindow['config']['advance'].ResetDefault() ;
         }

         var pgsqlBusList = [] ;
         var mysqlBusList = [] ;
         var mariadbBusList = [] ;

         var typeValid = [] ;
         var fromValid = [] ;
         var toValid = [] ;
         var dbValid = [] ;

         var normalInput  = $scope.CreateRelationWindow['config']['normal']['inputList'] ;
         var advanceInput = $scope.CreateRelationWindow['config']['advance']['inputList'] ;

         //修改关联类型的事件
         function switchCreateRelationType( name, key, value )
         {
            var to = $scope.CreateRelationWindow['config']['normal'].getValueByOne( 'To' ) ;

            if( value == 'pg-sdb' )
            {
               //切换配置项
               $scope.CreateRelationWindow['config']['normal'] .EnableItem( 'DbName' ) ;
               $scope.CreateRelationWindow['config']['advance'].EnableItem( 'preferedinstance' ) ;
               $scope.CreateRelationWindow['config']['advance'].EnableItem( 'transaction' ) ;
               
               $scope.CreateRelationWindow['config']['advance'].DisableItem( 'sequoiadb_auto_partition' ) ;
               $scope.CreateRelationWindow['config']['advance'].DisableItem( 'sequoiadb_use_bulk_insert' ) ;
               $scope.CreateRelationWindow['config']['advance'].DisableItem( 'sequoiadb_bulk_insert_size' ) ;
               $scope.CreateRelationWindow['config']['advance'].DisableItem( 'sequoiadb_replica_size' ) ;
               $scope.CreateRelationWindow['config']['advance'].DisableItem( 'sequoiadb_selector_pushdown_threshold' ) ;
               $scope.CreateRelationWindow['config']['advance'].DisableItem( 'sequoiadb_alter_table_overhead_threshold' ) ;
               $scope.CreateRelationWindow['config']['advance'].DisableItem( 'sequoiadb_debug_log' ) ;
               $scope.CreateRelationWindow['config']['advance'].DisableItem( 'sequoiadb_error_level' ) ;
               $scope.CreateRelationWindow['config']['advance'].DisableItem( 'sequoiadb_execute_only_in_mysql' ) ;
               $scope.CreateRelationWindow['config']['advance'].DisableItem( 'sequoiadb_optimizer_options' ) ;
               $scope.CreateRelationWindow['config']['advance'].DisableItem( 'sequoiadb_rollback_on_timeout' ) ;

               fromValid = pgsqlBusList ;
               
               getPostgresqlDB( $scope.ClusterList[$scope.CurrentCluster]['ClusterName'], fromValid[0]['value'], function( dbList ){

                  clearArray( dbValid ) ;

                  $.each( dbList, function( index, dbInfo ){
                     dbValid.push( { 'key': dbInfo['datname'], 'value': dbInfo['datname'] } ) ;
                  } ) ;
                  //pg不支持‘-’，如果名字带有‘-’修改为‘_’
                  var from = fromValid[0]['value'].replace( /\-/g, '_' ) ;
                  to = to.replace( /\-/g, '_' ) ;
                  var relationName = sprintf( '?_?_?', from, to, dbValid[0]['value'] ) ;

                  setArrayItemValue( normalInput, 'name', 'Name',   { 'value': relationName } ) ;
                  setArrayItemValue( normalInput, 'name', 'DbName', { 'value': dbValid[0]['value'], 'valid': dbValid } ) ;
               } ) ;
            }  
            else if( value == 'mysql-sdb' )
            {
               //切换配置项
               $scope.CreateRelationWindow['config']['advance'].EnableItem( 'sequoiadb_auto_partition' ) ;
               $scope.CreateRelationWindow['config']['advance'].EnableItem( 'sequoiadb_use_bulk_insert' ) ;
               $scope.CreateRelationWindow['config']['advance'].EnableItem( 'sequoiadb_bulk_insert_size' ) ;
               $scope.CreateRelationWindow['config']['advance'].EnableItem( 'sequoiadb_replica_size' ) ;
               $scope.CreateRelationWindow['config']['advance'].EnableItem( 'sequoiadb_selector_pushdown_threshold' ) ;
               $scope.CreateRelationWindow['config']['advance'].EnableItem( 'sequoiadb_alter_table_overhead_threshold' ) ;
               $scope.CreateRelationWindow['config']['advance'].EnableItem( 'sequoiadb_debug_log' ) ;
               $scope.CreateRelationWindow['config']['advance'].EnableItem( 'sequoiadb_error_level' ) ;
               $scope.CreateRelationWindow['config']['advance'].EnableItem( 'sequoiadb_execute_only_in_mysql' ) ;
               $scope.CreateRelationWindow['config']['advance'].EnableItem( 'sequoiadb_optimizer_options' ) ;
               $scope.CreateRelationWindow['config']['advance'].EnableItem( 'sequoiadb_rollback_on_timeout' ) ;

               $scope.CreateRelationWindow['config']['normal'] .DisableItem( 'DbName' ) ;
               $scope.CreateRelationWindow['config']['advance'].DisableItem( 'preferedinstance' ) ;
               $scope.CreateRelationWindow['config']['advance'].DisableItem( 'transaction' ) ;

               fromValid = mysqlBusList ;
               
               var relationName = sprintf( '?_?', fromValid[0]['value'], to ) ;

               setArrayItemValue( normalInput, 'name', 'Name', { 'value': relationName } ) ;
            }
            else if( value == 'mariadb-sdb' )
            {
               //切换配置项
               $scope.CreateRelationWindow['config']['advance'].EnableItem( 'sequoiadb_auto_partition' ) ;
               $scope.CreateRelationWindow['config']['advance'].EnableItem( 'sequoiadb_use_bulk_insert' ) ;
               $scope.CreateRelationWindow['config']['advance'].EnableItem( 'sequoiadb_bulk_insert_size' ) ;
               $scope.CreateRelationWindow['config']['advance'].EnableItem( 'sequoiadb_replica_size' ) ;
               $scope.CreateRelationWindow['config']['advance'].EnableItem( 'sequoiadb_selector_pushdown_threshold' ) ;
               $scope.CreateRelationWindow['config']['advance'].EnableItem( 'sequoiadb_alter_table_overhead_threshold' ) ;
               $scope.CreateRelationWindow['config']['advance'].EnableItem( 'sequoiadb_debug_log' ) ;
               $scope.CreateRelationWindow['config']['advance'].EnableItem( 'sequoiadb_error_level' ) ;
               $scope.CreateRelationWindow['config']['advance'].EnableItem( 'sequoiadb_execute_only_in_mysql' ) ;
               $scope.CreateRelationWindow['config']['advance'].EnableItem( 'sequoiadb_optimizer_options' ) ;
               $scope.CreateRelationWindow['config']['advance'].EnableItem( 'sequoiadb_rollback_on_timeout' ) ;

               $scope.CreateRelationWindow['config']['normal'] .DisableItem( 'DbName' ) ;
               $scope.CreateRelationWindow['config']['advance'].DisableItem( 'preferedinstance' ) ;
               $scope.CreateRelationWindow['config']['advance'].DisableItem( 'transaction' ) ;

               fromValid = mariadbBusList ;
               
               var relationName = sprintf( '?_?', fromValid[0]['value'], to ) ;

               setArrayItemValue( normalInput, 'name', 'Name', { 'value': relationName } ) ;
            }

            setArrayItemValue( normalInput, 'name', 'From', { 'value': fromValid[0]['value'], 'valid': fromValid } ) ;
         }

         //修改关联服务名的事件
         function switchCreateRelationFrom( name, key, value )
         {
            var current = $scope.CreateRelationWindow['config']['normal'].getValue() ;
            var type = current['Type'] ;
            var to = current['To'] ;

            if( type == 'pg-sdb' )
            {
               getPostgresqlDB( $scope.ClusterList[$scope.CurrentCluster]['ClusterName'], value, function( dbList ){

                  clearArray( dbValid ) ;

                  $.each( dbList, function( index, dbInfo ){
                     dbValid.push( { 'key': dbInfo['datname'], 'value': dbInfo['datname'] } ) ;
                  } ) ;
                  
                  //pg不支持‘-’，如果名字带有‘-’修改为‘_’
                  value = value.replace( /\-/g, '_' ) ;
                  to    = to.replace( /\-/g, '_' ) ;
                  var relationName = sprintf( '?_?_?', value, to, dbValid[0]['value'] ) ;

                  setArrayItemValue( normalInput, 'name', 'Name',   { 'value': relationName } ) ;
                  setArrayItemValue( normalInput, 'name', 'DbName', { 'value': dbValid[0]['value'], 'valid': dbValid } ) ;
               } ) ;
            }  
            else if( type == 'mysql-sdb' )
            {
               var relationName = sprintf( '?_?', value, to ) ;
               
               setArrayItemValue( normalInput, 'name', 'Name', { 'value': relationName } ) ;
            }
            else if( type == 'mariadb-sdb' )
            {
               var relationName = sprintf( '?_?', value, to ) ;
               
               setArrayItemValue( normalInput, 'name', 'Name', { 'value': relationName } ) ;
            }
         }

         //修改关联服务名的数据库的事件
         function switchCreateRelationDbName( name, key, value )
         {
            var relationName = '' ;
            var current = $scope.CreateRelationWindow['config']['normal'].getValue() ;
            var type = current['Type'] ;
            var from = current['From'] ;
            var to = current['To'] ;

            if( type == 'pg-sdb' )
            {
               //pg不支持‘-’，如果名字带有‘-’修改为‘_’
               from = from.replace( /\-/g, '_' ) ;
               to = to.replace( /\-/g, '_' ) ;
               relationName = sprintf( '?_?_?', from, to, value ) ;
            }  
            else if( type == 'mysql-sdb' )
            {
               relationName = sprintf( '?_?', from, to ) ;
            }
            else if( type == 'mariadb-sdb' )
            {
               relationName = sprintf( '?_?', from, to ) ;
            }

            setArrayItemValue( normalInput, 'name', 'Name', { 'value': relationName } ) ;
         }

         //修改被关联服务名的事件
         function switchCreateRelationTo( name, key, value )
         {
            var relationName = '' ;
            var current = $scope.CreateRelationWindow['config']['normal'].getValue() ;
            var type = current['Type'] ;
            var from = current['From'] ;

            if( type == 'pg-sdb' )
            {
               var dbName = current['DbName'] ;
               
               //pg不支持‘-’，如果名字带有‘-’修改为‘_’
               from = from.replace( /\-/g, '_' ) ;
               var to = value.replace( /\-/g, '_' ) ;
               relationName = sprintf( '?_?_?', from, to, dbName ) ;
            }  
            else if( type == 'mysql-sdb' )
            {
               relationName = sprintf( '?_?', from, value ) ;
            }
            else if( type == 'mariadb-sdb' )
            {
               relationName = sprintf( '?_?', from, value ) ;
            }

            setArrayItemValue( normalInput, 'name', 'Name', { 'value': relationName } ) ;
            setCreateRelationAddress( value, advanceInput ) ;
         }

         //获取业务列表
         $.each( $scope.ModuleList, function( index, moduleInfo ){
            if( moduleInfo['BusinessType'] == 'sequoiasql-postgresql' )
            {
               pgsqlBusList.push( { 'key': moduleInfo['BusinessName'], 'value': moduleInfo['BusinessName'] } ) ;
            }
            else if( moduleInfo['BusinessType'] == 'sequoiasql-mysql' )
            {
               mysqlBusList.push( { 'key': moduleInfo['BusinessName'], 'value': moduleInfo['BusinessName'] } ) ;
            }
            else if( moduleInfo['BusinessType'] == 'sequoiasql-mariadb' )
            {
               mariadbBusList.push( { 'key': moduleInfo['BusinessName'], 'value': moduleInfo['BusinessName'] } ) ;
            }
            else if( moduleInfo['BusinessType'] == 'sequoiadb' )
            {
               toValid.push( { 'key': moduleInfo['BusinessName'], 'value': moduleInfo['BusinessName'] } ) ;
            }
         } ) ;

         if ( mysqlBusList.length > 0 )
         {
            typeValid.push( { 'key': $scope.autoLanguage( 'MySQL 关联 SequoiaDB' ), 'value': 'mysql-sdb' } ) ;
         }
         if ( mariadbBusList.length > 0 )
         {
            typeValid.push( { 'key': $scope.autoLanguage( 'MariaDB 关联 SequoiaDB' ), 'value': 'mariadb-sdb' } ) ;
         }
         if ( pgsqlBusList.length > 0 )
         {
            typeValid.push( { 'key': $scope.autoLanguage( 'PostgreSQL 关联 SequoiaDB' ), 'value': 'pg-sdb' } ) ;
         }

         if ( typeValid[0]['value'] == 'pg-sdb' )
         {
            fromValid = pgsqlBusList ;

            setArrayItemValue( normalInput,  'name', 'DbName',           { 'enable': true } ) ;
            setArrayItemValue( advanceInput, 'name', 'preferedinstance', { 'enable': true } ) ;
            setArrayItemValue( advanceInput, 'name', 'transaction',      { 'enable': true } ) ;

            setArrayItemValue( advanceInput, 'name', 'sequoiadb_auto_partition',                { 'enable': false } ) ;
            setArrayItemValue( advanceInput, 'name', 'sequoiadb_use_bulk_insert',               { 'enable': false } ) ;
            setArrayItemValue( advanceInput, 'name', 'sequoiadb_bulk_insert_size',              { 'enable': false } ) ;
            setArrayItemValue( advanceInput, 'name', 'sequoiadb_replica_size',                  { 'enable': false } ) ;
            setArrayItemValue( advanceInput, 'name', 'sequoiadb_selector_pushdown_threshold',   { 'enable': false } ) ;
            setArrayItemValue( advanceInput, 'name', 'sequoiadb_alter_table_overhead_threshold',{ 'enable': false } ) ;
            setArrayItemValue( advanceInput, 'name', 'sequoiadb_debug_log',                     { 'enable': false } ) ;
            setArrayItemValue( advanceInput, 'name', 'sequoiadb_error_level',                   { 'enable': false } ) ;
            setArrayItemValue( advanceInput, 'name', 'sequoiadb_execute_only_in_mysql',         { 'enable': false } ) ;
            setArrayItemValue( advanceInput, 'name', 'sequoiadb_optimizer_options',             { 'enable': false } ) ;
            setArrayItemValue( advanceInput, 'name', 'sequoiadb_rollback_on_timeout',           { 'enable': false } ) ;

            getPostgresqlDB( $scope.ClusterList[$scope.CurrentCluster]['ClusterName'], fromValid[0]['value'], function( dbList ){

               clearArray( dbValid ) ;

               $.each( dbList, function( index, dbInfo ){
                  dbValid.push( { 'key': dbInfo['datname'], 'value': dbInfo['datname'] } ) ;
               } ) ;

               var from = fromValid[0]['value'].replace( /\-/g, '_' ) ;
               var to   = toValid[0]['value'].replace( /\-/g, '_' ) ;
               var relationName = sprintf( '?_?_?', from, to, dbValid[0]['value'] ) ;

               setArrayItemValue( normalInput, 'name', 'Name',   { 'value': relationName } ) ;
               setArrayItemValue( normalInput, 'name', 'DbName', { 'value': dbValid[0]['value'], 'valid': dbValid } ) ;
            } ) ;
         }
         else if ( typeValid[0]['value'] == 'mysql-sdb' )
         {
            fromValid = mysqlBusList ;

            setArrayItemValue( advanceInput, 'name', 'sequoiadb_auto_partition',                { 'enable': true } ) ;
            setArrayItemValue( advanceInput, 'name', 'sequoiadb_use_bulk_insert',               { 'enable': true } ) ;
            setArrayItemValue( advanceInput, 'name', 'sequoiadb_bulk_insert_size',              { 'enable': true } ) ;
            setArrayItemValue( advanceInput, 'name', 'sequoiadb_replica_size',                  { 'enable': true } ) ;
            setArrayItemValue( advanceInput, 'name', 'sequoiadb_selector_pushdown_threshold',   { 'enable': true } ) ;
            setArrayItemValue( advanceInput, 'name', 'sequoiadb_alter_table_overhead_threshold',{ 'enable': true } ) ;
            setArrayItemValue( advanceInput, 'name', 'sequoiadb_debug_log',                     { 'enable': true } ) ;
            setArrayItemValue( advanceInput, 'name', 'sequoiadb_error_level',                   { 'enable': true } ) ;
            setArrayItemValue( advanceInput, 'name', 'sequoiadb_execute_only_in_mysql',         { 'enable': true } ) ;
            setArrayItemValue( advanceInput, 'name', 'sequoiadb_optimizer_options',             { 'enable': true } ) ;
            setArrayItemValue( advanceInput, 'name', 'sequoiadb_rollback_on_timeout',           { 'enable': true } ) ;

            setArrayItemValue( normalInput,  'name', 'DbName',           { 'enable': false } ) ;
            setArrayItemValue( advanceInput, 'name', 'preferedinstance', { 'enable': false } ) ;
            setArrayItemValue( advanceInput, 'name', 'transaction',      { 'enable': false } ) ;

            var relationName = sprintf( '?_?', fromValid[0]['value'], toValid[0]['value'] ) ;
            setArrayItemValue( normalInput, 'name', 'Name', { 'value': relationName } ) ;
         }
         else if ( typeValid[0]['value'] == 'mariadb-sdb' )
         {
            fromValid = mariadbBusList ;

            setArrayItemValue( advanceInput, 'name', 'sequoiadb_auto_partition',                { 'enable': true } ) ;
            setArrayItemValue( advanceInput, 'name', 'sequoiadb_use_bulk_insert',               { 'enable': true } ) ;
            setArrayItemValue( advanceInput, 'name', 'sequoiadb_bulk_insert_size',              { 'enable': true } ) ;
            setArrayItemValue( advanceInput, 'name', 'sequoiadb_replica_size',                  { 'enable': true } ) ;
            setArrayItemValue( advanceInput, 'name', 'sequoiadb_selector_pushdown_threshold',   { 'enable': true } ) ;
            setArrayItemValue( advanceInput, 'name', 'sequoiadb_alter_table_overhead_threshold',{ 'enable': true } ) ;
            setArrayItemValue( advanceInput, 'name', 'sequoiadb_debug_log',                     { 'enable': true } ) ;
            setArrayItemValue( advanceInput, 'name', 'sequoiadb_error_level',                   { 'enable': true } ) ;
            setArrayItemValue( advanceInput, 'name', 'sequoiadb_execute_only_in_mysql',         { 'enable': true } ) ;
            setArrayItemValue( advanceInput, 'name', 'sequoiadb_optimizer_options',             { 'enable': true } ) ;
            setArrayItemValue( advanceInput, 'name', 'sequoiadb_rollback_on_timeout',           { 'enable': true } ) ;

            setArrayItemValue( normalInput,  'name', 'DbName',           { 'enable': false } ) ;
            setArrayItemValue( advanceInput, 'name', 'preferedinstance', { 'enable': false } ) ;
            setArrayItemValue( advanceInput, 'name', 'transaction',      { 'enable': false } ) ;

            var relationName = sprintf( '?_?', fromValid[0]['value'], toValid[0]['value'] ) ;
            setArrayItemValue( normalInput, 'name', 'Name', { 'value': relationName } ) ;
         }

         //设置表单规则,默认值,事件
         setArrayItemValue( normalInput, 'name', 'Type',   { 'value': typeValid[0]['value'], 'valid': typeValid, 'onChange': switchCreateRelationType } ) ;
         setArrayItemValue( normalInput, 'name', 'From',   { 'value': fromValid[0]['value'], 'valid': fromValid, 'onChange': switchCreateRelationFrom } ) ;
         setArrayItemValue( normalInput, 'name', 'DbName', { 'onChange': switchCreateRelationDbName } ) ;
         setArrayItemValue( normalInput, 'name', 'To',     { 'value': toValid[0]['value'],   'valid': toValid,   'onChange': switchCreateRelationTo   } ) ;

         setCreateRelationAddress( toValid[0]['value'], advanceInput ) ;

         $scope.CreateRelationWindow['config']['normal']['inputList']  = normalInput ;
         $scope.CreateRelationWindow['config']['advance']['inputList'] = advanceInput ;

         $scope.CreateRelationWindow['callback']['SetOkButton']( $scope.autoLanguage( '确定' ), function(){
            var inputErrNum1 = $scope.CreateRelationWindow['config']['normal'].getErrNum() ;
            var inputErrNum2 = $scope.CreateRelationWindow['config']['advance'].getErrNum() ;
            var isAllClear   = ( inputErrNum1 == 0 && inputErrNum2 == 0 ) ;

            $scope.CreateRelationWindow['config'].inputErrNum1 = inputErrNum1 ;
            $scope.CreateRelationWindow['config'].inputErrNum2 = inputErrNum2 ;

            if( isAllClear )
            {
               var options  = {} ;
               var formVal1 = $scope.CreateRelationWindow['config']['normal'].getValue() ;
               var formVal2 = $scope.CreateRelationWindow['config']['advance'].getValue() ;
               var address  = formVal2['address'].join( ',' ) ;

               for ( var key in formVal2 )
               {
                  if ( key == 'address' ||
                       ( key == 'sequoiadb_auto_partition' && formVal2['sequoiadb_auto_partition'] == "ON" ) ||
                       ( key == 'sequoiadb_use_bulk_insert' && formVal2['sequoiadb_use_bulk_insert'] == "ON" ) ||
                       ( key == 'sequoiadb_bulk_insert_size' && formVal2['sequoiadb_bulk_insert_size'] == 2000 ) ||
                       ( key == 'sequoiadb_replica_size' && formVal2['sequoiadb_replica_size'] == 1 ) ||
                       ( key == 'sequoiadb_selector_pushdown_threshold' && formVal2['sequoiadb_selector_pushdown_threshold'] == 30 ) ||
                       ( key == 'sequoiadb_debug_log' && formVal2['sequoiadb_debug_log'] == "OFF" ) ||
                       ( key == 'sequoiadb_alter_table_overhead_threshold' && formVal2['sequoiadb_alter_table_overhead_threshold'] == 10000000 ) ||
                       ( key == 'sequoiadb_error_level' && formVal2['sequoiadb_error_level'] == "error" ) ||
                       ( key == 'sequoiadb_execute_only_in_mysql' && formVal2['sequoiadb_execute_only_in_mysql'] == "OFF" ) ||
                       ( key == 'sequoiadb_optimizer_options' && formVal2['sequoiadb_optimizer_options'] == "direct_count,direct_delete,direct_update" ) ||
                       ( key == 'sequoiadb_rollback_on_timeout' && formVal2['sequoiadb_rollback_on_timeout'] == 'OFF' )
                     )
                  {
                     continue ;
                  }
                  options[key] = formVal2[key] ;
               }

               if ( formVal1['Type'] == 'pg-sdb' )
               {
                  options['DbName']  = formVal1['DbName'] ;
                  options['address'] = address ;
               }
               else if ( formVal1['Type'] == 'mysql-sdb' )
               {
                  options['sequoiadb_conn_addr'] = address ;
               }
               else if ( formVal1['Type'] == 'mariadb-sdb' )
               {
                  options['sequoiadb_conn_addr'] = address ;
               }

               createRelation( formVal1['Name'], formVal1['From'], formVal1['To'], options ) ;
            }
            else
            {
               if ( inputErrNum1 > 0 )
               {
                  $scope.CreateRelationWindow['config'].ShowType = 1 ;
                  $scope.CreateRelationWindow['config']['normal'].scrollToError( null ) ;
               }
               else if ( inputErrNum2 > 0 )
               {
                  $scope.CreateRelationWindow['config'].ShowType = 2 ;
                  $scope.CreateRelationWindow['config']['advance'].scrollToError( null ) ;
               }
            }

            return isAllClear ;
         } ) ;

         $scope.CreateRelationWindow['callback']['SetTitle']( $scope.autoLanguage( '添加实例存储' ) ) ;
         $scope.CreateRelationWindow['callback']['Open']() ;
      } ;

      SdbSwap.RelationshipPromise.then( function( data ){
         var isShowRelationTip = SdbFunction.LocalData( 'ShowRelationTip' ) ;
         if( data['relationship'].length == 0 && isNull( isShowRelationTip ) )
         {
            var serviceNum1 = 0 ;
            var serviceNum2 = 0 ;
            $.each( data['moduleList'], function( index, info ){
               if( info['BusinessType'] == 'sequoiadb' && info['DeployMod'] == 'distribution' )
               {
                  ++serviceNum1 ;
               }
               else if( info['BusinessType'] == 'sequoiasql-mysql' || info['BusinessType'] == 'sequoiasql-postgresql' )
               {
                  ++serviceNum2 ;
               }
            } ) ;
            if( serviceNum1 > 0 && serviceNum2 > 0 )
            {
               SdbFunction.LocalData( 'ShowRelationTip', true ) ;
               $scope.Components.Confirm.type = 2 ;
               $scope.Components.Confirm.context = $scope.autoLanguage( '是否为实例添加分布式存储？' ) ;
               $scope.Components.Confirm.isShow = true ;
               $scope.Components.Confirm.okText = $scope.autoLanguage( '是' ) ;
               $scope.Components.Confirm.ok = function(){
                  $scope.Components.Confirm.isShow = false ;
                  $scope.OpenCreateRelation() ;
               }
            }
         }
      } ) ;

      //解除关联 弹窗
      $scope.RemoveRelationWindow = {
         'config': {},
         'callback': {}
      } ;

      //打开 解除关联 弹窗
      $scope.OpenRemoveRelation= function(){
         if( SdbSwap.relationshipList.length == 0 )
         {
            return;
         }
         
         var relationInfoList = [] ;

         $.each( SdbSwap.relationshipList, function( index, relationInfo ){
            var from = '' ;
            var to = '' ;
            $.each( $scope.ModuleList, function( index2, moduleInfo ){
               var moduleType = moduleInfo['BusinessType'] ;

               if ( moduleType == 'sequoiasql-mysql' )
               {
                  moduleType = 'mysql' ;
               }
               else if ( moduleType == 'sequoiasql-postgresql' )
               {
                  moduleType = 'postgresql' ;
               }

               if( relationInfo['To'] == moduleInfo['BusinessName'] )
               {
                  to = relationInfo['To'] + '  ( ' + moduleType + ' )' ;
               }
               if( relationInfo['From'] == moduleInfo['BusinessName'] )
               {
                  from = relationInfo['From'] + '  ( ' + moduleType + ' )' ;
               }
            } ) ;
            
            relationInfoList.push(
               { 'key': relationInfo['Name'], 'value': index, 'to': to, 'from': from }
            ) ;
         } ) ;

         $scope.RemoveRelationWindow['config'] = {
            inputList: [
               {
                  "name": "Name",
                  "webName": $scope.autoLanguage( '关联名' ),
                  "type": "select",
                  "required": true,
                  "value": relationInfoList[0]['value'],
                  "valid": relationInfoList,
                  "onChange": function( name, key, value ){
                     $scope.RemoveRelationWindow['config']['inputList'][1]['value'] = relationInfoList[value]['from'] ;
                     $scope.RemoveRelationWindow['config']['inputList'][2]['value'] = relationInfoList[value]['to'] ;
                  }
               },
               {
                  "name": "from",
                  "webName": $scope.autoLanguage( '实例名' ),
                  "type": "string",
                  "disabled": true,
                  "value": relationInfoList[0]['from']
               },
               {
                  "name": "to",
                  "webName": $scope.autoLanguage( '分布式存储' ),
                  "type": "string",
                  "disabled": true,
                  "value": relationInfoList[0]['to']
               }
            ]
         } ;


         $scope.RemoveRelationWindow['callback']['SetOkButton']( $scope.autoLanguage( '确定' ), function(){
            var isAllClear = $scope.RemoveRelationWindow['config'].check() ;
            if( isAllClear )
            {
               var formVal = $scope.RemoveRelationWindow['config'].getValue() ;
               removeRelation( relationInfoList[formVal['Name']]['key'] ) ;
            }
            return isAllClear ;
         } ) ;
         $scope.RemoveRelationWindow['callback']['SetTitle']( $scope.autoLanguage( '移除实例存储' ) ) ;
         $scope.RemoveRelationWindow['callback']['Open']() ;
      }
      
      //关联信息 弹窗
      $scope.RelationshipWindow = {
         'config': [],
         'callback': {}
      } ;

      //打开 关联信息 弹窗
      $scope.ShowRelationship = function( moduleName ){
         $scope.RelationshipWindow['config'] = [] ;
         $.each( SdbSwap.relationshipList, function( index, info ){
            if( moduleName == info['From'] )
            {
               info['where'] = 'From' ;
            }
            else if( moduleName == info['To'] )
            {
               info['where'] = 'To' ;
            }
            else
            {
               return ;
            }
            $scope.RelationshipWindow['config'].push( info ) ;

         } ) ;
         $scope.RelationshipWindow['callback']['SetTitle']( $scope.autoLanguage( '关联信息' ) ) ;
         $scope.RelationshipWindow['callback']['SetCloseButton']( $scope.autoLanguage( '关闭' ), function(){
            $scope.RelationshipWindow['callback']['Close']() ;
         } ) ;
         $scope.RelationshipWindow['callback']['Open']() ;
      }
   } ) ;
}());