//@ sourceURL=Index.js
(function(){
   var sacApp = window.SdbSacManagerModule ;
   //主控制器
   sacApp.controllerProvider.register( 'Deploy.Sdb.Discover.Ctrl', function( $scope, $location, $rootScope, SdbRest, SdbSignal ){
      $scope.ContainerBox = [ { offsetY: -106 }, { offsetY: -40 } ] ;
      $scope.ClusterName  = $rootScope.tempData( 'Deploy', 'ClusterName' ) ;
      $scope.ModuleName   = $rootScope.tempData( 'Deploy', 'ModuleName' ) ;
      $scope.DiscoverConf = $rootScope.tempData( 'Deploy', 'DiscoverConf' ) ;

      if( $scope.ModuleName == null || $scope.ClusterName == null )
      {
         $location.path( '/Transfer' ).search( { 'r': new Date().getTime() } ) ;
         return ;
      }

      //如果从安装主机进来的，有步骤条
      if( $scope.DiscoverConf != null )
      {
         $scope.stepList = _Deploy.BuildDiscoverStep( $scope, $location, $scope['Url']['Action'], 'sequoiadb' ) ;
      }

      //获取业务配置
      var getBusinessConf = function(){
         var data = { 'cmd': 'query node configure', 'filter': JSON.stringify( { 'ClusterName': $scope.ClusterName, 'BusinessName': $scope.ModuleName } ) } ;
         SdbRest.OmOperation( data, {
            'success': function( result ){
               $.each( result, function( index, hostInfo ){
                  $.each( hostInfo['Config'], function( index2, nodeInfo ){
                     nodeInfo['hostname'] = hostInfo['HostName'] ;
                  } ) ;
               } ) ;
               SdbSignal.commit( "GetBusinessConf", result ) ;
            },
            'failed': function( errorInfo ){
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  getBusinessConf() ;
                  return true ;
               } ) ;
            }
         } ) ;
      }

      getBusinessConf() ;

      //返回
      $scope.GotoDeploy = function(){
         $location.path( '/Deploy/Index' ).search( { 'r': new Date().getTime() } ) ;
      }

   } ) ;
   //左边概览信息控制器
   sacApp.controllerProvider.register( 'Deploy.Sdb.Discover.Preview.Ctrl', function( $scope, SdbSignal ){
      //业务配置
      $scope.BusinessConf = {
         'hostNum': 0,
         'nodeNum': 0,
         'dataGroupNum': 0,
         'dataNum': 0,
         'cataNum': 0,
         'coordNum': 0,
         'minReplicaNum': 0,
         'maxReplicaNum': 0,
         'moduleMode': '-'
      } ;
      SdbSignal.on( "GetBusinessConf", function( result ){
         if( result.length == 0 )
         {
            return ;
         }
         if( result[0]['DeployMod'] == 'standalone' )
         {
            $scope.BusinessConf['hostNum'] = 1 ;
            $scope.BusinessConf['nodeNum'] = 1 ;
            $scope.BusinessConf['dataGroupNum'] = '-' ;
            $scope.BusinessConf['dataNum'] = '-' ;
            $scope.BusinessConf['cataNum'] = '-' ;
            $scope.BusinessConf['coordNum'] = '-' ;
            $scope.BusinessConf['minReplicaNum'] = '-' ;
            $scope.BusinessConf['maxReplicaNum'] = '-' ;
            $scope.BusinessConf['moduleMode'] = result[0]['DeployMod'] ;
         }
         else
         {
            var replicaMax = 0 ;
            var replicaMin = 10 ;
            var tempGroupsArr = {} ;

            $scope.BusinessConf['moduleMode'] = result[0]['DeployMod'] ;
            $.each( result, function( index, hostInfo ){
               ++$scope.BusinessConf['hostNum'] ;
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
                     ++$scope.BusinessConf['dataNum'] ;
                  }
                  else if( nodeInfo['role'] == 'coord' )
                  {
                     ++$scope.BusinessConf['coordNum'] ;
                  }
                  else if( nodeInfo['role'] == 'catalog' )
                  {
                     ++$scope.BusinessConf['cataNum'] ;
                  }
                  else
                  {
                     return true ;
                  }
                  ++$scope.BusinessConf['nodeNum'] ;
               } ) ;            
            } ) ;
            $.each( tempGroupsArr, function( index, num ){
               ++$scope.BusinessConf['dataGroupNum'] ;
               if( num > replicaMax )
               {
                  replicaMax = num ;
               }
               if( num < replicaMin )
               {
                  replicaMin = num ;
               }
            } ) ;
            $scope.BusinessConf['minReplicaNum'] = replicaMin ;
            $scope.BusinessConf['maxReplicaNum'] = replicaMax ;
         }
      } ) ;
   } ) ;
   //右边表格
   sacApp.controllerProvider.register( 'Deploy.Sdb.Discover.Table.Ctrl', function( $scope, SdbSignal ){
      //节点列表
      $scope.NodeTable = {
         'title': {
            'hostname':       $scope.autoLanguage( '主机名' ),
            'svcname':        $scope.autoLanguage( '服务名' ),
            'dbpath':         $scope.autoLanguage( '数据路径' ),
            'role':           $scope.autoLanguage( '角色' ),
            'datagroupname':  $scope.autoLanguage( '分区组' )
         },
         'body': [],
         'options': {
            'width': {
               'hostname': '20%',
               'svcname': '10%',
               'dbpath': '36%',
               'role': '10%',
               'datagroupname': '12%'
            },
            'sort': {
               'hostname': true,
               'svcname': true,
               'dbpath': true,
               'role': true,
               'datagroupname': true
            },
            'max': 50,
            'filter': {
               'hostname': 'indexof',
               'svcname': 'indexof',
               'dbpath': 'indexof',
               'role': [
                  { 'key': $scope.autoLanguage( '全部' ), 'value': '' },
                  { 'key': 'coord', 'value': 'coord' },
                  { 'key': 'catalog', 'value': 'catalog' },
                  { 'key': 'data', 'value': 'data' }
               ],
               'datagroupname': 'indexof'
            }
         },
         'callback': {}
      } ;
      
      //节点配置 弹窗
      $scope.NodeConfWindow = {
         'config': [],
         'callback': {}
      }

      //节点列表
      SdbSignal.on( "GetBusinessConf", function( result ){
         $.each( result, function( index, hostInfo ){
            $.each( hostInfo['Config'], function( index2, nodeInfo ){
               $scope.NodeTable['body'].push( nodeInfo ) ;
            } ) ;
         } ) ;
      } ) ;

      //打开 节点配置 弹窗
      $scope.ShowNodeConf = function( nodeInfo ){
         $scope.NodeConfWindow['config'] = [] ;
         $scope.NodeConfWindow['config'].push( { 'key': 'nodename', 'value': nodeInfo['hostname'] + ':' + nodeInfo['svcname'] } ) ;
         $.each( nodeInfo, function( key, value ){
            $scope.NodeConfWindow['config'].push( { 'key': key, 'value': value } ) ;
         } ) ;
         $scope.NodeConfWindow['callback']['SetIcon']( '' ) ;
         $scope.NodeConfWindow['callback']['SetTitle']( $scope.autoLanguage( '节点配置' ) ) ;
         $scope.NodeConfWindow['callback']['Open']() ;
      }
   } ) ;
}());