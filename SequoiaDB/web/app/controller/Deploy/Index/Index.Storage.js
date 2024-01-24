//@ sourceURL=Deploy.Index.Storage.Ctrl.js
//"use strict" ;
(function(){
   var sacApp = window.SdbSacManagerModule ;
      //存储控制器
   sacApp.controllerProvider.register( 'Deploy.Index.Storage.Ctrl', function( $scope, $rootScope, $location, SdbFunction, SdbRest, SdbSignal, SdbSwap ){

      $scope.UninstallTips = '' ;
      //服务扩容 弹窗
      $scope.ExtendWindow = {
         'config': {},
         'callback': {}
      } ;

      //打开 服务扩容 弹窗
      var showExtendWindow = function(){
         if( $scope.ClusterList.length > 0 && SdbSwap.distributionNum != 0 )
         {
            $scope.ExtendWindow['config'] = {
               inputList: [
                  {
                     "name": 'moduleName',
                     "webName": $scope.autoLanguage( '存储集群名' ),
                     "type": "select",
                     "value": null,
                     "valid": []
                  }
               ]
            } ;
            var clusterName = $scope.ClusterList[ $scope.CurrentCluster ]['ClusterName'] ;
            $.each( $scope.StorageList, function( index, moduleInfo ){
               if( clusterName == moduleInfo['ClusterName'] && moduleInfo['BusinessType'] == 'sequoiadb' && moduleInfo['DeployMod'] == 'distribution' )
               {
                  if( $scope.ExtendWindow['config']['inputList'][0]['value'] == null )
                  {
                     $scope.ExtendWindow['config']['inputList'][0]['value'] = index ;
                  }
                  $scope.ExtendWindow['config']['inputList'][0]['valid'].push( { 'key': moduleInfo['BusinessName'], 'value': index } )
               }
            } ) ;
            $scope.ExtendWindow['callback']['SetOkButton']( $scope.autoLanguage( '确定' ), function(){
               var isAllClear = $scope.ExtendWindow['config'].check() ;
               if( isAllClear )
               {
                  var formVal = $scope.ExtendWindow['config'].getValue() ;
                  $rootScope.tempData( 'Deploy', 'Model',       'Module' ) ;
                  $rootScope.tempData( 'Deploy', 'Module',      'sequoiadb' ) ;
                  $rootScope.tempData( 'Deploy', 'ModuleName',  $scope.StorageList[ formVal['moduleName'] ]['BusinessName'] ) ;
                  $rootScope.tempData( 'Deploy', 'ClusterName', $scope.ClusterList[ $scope.CurrentCluster ]['ClusterName'] ) ;
                  $rootScope.tempData( 'Deploy', 'DeployMod',   $scope.StorageList[ formVal['moduleName'] ]['DeployMod'] ) ;
                  SdbFunction.LocalData( 'SdbClusterName', $scope.ClusterList[ $scope.CurrentCluster ]['ClusterName'] ) ;
                  SdbFunction.LocalData( 'SdbModuleName',  $scope.StorageList[ formVal['moduleName'] ]['BusinessName'] ) ;
                  $location.path( '/Deploy/SDB-ExtendConf' ).search( { 'r': new Date().getTime() } ) ;
               }
               return isAllClear ;
            } ) ;
            $scope.ExtendWindow['callback']['SetTitle']( $scope.autoLanguage( '存储集群扩容' ) ) ;
            $scope.ExtendWindow['callback']['SetIcon']( '' ) ;
            $scope.ExtendWindow['callback']['Open']() ;
         }
      }

      //服务减容 弹窗
      $scope.ShrinkWindow = {
         'config': {},
         'callback': {}
      }

      //打开 服务减容 弹窗
      var showShrinkWindow = function(){
         if( $scope.ClusterList.length > 0 && SdbSwap.distributionNum != 0 )
         {
            $scope.ShrinkWindow['config'] = {
               inputList: [
                  {
                     "name": 'moduleName',
                     "webName": $scope.autoLanguage( '存储集群名' ),
                     "type": "select",
                     "value": null,
                     "valid": []
                  }
               ]
            } ;
            var clusterName = $scope.ClusterList[ $scope.CurrentCluster ]['ClusterName'] ;
            $.each( $scope.StorageList, function( index, moduleInfo ){
               if( clusterName == moduleInfo['ClusterName'] && moduleInfo['BusinessType'] == 'sequoiadb' && moduleInfo['DeployMod'] == 'distribution' )
               {
                  if( $scope.ShrinkWindow['config']['inputList'][0]['value'] == null )
                  {
                     $scope.ShrinkWindow['config']['inputList'][0]['value'] = index ;
                  }
                  $scope.ShrinkWindow['config']['inputList'][0]['valid'].push( { 'key': moduleInfo['BusinessName'], 'value': index } )
               }
            } ) ;
            $scope.ShrinkWindow['callback']['SetOkButton']( $scope.autoLanguage( '确定' ), function(){
               var isAllClear = $scope.ShrinkWindow['config'].check() ;
               if( isAllClear )
               {
                  var formVal = $scope.ShrinkWindow['config'].getValue() ;
                  $rootScope.tempData( 'Deploy', 'Model',       'Module' ) ;
                  $rootScope.tempData( 'Deploy', 'Module',      'sequoiadb' ) ;
                  $rootScope.tempData( 'Deploy', 'ModuleName',  $scope.StorageList[ formVal['moduleName'] ]['BusinessName'] ) ;
                  $rootScope.tempData( 'Deploy', 'ClusterName', $scope.ClusterList[ $scope.CurrentCluster ]['ClusterName'] ) ;
                  $rootScope.tempData( 'Deploy', 'Shrink', true ) ;
                  SdbFunction.LocalData( 'SdbModuleName',  $scope.StorageList[ formVal['moduleName'] ]['BusinessName'] ) ;
                  $location.path( '/Deploy/SDB-ShrinkConf' ).search( { 'r': new Date().getTime() } ) ;
               }
               return isAllClear ;
            } ) ;
            $scope.ShrinkWindow['callback']['SetTitle']( $scope.autoLanguage( '存储集群减容' ) ) ;
            $scope.ShrinkWindow['callback']['SetIcon']( '' ) ;
            $scope.ShrinkWindow['callback']['Open']() ;
         }
      }

      //同步服务 弹窗
      $scope.SyncWindow = {
         'config': {},
         'callback': {}
      }

      //打开 同步服务 弹窗
      var showSyncWindow = function(){
         if( SdbSwap.sdbModuleNum != 0 )
         {
            $scope.SyncWindow['config'] = {
               inputList: [
                  {
                     "name": 'moduleName',
                     "webName": $scope.autoLanguage( '存储集群名' ),
                     "type": "select",
                     "value": null,
                     "valid": []
                  }
               ]
            } ;
            var clusterName = $scope.ClusterList[ $scope.CurrentCluster ]['ClusterName'] ;
            $.each( $scope.StorageList, function( index, moduleInfo ){
               if( clusterName == moduleInfo['ClusterName'] && moduleInfo['BusinessType'] == 'sequoiadb')
               {
                  if( $scope.SyncWindow['config']['inputList'][0]['value'] == null )
                  {
                     $scope.SyncWindow['config']['inputList'][0]['value'] = index ;
                  }
                  $scope.SyncWindow['config']['inputList'][0]['valid'].push( { 'key': moduleInfo['BusinessName'], 'value': index } )
               }
            } ) ;
            $scope.SyncWindow['callback']['SetOkButton']( $scope.autoLanguage( '确定' ), function(){
               var isAllClear = $scope.SyncWindow['config'].check() ;
               if( isAllClear )
               {
                  var formVal = $scope.SyncWindow['config'].getValue() ;
                  $rootScope.tempData( 'Deploy', 'ModuleName',  $scope.StorageList[ formVal['moduleName'] ]['BusinessName'] ) ;
                  $rootScope.tempData( 'Deploy', 'ClusterName', $scope.ClusterList[ $scope.CurrentCluster ]['ClusterName'] ) ;
                  $rootScope.tempData( 'Deploy', 'InstallPath', $scope.ClusterList[ $scope.CurrentCluster ]['InstallPath'] ) ;
                  $location.path( '/Deploy/SDB-Sync' ).search( { 'r': new Date().getTime() } ) ;
               }
               return isAllClear ;
            } ) ;
            $scope.SyncWindow['callback']['SetTitle']( $scope.autoLanguage( '同步配置' ) ) ;
            $scope.SyncWindow['callback']['SetIcon']( '' ) ;
            $scope.SyncWindow['callback']['Open']() ;
         }
      }

      //重启服务 弹窗
      $scope.RestartWindow = {
         'config': {
            'inputList': [
               {
                  "name": 'moduleName',
                  "webName": $scope.autoLanguage( '存储集群名' ),
                  "type": "select",
                  "value": null,
                  "valid": []
               }
            ]
         },
         'callback': {}
      }

      function restartModule( clusterName, moduleName )
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
                  restartModule( clusterName, moduleName ) ;
                  return true ;
               } ) ;
            }
         } ) ;
      }

      //打开 重启服务 弹窗
      var showRestartWindow = function(){
         if( $scope.StorageNum == 0 )
         {
            return ;
         }
         $scope.RestartWindow['config']['inputList'][0]['value'] = null ;
         $scope.RestartWindow['config']['inputList'][0]['valid'] = [] ;

         var clusterName = $scope.ClusterList[ $scope.CurrentCluster ]['ClusterName'] ;
         $.each( $scope.StorageList, function( index, moduleInfo ){
            if( clusterName == moduleInfo['ClusterName'] && moduleInfo['BusinessType'] == 'sequoiadb' )
            {
               if( $scope.RestartWindow['config']['inputList'][0]['value'] == null )
               {
                  $scope.RestartWindow['config']['inputList'][0]['value'] = index ;
               }
               $scope.RestartWindow['config']['inputList'][0]['valid'].push( { 'key': moduleInfo['BusinessName'], 'value': index } )
            }
         } ) ;

         $scope.RestartWindow['callback']['SetOkButton']( $scope.autoLanguage( '确定' ), function(){
            var isAllClear = $scope.RestartWindow['config'].check() ;
            if( isAllClear )
            {
               var formVal = $scope.RestartWindow['config'].getValue() ;
               $scope.Components.Confirm.type = 2 ;
               $scope.Components.Confirm.context = $scope.autoLanguage( '该操作将重启所有节点，是否确定继续？' ) ;
               $scope.Components.Confirm.isShow = true ;
               $scope.Components.Confirm.okText = $scope.autoLanguage( '确定' ) ;
               $scope.Components.Confirm.ok = function(){
                  restartModule( $scope.ClusterList[ $scope.CurrentCluster ]['ClusterName'], $scope.StorageList[ formVal['moduleName'] ]['BusinessName'] ) ;
                  $scope.Components.Confirm.isShow = false ;
               }
            }
            return isAllClear ;
         } ) ;
         $scope.RestartWindow['callback']['SetTitle']( $scope.autoLanguage( '重启存储集群' ) ) ;
         $scope.RestartWindow['callback']['SetIcon']( '' ) ;
         $scope.RestartWindow['callback']['Open']() ;
      }

      //添加服务 弹窗
      $scope.InstallModule = {
         'config': {},
         'callback': {}
      } ;

      //打开 添加服务 弹窗
      $scope.ShowInstallModule = function(){
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
                     "webName": $scope.autoLanguage( '存储集群名' ),
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
                     "webName": $scope.autoLanguage( '类型' ),
                     "type": "select",
                     "value": null,
                     "valid": []
                  }
               ]
            } ;
            $scope.InstallModule['config']['inputList'][0]['value'] = SdbSwap.generateModuleName( 'SequoiaDB' ) ;
            $.each( SdbSwap.moduleType, function( index, typeInfo ){
               if( typeInfo['BusinessType'] == 'sequoiasql-mysql' || typeInfo['BusinessType'] == 'sequoiasql-postgresql' || typeInfo['BusinessType'] == 'sequoiasql-mariadb' )
               {
                  return true ;
               }
               if ( isNull( $scope.InstallModule['config']['inputList'][1]['value'] ) )
               {
                  $scope.InstallModule['config']['inputList'][1]['value'] = index ;
               }
               $scope.InstallModule['config']['inputList'][1]['valid'].push( { 'key': typeInfo['BusinessDesc'], 'value': index } ) ;
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
                     return [ { 'name': 'moduleName', 'error': $scope.autoLanguage( '存储集群名已经存在' ) } ]
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
                  if( SdbSwap.moduleType[ formVal['moduleType'] ]['BusinessType'] == 'sequoiadb' )
                  {
                     $location.path( '/Deploy/SDB-Conf' ).search( { 'r': new Date().getTime() } ) ;
                  }
                  //当服务类型是postgresql时
                  else if( SdbSwap.moduleType[ formVal['moduleType'] ]['BusinessType'] == 'sequoiasql-postgresql' )
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
                     /*if( checkSqlHost == 0 )
                     {
                        $scope.Components.Confirm.type = 3 ;
                        $scope.Components.Confirm.context = $scope.autoLanguage( '创建 SequoiaSQL-PostgreSQL 服务需要主机已经部署 SequoiaSQL-PostgreSQL 包。' ) ;
                        $scope.Components.Confirm.isShow = true ;
                        $scope.Components.Confirm.noClose = true ;
                     }
                     else*/
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
                     /*if( checkSqlHost == 0 )
                     {
                        $scope.Components.Confirm.type = 3 ;
                        $scope.Components.Confirm.context = $scope.autoLanguage( '创建 SequoiaSQL-MySQL 服务需要主机已经部署 SequoiaSQL-MySQL 包。' ) ;
                        $scope.Components.Confirm.isShow = true ;
                        $scope.Components.Confirm.noClose = true ;
                     }
                     else*/
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
               }
               return isAllClear ;
            } ) ;
            $scope.InstallModule['callback']['SetTitle']( $scope.autoLanguage( '创建存储集群' ) ) ;
            $scope.InstallModule['callback']['SetIcon']( '' ) ;
            $scope.InstallModule['callback']['Open']() ;
         }
      }

      //发现服务 弹窗
      $scope.AppendModule = {
         'config': {},
         'callback': {}
      } ;

      //打开 发现服务 弹窗
      $scope.ShowAppendModule = function(){
         if( $scope.ClusterList.length == 0 )
         {
            return ;
         }
         $scope.AppendModule['config'] = {
            inputList: [
               {
                  "name": 'moduleName',
                  "webName": $scope.autoLanguage( '存储集群名' ),
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
                  "webName": $scope.autoLanguage( '类型' ),
                  "type": "select",
                  "value": 'sequoiadb',
                  "valid": [
                     { 'key': 'SequoiaDB', 'value': 'sequoiadb' }
                  ]
               }
            ]
         } ;
         
         $scope.AppendModule['config']['inputList'][0]['value'] = SdbSwap.generateModuleName( 'SequoiaDB' ) ;
         $scope.AppendModule['callback']['SetOkButton']( $scope.autoLanguage( '确定' ), function(){
            var isAllClear = $scope.AppendModule['config'].check( function( formVal ){
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
                  return [ { 'name': 'moduleName', 'error': $scope.autoLanguage( '存储集群名已经存在' ) } ]
               }
               else
               {
                  return [] ;
               }
            } ) ;
            if( isAllClear )
            {
               $scope.AppendModule['callback']['Close']() ;
               var formVal = $scope.AppendModule['config'].getValue() ;
               setTimeout( function(){
                  showAppendSdb( formVal['moduleName'] ) ;
                  $scope.$apply() ;
               } ) ;
            }
            else
            {
               return false ;
            }
         } ) ;
         $scope.AppendModule['callback']['SetTitle']( $scope.autoLanguage( '添加已有的存储集群' ) ) ;
         $scope.AppendModule['callback']['SetIcon']( '' ) ;
         $scope.AppendModule['callback']['Open']() ;
      }

      //从发现前往添加主机
      function gotoAddHost( configure )
      {
         $rootScope.tempData( 'Deploy', 'Model', 'Deploy' ) ;
         $rootScope.tempData( 'Deploy', 'Module', configure['BusinessType'] );
         $rootScope.tempData( 'Deploy', 'ClusterName', $scope.ClusterList[ $scope.CurrentCluster ]['ClusterName'] ) ;
         $rootScope.tempData( 'Deploy', 'InstallPath', $scope.ClusterList[ $scope.CurrentCluster ]['InstallPath'] ) ;
         $rootScope.tempData( 'Deploy', 'DiscoverConf', configure ) ;
         $location.path( '/Deploy/ScanHost' ).search( { 'r': new Date().getTime() } ) ;
      }

      //发现sdb 弹窗
      $scope.AppendSdb = {
         'config': {},
         'callback': {}
      }

      //打开 发现sdb 弹窗
      var showAppendSdb = function( moduleName ){
         $scope.AppendSdb['config'] = {
            inputList: [
               {
                  "name": 'HostName',
                  "webName": $scope.autoLanguage( '地址' ),
                  "type": "string",
                  "required": true,
                  "desc": $scope.autoLanguage( 'coord节点或standalone所在的主机名或者IP地址' ),
                  "value": "",
                  "valid": {
                     "min": 1
                  }
               },
               {
                  "name": 'ServiceName',
                  "webName": $scope.autoLanguage( '服务名' ),
                  "type": "port",
                  "required": true,
                  "desc": $scope.autoLanguage( 'coord节点或standalone端口号' ),
                  "value": '',
                  "valid": {}
               },
               {
                  "name": 'User',
                  "webName": $scope.autoLanguage( '数据库用户名' ),
                  "type": "string",
                  "value": ""
               },
               {
                  "name": 'Passwd',
                  "webName": $scope.autoLanguage( '数据库密码' ),
                  "type": "password",
                  "value": ""
               },
               {
                  "name": 'AgentService',
                  "webName": $scope.autoLanguage( '代理端口' ),
                  "type": "port",
                  "value": '11790',
                  "valid": {}
               }
            ]
         }
         $scope.AppendSdb['callback']['SetOkButton']( $scope.autoLanguage( '确定' ), function(){
            var isAllClear = $scope.AppendSdb['config'].check() ;
            if( isAllClear )
            {
               var formVal = $scope.AppendSdb['config'].getValue() ;
               var configure = {} ;
               configure['ClusterName']  = $scope.ClusterList[ $scope.CurrentCluster ]['ClusterName'] ;
               configure['BusinessType'] = 'sequoiadb' ;
               configure['BusinessName'] = moduleName ;
               configure['BusinessInfo'] = formVal ;
               discoverModule( configure ) ;
            }
            return isAllClear ;
         } ) ;
         $scope.AppendSdb['callback']['SetTitle']( 'SequoiaDB' ) ;
         $scope.AppendSdb['callback']['SetIcon']( '' ) ;
         $scope.AppendSdb['callback']['Open']() ;
      }

      //发现服务
      var discoverModule = function( configure ){
         var data = { 'cmd': 'discover business', 'ConfigInfo': JSON.stringify( configure ) } ;
         SdbRest.OmOperation( data, {
            'success': function(){
               if( configure['BusinessType'] == 'sequoiadb' )
               {
                  $rootScope.tempData( 'Deploy', 'ModuleName', configure['BusinessName'] ) ;
                  $rootScope.tempData( 'Deploy', 'ClusterName', configure['ClusterName'] ) ;
                  $location.path( '/Deploy/SDB-Discover' ).search( { 'r': new Date().getTime() }  ) ;
               }
               else
               {
                  $location.path( '/Deploy/Index' ).search( { 'r': new Date().getTime() }  ) ;
               }
            }, 
            'failed': function( errorInfo ){
               if( configure['BusinessType'] == 'sequoiadb' && isArray( errorInfo['hosts'] ) && errorInfo['hosts'].length > 0 )
               {
                  $scope.Components.Confirm.type = 3 ;
                  $scope.Components.Confirm.context = $scope.autoLanguage( '发现SequoiaDB需要先在集群中添加该服务的所有主机。是否前往添加主机？' ) ;
                  $scope.Components.Confirm.isShow = true ;
                  $scope.Components.Confirm.okText = $scope.autoLanguage( '是' ) ;
                  $scope.Components.Confirm.ok = function(){
                     $rootScope.tempData( 'Deploy', 'DiscoverHostList', errorInfo['hosts'] ) ;
                     gotoAddHost( configure ) ;
                  }
               }
               else
               {
                  _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                     discoverModule( configure ) ;
                     return true ;
                  } ) ;
               }
               
            }
         } ) ;
      }

      //卸载服务
      var uninstallModule = function( index, isForce ){
         if( typeof( $scope.StorageList[index]['AddtionType'] ) == 'undefined' || $scope.StorageList[index]['AddtionType'] != 1 )
         {
            var data = { 'cmd': 'remove business', 'BusinessName': $scope.StorageList[index]['BusinessName'], 'force': isForce } ;
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
            var data = { 'cmd': 'undiscover business', 'ClusterName': $scope.StorageList[index]['ClusterName'], 'BusinessName': $scope.StorageList[index]['BusinessName'] } ;
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

      //卸载服务 弹窗
      $scope.UninstallModuleWindow = {
         'config': {},
         'callback': {}
      } ;

      //打开 卸载服务 弹窗
      var showUninstallModule = function(){
         if( $scope.ClusterList.length == 0 || $scope.StorageNum == 0 )
         {
            return ;
         }
         if( $scope.ModuleNum == 0 )
         {
            $scope.Components.Confirm.type = 3 ;
            $scope.Components.Confirm.context = $scope.autoLanguage( '已经没有服务了。' ) ;
            $scope.Components.Confirm.isShow = true ;
            $scope.Components.Confirm.noClose = true ;
            return ;
         }
         $scope.UninstallTips = '' ;
         var clusterName = $scope.ClusterList[ $scope.CurrentCluster ]['ClusterName'] ;
         $scope.UninstallModuleWindow['config'] = {
            'inputList': [
               {
                  "name": 'moduleIndex',
                  "webName": $scope.autoLanguage( '存储集群名' ),
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
         }
         $.each( $scope.StorageList, function( index, moduleInfo ){
            if( clusterName == moduleInfo['ClusterName'] )
            {
               if( $scope.UninstallModuleWindow['config']['inputList'][0]['value'] == null )
               {
                  $scope.UninstallModuleWindow['config']['inputList'][0]['value'] = index ;
               }
               $scope.UninstallModuleWindow['config']['inputList'][0]['valid'].push( { 'key': moduleInfo['BusinessName'], 'value': index } )
            }
         } ) ;
         $scope.UninstallModuleWindow['callback']['SetOkButton']( $scope.autoLanguage( '确定' ), function(){
            var isAllClear = $scope.UninstallModuleWindow['config'].check() ;
            if( isAllClear )
            {
               var formVal = $scope.UninstallModuleWindow['config'].getValue() ;
               if( formVal['force'] == true )
               {
                  $scope.Components.Confirm.type = 2 ;
                  $scope.Components.Confirm.context = $scope.autoLanguage( '该操作将删除所有数据，是否确定继续？' ) ;
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
         $scope.UninstallModuleWindow['callback']['SetTitle']( $scope.autoLanguage( '删除存储集群' ) ) ;
         $scope.UninstallModuleWindow['callback']['Open']() ;
      }

      //解绑服务
      var unbindModule = function( clusterName, businessName ){
         var data = {
            'cmd': 'unbind business', 'ClusterName': clusterName, 'BusinessName': businessName
         } ;
         SdbRest.OmOperation( data, {
            'success': function(){
               $scope.Components.Confirm.type = 4 ;
               $scope.Components.Confirm.context = sprintf( $scope.autoLanguage( '存储集群：? 移除成功。' ), businessName ) ;
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
                  unbindModule( clusterName, businessName ) ;
                  return true ;
               } ) ;
            }
         } ) ;
      }

      //解绑服务 弹窗
      $scope.UnbindModuleWindow = {
         'config': {},
         'callback': {}
      }

      //打开 解绑服务 弹窗
      var showUnbindModule = function(){
         if( $scope.ClusterList.length > 0 && $scope.ModuleNum != 0 )
         {
            var clusterName = $scope.ClusterList[ $scope.CurrentCluster ]['ClusterName'] ;
            $scope.UnbindModuleWindow['config'] = {
               'inputList': [
                  {
                     "name": 'moduleIndex',
                     "webName": $scope.autoLanguage( '存储集群名' ),
                     "type": "select",
                     "value": null,
                     "valid": []
                  }
               ]
            }
            $.each( $scope.StorageList, function( index, moduleInfo ){
               if( clusterName == moduleInfo['ClusterName'] )
               {
                  if( $scope.UnbindModuleWindow['config']['inputList'][0]['value'] == null )
                  {
                     $scope.UnbindModuleWindow['config']['inputList'][0]['value'] = index ;
                  }
                  $scope.UnbindModuleWindow['config']['inputList'][0]['valid'].push( { 'key': moduleInfo['BusinessName'], 'value': index } )
               }
            } ) ;
            $scope.UnbindModuleWindow['callback']['SetOkButton']( $scope.autoLanguage( '确定' ), function(){
               var isAllClear = $scope.UnbindModuleWindow['config'].check() ;
               if( isAllClear )
               {
                  var formVal = $scope.UnbindModuleWindow['config'].getValue() ;
                  var businessName = $scope.StorageList[ formVal['moduleIndex'] ]['BusinessName'] ;
                  var clusterName = $scope.ClusterList[ $scope.CurrentCluster ]['ClusterName'] ;
                  unbindModule( clusterName, businessName ) ;
               }
               return isAllClear ;
            } ) ;
            $scope.UnbindModuleWindow['callback']['SetTitle']( $scope.autoLanguage( '移除存储集群' ) ) ;
            $scope.UnbindModuleWindow['callback']['Open']() ;
         }
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
               "webName": $scope.autoLanguage( '存储集群名' ),
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
         if( typeof( $scope.StorageList[index]['authority'][0]['User'] ) != 'undefined' )
         {
            user = $scope.StorageList[index]['authority'][0]['User'] ;
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
         $scope.Components.Confirm.title = $scope.autoLanguage( '要删除该存储集群的鉴权吗？' ) ;
         $scope.Components.Confirm.context = $scope.autoLanguage( '存储集群名' ) + ': ' + businessName ;
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

      //判断类型
      $scope.Typeof = function( value ){
         return typeof( value ) ;
      }

      $scope.GotoExpansion = function(){
         $location.path( '/Deploy/SDB-ExtendConf' ).search( { 'r': new Date().getTime() } ) ;
      }

      //添加服务下拉菜单
      $scope.AddModuleDropdown = {
         'config': [
            { 'key': $scope.autoLanguage( '创建存储集群' ) },
            { 'key': $scope.autoLanguage( '添加已有的存储集群' ) }
         ],
         'OnClick': function( index ){
            if( index == 0 )
            {
               $scope.ShowInstallModule() ;
            }
            else
            {
               $scope.ShowAppendModule() ;
            }
            $scope.AddModuleDropdown['callback']['Close']() ;
         },
         'callback': {}
      }

      //打开 添加服务下拉菜单
      $scope.OpenAddModuleDropdown = function( event ){
         if( $scope.ClusterList.length > 0 )
         {
            $scope.AddModuleDropdown['callback']['Open']( event.currentTarget ) ;
         }
      }

      //删除服务下拉菜单
      $scope.DeleteModuleDropdown = {
         'config': [
            { 'key': $scope.autoLanguage( '删除存储集群' ) },
            { 'key': $scope.autoLanguage( '移除存储集群' ) }
         ],
         'OnClick': function( index ){
            if( index == 0 )
            {
               showUninstallModule() ;
            }
            else
            {
               showUnbindModule() ;
            }
            $scope.DeleteModuleDropdown['callback']['Close']() ;
         },
         'callback': {}
      }

      //打开 删除服务下拉菜单
      $scope.OpenDeleteModuleDropdown = function( event ){
         if( $scope.ClusterList.length > 0 )
         {
            $scope.DeleteModuleDropdown['callback']['Open']( event.currentTarget ) ;
         }
      }

      //修改服务下拉菜单
      $scope.EditModuleDropdown = {
         'config': [],
         'OnClick': function( index ){
            if( index == 0 )
            {
               showExtendWindow() ;
            }
            else if( index == 1 )
            {
               showShrinkWindow() ;
            }
            else if( index == 2 )
            {
               showRestartWindow() ;
            }
            else if( index == 3 )
            {
               showSyncWindow() ;
            }
            $scope.EditModuleDropdown['callback']['Close']() ;
         },
         'callback': {}
      }

      //打开 存储集群操作
      $scope.OpenEditModuleDropdown = function( event ){
         if( $scope.ClusterList.length > 0 )
         {
            $scope.EditModuleDropdown['config'] = [] ;
            var disabled = false ;
            var syncDisabled = false ;
            if( SdbSwap.distributionNum == 0 )
            {
               disabled = true ;
            }
            if( SdbSwap.sdbModuleNum == 0 )
            {
               syncDisabled = true ;
            }
            $scope.EditModuleDropdown['config'].push( { 'key': $scope.autoLanguage( '扩容' ), 'disabled': disabled } ) ;
            $scope.EditModuleDropdown['config'].push( { 'key': $scope.autoLanguage( '减容' ), 'disabled': disabled } ) ;
            $scope.EditModuleDropdown['config'].push( { 'key': $scope.autoLanguage( '重启' ), 'disabled': $scope.ModuleNum == 0 } ) ;
            $scope.EditModuleDropdown['config'].push( { 'key': $scope.autoLanguage( '同步配置' ), 'disabled': syncDisabled } ) ;
            $scope.EditModuleDropdown['callback']['Open']( event.currentTarget ) ;
         }
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