//@ sourceURL=NodesSync.js
(function(){
   var sacApp = window.SdbSacManagerModule ;
   //控制器
   sacApp.controllerProvider.register( 'Monitor.SdbOverview.NodesSync.Ctrl', function( $scope, $compile, $location, $timeout, $interval, SdbRest, SdbFunction ){
      
      var clusterName = SdbFunction.LocalData( 'SdbClusterName' ) ;
      var moduleType = SdbFunction.LocalData( 'SdbModuleType' ) ;
      var moduleMode = SdbFunction.LocalData( 'SdbModuleMode' ) ;
      var moduleName = SdbFunction.LocalData( 'SdbModuleName' ) ;
      if( clusterName == null || moduleType != 'sequoiadb' || moduleMode == null || moduleName == null )
      {
         $location.path( '/Transfer' ).search( { 'r': new Date().getTime() } ) ;
         return;
      }

      if( moduleMode != 'distribution' )  //只支持集群模式
      {
         $location.path( '/Monitor/SDB/Index' ).search( { 'r': new Date().getTime() } ) ;
         return ;
      }

      //初始化
      $scope.IntervalTimeConfig = {
         'interval': 3,
         'beginRun': true,
         'play': true
      } ;
      //刷新状态
      $scope.RefreshType = $scope.autoLanguage( '停止刷新' ) ;
      //显示全部或者只显示数据不同步的节点
      var selector = 'different' ;
      //上一次的值
      $scope.LastValue = [] ;
      //节点列表的表格
      $scope.NodeTable = {
         'title': {
            'ServiceStatus':           'Status',
            'NodeName':                'NodeName',
            'HostName':                'HostName',
            'GroupName':               'GroupName',
            'IsPrimary':               'IsPrimary',
            'Role':                    'Role',
            'TotalRecords':            'TotalRecords',
            'TotalLobs':               'TotalLobs',
            'CompleteLSN':             'CompleteLSN'
         },
         'body': [],
         'options': {
            'width': {
               'ServiceStatus':  '80px',
               'IsPrimary':      '100px',
               'Role':           '70px'
            },
            'sort': {
               'ServiceStatus':           true,
               'NodeName':                true,
               'HostName':                true,
               'GroupName':               true,
               'IsPrimary':               true,
               'Role':                    true,
               'TotalRecords':            true,
               'TotalLobs':               true,
               'CompleteLSN':             true
            },
            'max': 50,
            'filter': {
               'ServiceStatus': [
                  { 'key': $scope.autoLanguage( '全部' ), 'value': '' },
                  { 'key': $scope.autoLanguage( '正常' ), 'value': true },
                  { 'key': $scope.autoLanguage( '异常' ), 'value': false }
               ],
               'IsPrimary': [
                  { 'key': $scope.autoLanguage( '全部' ), 'value': '' },
                  { 'key': 'true', 'value': true },
                  { 'key': 'false', 'value': false }
               ],
               'Role': [
                  { 'key': $scope.autoLanguage( '全部' ), 'value': '' },
                  { 'key': 'Catalog', 'value': 'catalog' },
                  { 'key': 'Data',    'value': 'data' }
               ],
               'NodeName':                'indexof',
               'HostName':                'indexof',
               'GroupName':               'indexof',
               'TotalRecords':            'number',
               'TotalLobs':               'number',
               'CompleteLSN':             'number'
            }
         },
         'callback': {}
      } ;
      //定时器
      $scope.Timer = {
         'config': $scope.IntervalTimeConfig,
         'callback': {}
      } ;
      //实时刷新设置 弹窗
      $scope.CreateBrush = {
         'config': {},
         'callback': {}
      } ;

      //获取节点列表
      var getNodesList = function( dbList, clList ){
         var data = { 'cmd': 'list groups' } ;
         SdbRest.DataOperation( data, {
            'success': function( groups ){
               var nodesList = [] ;
               var isPrimaryList = [] ;
               var noPrimaryList = [] ;
               $.each( groups, function( index, groupInfo ){
                  if( groupInfo['Role'] == 2 )
                  {
                     groupInfo['Role'] = 'catalog' ;
                  }
                  else if( groupInfo['Role'] == 1 )
                  {
                     return true ;
                  }
                  else if( groupInfo['Role'] == 0 )
                  {
                     groupInfo['Role'] = 'data' ;
                  }
                  $.each( groupInfo['Group'], function( index2, nodeInfo ){
                     nodesList.push( { 'HostName': nodeInfo['HostName'], 'ServiceName': nodeInfo['Service']['0']['Name'], 'GroupName': groupInfo['GroupName'], 'Role': groupInfo['Role']  } )
                  } ) ;
               } ) ;


               $.each( nodesList, function( index, nodeInfo ){
                  nodesList[index]['ServiceStatus'] = true ;
                  nodesList[index]['Status'] = 'Normal' ;
                  nodesList[index]['NodeName'] = nodeInfo['HostName'] + ':' + nodeInfo['ServiceName'] ;
                  nodesList[index]['IsPrimary'] = false ;
                  nodesList[index]['TotalRecords'] = 0 ;
                  nodesList[index]['TotalLobs'] = 0 ;
                  nodesList[index]['i'] = index ;

                  //统计节点信息
                  $.each( dbList, function( index2, dbInfo ){
                     if( nodeInfo['NodeName'] == dbInfo['NodeName'] )
                     {
                        nodesList[index] = $.extend( nodesList[index], dbInfo ) ;
                        return false ;
                     }
                  } ) ;

                  //统计节点的记录
                  $.each( clList, function( index3, clInfo ){
                     if( nodesList[index]['NodeName'] == clInfo['NodeName'] )
                     {
                        nodesList[index]['TotalRecords'] += clInfo['TotalRecords'] ;
                     }
                  } ) ;

                  //如果节点错误，那么数据为空
                  $.each( dbList, function( index3, dbInfo ){
                     if( isArray( dbInfo['ErrNodes'] ) )
                     {
                        $.each( dbInfo['ErrNodes'], function( nodeIndex, errNodeInfo ){
                           if( nodesList[index]['NodeName'] == errNodeInfo['NodeName'] )
                           {
                              nodesList[index]['ServiceStatus'] = false ;
                              nodesList[index]['Status'] = '' ;
                              nodesList[index]['Flag'] = errNodeInfo['Flag'] ;
                              nodesList[index]['TotalRecords'] = '-' ;
                              nodesList[index]['TotalLobs'] = '-' ;
                              nodesList[index]['CompleteLSN'] = '-' ;
                              return false ;
                           }
                        } ) ;
                     }
                  } ) ;
                  if( nodesList[index]['IsPrimary'] == true )
                  {
                     isPrimaryList.push( nodesList[index] ) ;
                  }
                  else if( nodesList[index]['IsPrimary'] == false )
                  {
                     noPrimaryList.push( nodesList[index] ) ;
                  }
               } ) ;
               if( selector == 'different' )
               {
                  nodesList = [] ;
                  $.each( isPrimaryList, function( nodeIndex, nodeInfo ){
                     $.each( noPrimaryList, function( nodeIndex2, nodeInfo2 ){
                        if( nodeInfo['GroupName'] == nodeInfo2['GroupName'] && nodeInfo['CompleteLSN'] != nodeInfo2['CompleteLSN'] )
                        {
                           if( nodeInfo['show'] != false )
                           {
                              nodesList.push( nodeInfo ) ;
                           }
                           nodeInfo['show'] = false ;
                           nodesList.push( nodeInfo2 ) ;
                           return ;
                        }
                     } ) ;
                  } ) ;
               }

               $scope.LastValue = $scope.NodeTable['body'] ;
               $scope.NodeTable['body'] = nodesList ;

               if( $scope.IntervalTimeConfig['play'] == true )
               {
                  $scope.Timer['callback']['Start']( getDbList ) ;
               }


            },
            'failed': function( errorInfo ){
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  getNodesList( dbList, clList ) ;
                  return true ;
               } ) ;
            }
         },{
            'showLoading': false
         } ) ;
      }

      //获取CL快照
      var getClList = function( dbList ){
         var sql = 'SELECT t1.Name, t1.Details.TotalRecords, t1.Details.NodeName from (SELECT Name, Details FROM $SNAPSHOT_CL WHERE Global = true split By Details) As t1' ;
         SdbRest.Exec( sql, {
            'success': function( clList ){
               getNodesList( dbList, clList ) ;
            },
            'failed': function( errorInfo ){
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  getClList( dbList ) ;
                  return true ;
               } ) ;
            }
         }, { 'showLoading': false } ) ;
      }

      //获取DB快照
      var getDbList = function(){
         var sql  = 'SELECT NodeName, HostName, ServiceName, GroupName, IsPrimary, CompleteLSN FROM $SNAPSHOT_DB' ;
         SdbRest.Exec( sql, {
            'success': function( dbList ){
               getClList( dbList ) ;
            },
            'failed': function( errorInfo ){
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  getDbList() ;
                  return true ;
               } ) ;
            }
         }, { 'showLoading': false } ) ;
      }

      getDbList() ;

      
      //选择全部 or 数据不同步 下拉菜单
      $scope.SelectorMenu = {
         'config': [
            { 'field': $scope.autoLanguage( '全部节点' ), 'checked': false, 'type': "all" },
            { 'field': $scope.autoLanguage( '数据不同步节点' ), 'checked': true, 'type': "different" }
         ],
         'OnClick': function( index ){
            $.each( $scope.SelectorMenu['config'], function( index2, config ){
               $scope.SelectorMenu['config'][index2]['checked'] = false ;
            } ) ;
            selector = $scope.SelectorMenu['config'][index]['type'] ;
            $scope.SelectorMenu['config'][index]['checked'] = true ;
            $scope.SelectorMenu['callback']['Close']() ;
            getDbList() ;
         },
         'callback': {}
      } ;

      //打开 选择显示节点 弹窗
      $scope.ShowSelectorMenu = function( event ){
         $scope.SelectorMenu['callback']['Open']( event.currentTarget ) ;
      }


      //打开 实时刷新 弹窗
      $scope.OpenBrushWindows = function(){
         //表单参数
         var brushForm = {
            'inputList': [
               {
                  "name": "interval",
                  "webName": $scope.autoLanguage( '刷新间距(秒)' ),
                  "type": "int",
                  "value": $scope.Timer['callback']['GetInterval'](),
                  "valid": {
                     'min': 1
                  }
               }
            ]
         } ;
         $scope.CreateBrush['config'] = brushForm ;
         //设置确定按钮
         $scope.CreateBrush['callback']['SetOkButton']( $scope.autoLanguage( '确定' ), function(){
            var isAllClear = brushForm.check() ;
            if( isAllClear )
            {
               var formVal = brushForm.getValue() ;
               $scope.IntervalTimeConfig['interval'] = formVal['interval'] ;
               $scope.Timer['callback']['SetInterval']( formVal['interval'] ) ;
            }
            return isAllClear ;
         } ) ;
         //设置标题
         $scope.CreateBrush['callback']['SetTitle']( $scope.autoLanguage( '实时刷新设置' ) ) ;
         //设置图标
         $scope.CreateBrush['callback']['SetIcon']( '' ) ;
         //打开窗口
         $scope.CreateBrush['callback']['Open']() ;
      }

      $scope.RefreshCtrl = function(){
         if( $scope.IntervalTimeConfig['play'] == true )
         {
            $scope.IntervalTimeConfig['play'] = false ; 
            $scope.RefreshType = $scope.autoLanguage( '启动刷新' )
            $scope.Timer['callback']['Stop']() ;
         }
         else
         {
            $scope.IntervalTimeConfig['play'] = true ; 
            $scope.RefreshType = $scope.autoLanguage( '停止刷新' ) ;
            getDbList() ;
            //$scope.Timer['callback']['Start']( getDbList ) ;
         }
      }
       //跳转至资源
      $scope.GotoResources = function(){
         $location.path( '/Monitor/SDB-Resources/Session' ).search( { 'r': new Date().getTime() } ) ;
      }

      //跳转至主机列表
      $scope.GotoHostList = function(){
         $location.path( '/Monitor/SDB-Host/List/Index' ).search( { 'r': new Date().getTime() } ) ;
      }
      
      
      //跳转至节点列表
      $scope.GotoNodes = function(){
         $location.path( '/Monitor/SDB-Nodes/Nodes' ).search( { 'r': new Date().getTime() } ) ;
      }
    
      //跳转至节点信息
      $scope.GotoNode = function( HostName, ServiceName ){
         SdbFunction.LocalData( 'SdbHostName', HostName ) ;
         SdbFunction.LocalData( 'SdbServiceName', ServiceName ) ;
         $location.path( '/Monitor/SDB-Nodes/Node/Index' ).search( { 'r': new Date().getTime() } ) ;
      }

      //跳转至分区组信息
      $scope.GotoGroup = function( GroupName ){
         SdbFunction.LocalData( 'SdbGroupName', GroupName ) ;
         $location.path( '/Monitor/SDB-Nodes/Group/Index' ).search( { 'r': new Date().getTime() } ) ;
      }

      //跳转至主机信息
      $scope.GotoHostInfo = function( HostName ){
         SdbFunction.LocalData( 'SdbHostName', HostName ) ;
         $location.path( '/Monitor/SDB-Host/Info/Index' ).search( { 'r': new Date().getTime() } ) ;
      }

      //跳转至数据库操作
      $scope.GotoDatabase = function(){
         $location.path( '/Data/SDB-Database/Index' ).search( { 'r': new Date().getTime() } ) ;
      }

   } ) ;
}());