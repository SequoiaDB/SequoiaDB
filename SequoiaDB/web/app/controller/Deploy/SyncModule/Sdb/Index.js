//@ sourceURL=Index.js
//"use strict" ;
(function(){
   var sacApp = window.SdbSacManagerModule ;
   //主控制器
   sacApp.controllerProvider.register( 'Deploy.Sdb.Sync.Ctrl', function( $scope, $location, $rootScope, Loading, SdbRest, SdbSignal, SdbPromise ){
      $scope.ContainerBox = [ { offsetY: -106 }, { offsetY: -40 } ] ;
      $scope.ClusterName  = $rootScope.tempData( 'Deploy', 'ClusterName' ) ;
      $scope.ModuleName   = $rootScope.tempData( 'Deploy', 'ModuleName' ) ;
      $scope.SyncConf     = $rootScope.tempData( 'Deploy', 'SyncConf' ) ;
      if( $scope.ModuleName == null || $scope.ClusterName == null )
      {
         $location.path( '/Transfer' ).search( { 'r': new Date().getTime() } ) ;
         return ;
      }

      //如果从安装主机进来的，有步骤条
      if( $scope.SyncConf != null )
      {
         $scope.stepList = _Deploy.BuildSdbSyncStep( $scope, $location, $scope['Url']['Action'], 'sequoiadb' ) ;
      }

      //同步状态
      $scope.SyncStatus = 0 ;

      var afterSyncDefer  = SdbPromise.init( 2 ) ;

      //同步业务
      var syncBusinessConf = function(){
         var data = { 'cmd': 'sync business configure', 'ClusterName': $scope.ClusterName, 'BusinessName': $scope.ModuleName } ;
         SdbRest.OmOperation( data, {
            'success': function(){
               getBusinessConf() ;
            },
            'failed': function( errorInfo ){
               $scope.SyncStatus = 2 ;
               Loading.cancel() ;
               if( isArray( errorInfo['hosts'] ) && errorInfo['hosts'].length > 0 )
               {
                  $scope.Components.Confirm.type = 3 ;
                  $scope.Components.Confirm.context = $scope.autoLanguage( '同步后的业务有主机未添加到OM中，是否前往添加主机？' ) ;
                  $scope.Components.Confirm.isShow = true ;
                  $scope.Components.Confirm.okText = $scope.autoLanguage( '是' ) ;
                  $scope.Components.Confirm.ok = function(){
                     $rootScope.tempData( 'Deploy', 'Model', 'Deploy' ) ;
                     $rootScope.tempData( 'Deploy', 'Module', 'None' ) ;
                     $rootScope.tempData( 'Deploy', 'SyncConf', errorInfo['hosts'] ) ;
                     $location.path( '/Deploy/ScanHost' ).search( { 'r': new Date().getTime() } ) ;
                  }
               }
               else
               {
                  _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                     syncBusinessConf() ;
                     return true ;
                  } ) ;
               }
            }
         } ) ;
      }

      var isFirst = true ;
      //获取业务配置
      var getBusinessConf = function(){
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
                  SdbSignal.commit( "GetBeforeConf", result ) ;
                  afterSyncDefer.resolve( 'BeforeNodeConf', tempBusinessConfig ) ;
                  syncBusinessConf() ;
               }
               else
               {
                  SdbSignal.commit( 'GetAfterConf', result ) ;
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

      //创建loading
      Loading.cancel() ;
      Loading.create() ;

      getBusinessConf() ;

      //同步后节点配置
      afterSyncDefer.then( function( result ){
         SdbSignal.commit( 'GetFinishConf', result ) ;
         $scope.SyncStatus = 1 ;
         Loading.cancel() ;
      } ) ;

      //返回
      $scope.GotoDeploy = function(){
         $location.path( '/Deploy/Index' ).search( { 'r': new Date().getTime() } ) ;
      }

   } ) ;
   //左边概览信息控制器
   sacApp.controllerProvider.register( 'Deploy.Sdb.Sync.Preview.Ctrl', function( $scope, SdbSignal ){
      //同步前的信息
      $scope.BeforeSync = {
         'hostNum': '-',
         'nodeNum': '-',
         'dataGroupNum': '-',
         'dataNum': '-',
         'cataNum': '-',
         'coordNum': '-',
         'minReplicaNum': '-',
         'maxReplicaNum': '-',
         'moduleMode': '-'
      } ;

      //同步后的信息
      $scope.AfterSync = {
         'hostNum': '-',
         'nodeNum': '-',
         'dataGroupNum': '-',
         'dataNum': '-',
         'cataNum': '-',
         'coordNum': '-',
         'minReplicaNum': '-',
         'maxReplicaNum': '-',
         'moduleMode': '-'
      } ;

      var countBusinessInfo = function( syncInfo, nodeList ) {
         if( nodeList.length == 0 )
         {
            return ;
         }
         if( nodeList[0]['DeployMod'] == 'standalone' )
         {
            syncInfo['hostNum'] = 1 ;
            syncInfo['nodeNum'] = 1 ;
            syncInfo['dataGroupNum'] = '-' ;
            syncInfo['dataNum'] = '-' ;
            syncInfo['cataNum'] = '-' ;
            syncInfo['coordNum'] = '-' ;
            syncInfo['minReplicaNum'] = '-' ;
            syncInfo['maxReplicaNum'] = '-' ;
            syncInfo['moduleMode'] = nodeList[0]['DeployMod'] ;
         }
         else
         {
            var replicaMax = 0 ;
            var replicaMin = 10 ;
            var tempGroupsArr = {} ;

            syncInfo['hostNum'] = 0 ;
            syncInfo['nodeNum'] = 0 ;
            syncInfo['dataGroupNum'] = 0 ;
            syncInfo['dataNum'] = 0 ;
            syncInfo['cataNum'] = 0 ;
            syncInfo['coordNum'] = 0 ;
            syncInfo['minReplicaNum'] = 0 ;
            syncInfo['maxReplicaNum'] = 0 ;
            syncInfo['moduleMode'] = nodeList[0]['DeployMod'] ;
            $.each( nodeList, function( index, hostInfo ){
               ++syncInfo['hostNum'] ;
               $.each( hostInfo['Config'], function( index2, nodeInfo ){
                  if( nodeInfo['role'] == 'data' )
                  {
                     if( isNaN( tempGroupsArr[nodeInfo['datagroupname']] ) == true )
                     {
                        tempGroupsArr[nodeInfo['datagroupname']] = 1 ;
                     }
                     else
                     {
                        ++tempGroupsArr[nodeInfo['datagroupname']] ;
                     }
                     ++syncInfo['dataNum'] ;
                  }
                  else if( nodeInfo['role'] == 'coord' )
                  {
                     ++syncInfo['coordNum'] ;
                  }
                  else if( nodeInfo['role'] == 'catalog' )
                  {
                     ++syncInfo['cataNum'] ;
                  }
                  else
                  {
                     return true ;
                  }
                  ++syncInfo['nodeNum'] ;
               } ) ;            
            } ) ;
            $.each( tempGroupsArr, function( index, num ){
               ++syncInfo['dataGroupNum'] ;
               if( num > replicaMax )
               {
                  replicaMax = num ;
               }
               if( num < replicaMin )
               {
                  replicaMin = num ;
               }
            } ) ;
            syncInfo['minReplicaNum'] = replicaMin ;
            syncInfo['maxReplicaNum'] = replicaMax ;
         }
      }

      SdbSignal.on( "GetBeforeConf", function( result ){
         countBusinessInfo( $scope.BeforeSync, result ) ;
      } ) ;

      SdbSignal.on( "GetAfterConf", function( result ){
         countBusinessInfo( $scope.AfterSync, result ) ;
      } ) ;

   } ) ;
   //右边表格
   sacApp.controllerProvider.register( 'Deploy.Sdb.Sync.Table.Ctrl', function( $scope, SdbSignal ){

      //已同步 筛选条件
      var filterSync = function( value ){
         return value != 1 ;
      }
      //有变化 筛选条件
      var filterChange = function( value ){
         return value != 0 ;
      }

      //节点列表
      $scope.NodeTable = {
         'title': {
            'type':                      $scope.autoLanguage( '类型' ),
            'nodeInfo.1.hostName':       $scope.autoLanguage( '主机名' ),
            'nodeInfo.1.svcname':        $scope.autoLanguage( '服务名' ),
            'nodeInfo.1.dbpath':         $scope.autoLanguage( '数据路径' ),
            'nodeInfo.1.role':           $scope.autoLanguage( '角色' ),
            'nodeInfo.1.datagroupname':  $scope.autoLanguage( '分区组' )
         },
         'body': [],
         'options': {
            'width': {
               'type': '70px',
               'nodeInfo.1.hostName': '20%',
               'nodeInfo.1.svcname': '10%',
               'nodeInfo.1.dbpath': '36%',
               'nodeInfo.1.role': '10%',
               'nodeInfo.1.datagroupname': '12%'
            },
            'sort': {
               'type': true,
               'nodeInfo.1.hostName': true,
               'nodeInfo.1.svcname': true,
               'nodeInfo.1.dbpath': true,
               'nodeInfo.1.role': true,
               'nodeInfo.1.datagroupname': true
            },
            'autoSort': { 'key': 'type', 'asc': false },
            'max': 50,
            'filter': {
               'type': [
                  { 'key': $scope.autoLanguage( '全部' ),   'value': '' },
                  { 'key': $scope.autoLanguage( '已同步' ), 'value': filterSync },
                  { 'key': $scope.autoLanguage( '变化' ),   'value': filterChange },
                  { 'key': $scope.autoLanguage( '无变化' ), 'value': 0 }
               ],
               'nodeInfo.1.hostName': 'indexof',
               'nodeInfo.1.svcname': 'indexof',
               'nodeInfo.1.dbpath': 'indexof',
               'nodeInfo.1.role': [
                  { 'key': $scope.autoLanguage( '全部' ), 'value': '' },
                  { 'key': 'coord', 'value': 'coord' },
                  { 'key': 'catalog', 'value': 'catalog' },
                  { 'key': 'data', 'value': 'data' }
               ],
               'nodeInfo.1.datagroupname': 'indexof'
            }
         },
         'callback': {}
      } ;
      
      //节点配置 弹窗
      $scope.NodeConfWindow = {
         'config': [],
         'callback': {}
      }

      //同步前的节点列表
      SdbSignal.on( "GetBeforeConf", function( result ){
         $.each( result, function( index, hostInfo ){
            $.each( hostInfo['Config'], function( index2, nodeInfo ){
               $scope.NodeTable['body'].push( { 'type': 0, 'nodeInfo': [ null, nodeInfo ] } ) ;
            } ) ;
         } ) ;
      } ) ;

      //判断同步后节点是否有修改
      var contrast = function( beforeNodeConf, afterNodeConf ){
         var type = 0 ;
         $.each( afterNodeConf, function( key, value ){
            if( afterNodeConf[key] != beforeNodeConf[key] )
            {
               type = 3 ;
               return false ;
            }
         } ) ;
         $.each( beforeNodeConf, function( key, value ){
            if( afterNodeConf[key] != beforeNodeConf[key] )
            {
               type = 3 ;
               return false ;
            }
         } ) ;
         return type ;
      }

      //两次配置获取完之后执行
      SdbSignal.on( 'GetFinishConf', function( result ){
         $scope.NodeTable['body'] = [] ;

         var beforeResult = result['BeforeNodeConf'] ;
         var afterResult  = result['AfterNodeConf'] ;

         //将已删除的节点传入表格body
         $.each( beforeResult, function( index, beforeNodeInfo ){
            var type = 1 ;
            $.each( afterResult, function( index2, afterNodeInfo ){
               if( afterNodeInfo['hostName'] == beforeNodeInfo['hostName'] &&
                   afterNodeInfo['svcname'] == beforeNodeInfo['svcname'] )
               {
                  type = 0 ;
                  return false ;
               }
            } ) ;
            if( type == 1 )
            {
               $scope.NodeTable['body'].push( { 'type': type, 'nodeInfo': [ null, beforeNodeInfo ] } ) ;
            }
         } ) ;

         //遍历判断节点是否修改
         $.each( afterResult, function( index, afterNodeInfo ){
            var type = 2 ;
            var beforeNodeInfo = null ;
            $.each( beforeResult, function( index2, nodeInfo ){
               if( afterNodeInfo['hostName'] == nodeInfo['hostName'] &&
                   afterNodeInfo['svcname'] == nodeInfo['svcname'] )
               {
                  type = contrast( nodeInfo, afterNodeInfo ) ;
                  beforeNodeInfo = nodeInfo ;
                  return false ;
               }
            } ) ;
            if( type == 2 )
            {
               //新增
               $scope.NodeTable['body'].push( { 'type': type, 'nodeInfo': [ null, afterNodeInfo ] } ) ;
            }
            else
            {
               //type:0 无变化; type:3 有变化
               $scope.NodeTable['body'].push( { 'type': type, 'nodeInfo': [ beforeNodeInfo, afterNodeInfo ] } ) ;
            }
         } ) ;
      } ) ;

      //打开 节点配置 弹窗
      $scope.ShowNodeConf = function( nodeInfo, type ){

         var configList = [] ;
         if( type == 3 )
         {
            //原节点的所有配置项
            var keyMap = {} ;
            $.each( nodeInfo[0], function( key, value ){
               keyMap[key] = configList.length ;
               configList.push( { 'type': 1, 'key': key, 'value': [ value, null ] } ) ;
            } ) ;
            $.each( keyMap, function( key, index ){
               if( typeof( nodeInfo[1][key] ) == 'undefined' )
               {
                  //删除的配置项
                  configList[index]['type'] = 1 ;
               }
               else
               {
                  if( nodeInfo[0][key] == nodeInfo[1][key] )
                  {
                     //无变化的配置项
                     configList[index]['type'] = 0 ;
                  }
                  else
                  {
                     //有变化的配置项
                     configList[index]['type'] = 3 ;
                  }
                  configList[index]['value'][1] = nodeInfo[1][key] ;
               }
            } ) ;
            $.each( nodeInfo[1], function( key, value ){
               if( isNaN( keyMap[key] ) == true )
               {
                  //新配置有的项
                  configList.push( { 'type': 2, 'key': key, 'value': [ null, value] } ) ;
               }
            } ) ;
         }
         else
         {
            $.each( nodeInfo[1], function( key, value ){
               configList.push( { 'type': 0, 'key': key, 'value': [ value, value ] } ) ;
            } ) ;
         }

         $scope.NodeConfWindow['config'] = {} ;
         $scope.NodeConfWindow['config']['type'] = type ;
         $scope.NodeConfWindow['config']['nodeInfo'] = configList

         $scope.NodeConfWindow['callback']['SetIcon']( '' ) ;
         $scope.NodeConfWindow['callback']['SetTitle']( $scope.autoLanguage( '节点配置' ) ) ;
         $scope.NodeConfWindow['callback']['Open']() ;
      }
   } ) ;
}());