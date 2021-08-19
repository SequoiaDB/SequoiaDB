//@ sourceURL=Conf.js
"use strict" ;
(function(){
   var sacApp = window.SdbSacManagerModule ;
   //主控制器
   sacApp.controllerProvider.register( 'Deploy.Sdb.ShrinkConf.Ctrl', function( $scope, $location, $rootScope, SdbRest, SdbSwap, SdbSignal ){
      var clusterName   = $rootScope.tempData( 'Deploy', 'ClusterName' ) ;
      var shrink        = $rootScope.tempData( 'Deploy', 'Shrink' ) ;
      $scope.ModuleName = $rootScope.tempData( 'Deploy', 'ModuleName' ) ;
      if( $scope.ModuleName == null || clusterName == null || shrink != true )
      {
         $location.path( '/Transfer' ).search( { 'r': new Date().getTime() } ) ;
         return ;
      }

      $scope.stepList = _Deploy.BuildSdbShrinkStep( $scope, $location, $scope['Url']['Action'], 'sequoiadb' ) ;
      if( $scope.stepList['info'].length == 0 )
      {
         $location.path( '/Deploy/Index' ).search( { 'r': new Date().getTime() } ) ;
         return ;
      }

      SdbSwap.nodeList = [] ;

      //获取业务配置
      var getBusinessConf = function(){
         var data = { 'cmd': 'query node configure', 'filter': JSON.stringify( { 'ClusterName': clusterName, 'BusinessName': $scope.ModuleName } ) } ;
         SdbRest.OmOperation( data, {
            'success': function( result ){

               var tempBusinessConfig = [] ;
               $.each( result, function( index, hostInfo ){
                  $.each( hostInfo['Config'], function( index2, nodeInfo ){
                     nodeInfo['hostname'] = hostInfo['HostName'] ;
                     nodeInfo['checked'] = false ;
                     tempBusinessConfig.push( nodeInfo ) ;
                  } ) ;
               } ) ;

               SdbSignal.commit( "GetNodeList", tempBusinessConfig ) ;
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

      //下一步
      $scope.GotoNext = function(){
         var config = [] ;
         $.each( SdbSwap.nodeList, function( index, nodeInfo ){
            if( nodeInfo['checked'] == true )
            {
               config.push( { 'HostName': nodeInfo['hostname'], 'svcname': nodeInfo['svcname'] } ) ;
            }
         } ) ;
         //没有选择节点提示
         if( config.length == 0 )
         {
            $scope.Components.Confirm.type = 3 ;
            $scope.Components.Confirm.context = $scope.autoLanguage( '没有减容节点，请修改减容配置。' ) ;
            $scope.Components.Confirm.isShow = true ;
            $scope.Components.Confirm.okText = $scope.autoLanguage( '确定' ) ;
            $scope.Components.Confirm.ok = function(){
               $scope.Components.Confirm.isShow = false ;
            }
            return ;
         }
         var data = {
            'cmd': 'shrink business',
            'ConfigInfo': JSON.stringify( { 'ClusterName': clusterName, 'BusinessName': $scope.ModuleName, 'Config': config } )
         } ;
         SdbRest.OmOperation( data, {
            'success': function( taskInfo ){
               $rootScope.tempData( 'Deploy', 'ModuleTaskID', taskInfo[0]['TaskID'] ) ;
               $location.path( '/Deploy/Task/Module' ).search( { 'r': new Date().getTime() } ) ;
            },
            'failed': function( errorInfo ){
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  $scope.GotoNext() ;
                  return true ;
               } ) ;
            }
         } ) ;
      }
   } ) ;

   //左边分区组列表 控制器
   sacApp.controllerProvider.register( 'Deploy.Sdb.Shrink.GroupList.Ctrl', function( $scope, SdbSwap, SdbSignal ){
      $scope.GroupList = {} ;
      SdbSignal.on( "GetNodeList", function( result ){
         //统计分区组和节点数
         $.each( result, function( index, nodeInfo ){
            var groupname = '' ;
            if( nodeInfo['role'] == 'coord' || nodeInfo['role'] == 'catalog' )
            {
               groupname = nodeInfo['role'] ;
            }
            else
            {
               groupname = nodeInfo['datagroupname'] ;
            }
            if( typeof( $scope.GroupList[groupname] ) == 'undefined' )
            {
               $scope.GroupList[groupname] = {
                  'groupName': groupname,
                  'nodeNum': 1
               }
            }
            else
            {
               ++$scope.GroupList[groupname]['nodeNum'] ;
            }
         } ) ;
      } ) ;
      
      //选中分区组
      $scope.CheckedGroup = function( groupName ){
         SdbSignal.commit( "checkedGroup", groupName ) ;
      }

   } ) ;

   //右边表格
   sacApp.controllerProvider.register( 'Deploy.Sdb.Shrink.Table.Ctrl', function( $scope, SdbSwap, SdbSignal ){
      //节点列表
      $scope.NodeTable = {
         'title': {
            'checked':        '',
            'hostname':       $scope.autoLanguage( '主机名' ),
            'svcname':        $scope.autoLanguage( '服务名' ),
            'dbpath':         $scope.autoLanguage( '数据路径' ),
            'role':           $scope.autoLanguage( '角色' ),
            'datagroupname':  $scope.autoLanguage( '分区组' )
         },
         'body': [],
         'options': {
            'width': {
               'checked':       '18px',
               'hostname':      '20%',
               'svcname':       '10%',
               'dbpath':        '36%',
               'role':          '10%',
               'datagroupname': '12%'
            },
            'sort': {
               'checked':      false,
               'hostname':      true,
               'svcname':       true,
               'dbpath':        true,
               'role':          true,
               'datagroupname': true
            },
            'autoSort': { 'key': 'role', 'asc': true },
            'max': 30,
            'filter': {
               'hostname': 'indexof',
               'svcname':  'indexof',
               'dbpath':   'indexof',
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

      //节点列表
      SdbSignal.on( "GetNodeList", function( result ){
         $scope.NodeTable['body'] = result ;
         SdbSwap.nodeList = $scope.NodeTable['body'] ;
      } ) ;

      //全选或全不选
      SdbSignal.on( "checkedGroup", function( groupName ){
         var sumNode = 0 ;
         var checkedNode = 0 ;
         var checked = true ;
         $.each( $scope.NodeTable['body'], function( index, nodeInfo ){
            if( ( groupName == 'catalog' || groupName == 'coord' ) && nodeInfo['role'] == groupName )
            {
               ++sumNode ;
               if( nodeInfo['checked'] == true )
               {
                  ++checkedNode ;
               }
            }
            else if( nodeInfo['datagroupname'] == groupName )
            {
               ++sumNode ;
               if( nodeInfo['checked'] == true )
               {
                  ++checkedNode ;
               }
            }
            else
            {
               return ;
            }
         } ) ;

         checked = sumNode == checkedNode ? false : true ;

         $.each( $scope.NodeTable['body'], function( index, nodeInfo ){
            if( ( groupName == 'catalog' || groupName == 'coord' ) && nodeInfo['role'] == groupName )
            {
               nodeInfo['checked'] = checked ; 
            }
            else if( nodeInfo['datagroupname'] == groupName )
            {
               nodeInfo['checked'] = checked ; 
            }
            else
            {
               return ;
            }
         } ) ;

      } ) ;

      //节点配置 弹窗
      $scope.NodeConfWindow = {
         'config': [],
         'callback': {}
      }

      //打开 节点配置 弹窗
      $scope.ShowNodeConf = function( nodeInfo ){
         $scope.NodeConfWindow['config'] = [] ;
         $scope.NodeConfWindow['config'].push( { 'key': 'nodename', 'value': nodeInfo['hostname'] + ':' + nodeInfo['svcname'] } ) ;
         $.each( nodeInfo, function( key, value ){
            if( key == 'checked' )
            {
               return ;
            }
            $scope.NodeConfWindow['config'].push( { 'key': key, 'value': value } ) ;
         } ) ;
         $scope.NodeConfWindow['callback']['SetIcon']( '' ) ;
         $scope.NodeConfWindow['callback']['SetTitle']( $scope.autoLanguage( '节点配置' ) ) ;
         $scope.NodeConfWindow['callback']['SetCloseButton']( $scope.autoLanguage( '关闭' ), function(){
            $scope.NodeConfWindow['callback']['Close']() ;
         } ) ;
         $scope.NodeConfWindow['callback']['Open']() ;
      }

      
   } ) ;
}());