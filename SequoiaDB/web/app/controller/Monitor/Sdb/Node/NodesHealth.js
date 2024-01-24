//@ sourceURL=NodesHealth.js
(function(){
   var sacApp = window.SdbSacManagerModule ;
   //控制器
   sacApp.controllerProvider.register( 'Monitor.SdbOverview.NodesHealth.Ctrl', function( $scope, $compile, $location, $timeout, $interval, SdbRest, SdbFunction ){
      
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
         'interval': 5,
         'play': false
      } ;
      //刷新状态
      $scope.RefreshType = $scope.autoLanguage( '启动刷新' ) ;
      //是否初始化
      var isBuildKeyList = false ;
      //节点列表的表格
      $scope.NodeTable = {
         'title': {
            'ServiceStatus':               'Status',
            'NodeName':                    'NodeName',
            'GroupName':                   'GroupName',
            'IsPrimary':                   'IsPrimary',
            'Role':                        'Role',
            'DataStatus':                  'DataStatus',
            'SyncControl':                 'SyncControl',
            'CompleteLSN':                 'CompleteLSN',
            'BeginLSN.Offset':             false,
            'BeginLSN.Version':            false,
            'CurrentLSN.Offset':           false,
            'CurrentLSN.Version':          false,
            'CommittedLSN.Offset':         false,
            'CommittedLSN.Version':        false,
            'Ulimit.CoreFileSize':         false,
            'Ulimit.VirtualMemory':        false,
            'Ulimit.OpenFiles':            false,
            'Ulimit.NumProc':              false,
            'Ulimit.FileSize':             false,
            'Ulimit.StackSize':            false,
            'ErrNum.SDB_OOM':              false,
            'ErrNum.SDB_NOSPC':            false,
            'ErrNum.SDB_TOO_MANY_OPEN_FD': false,
            'Memory.LoadPercent':          false,
            'Memory.TotalRAM':             false,
            'Memory.RssSize':              'Memory.RssSize',
            'Memory.LoadPercentVM':        false,
            'Memory.VMLimit':              false,
            'Memory.VMSize':               false,
            'Disk.Name':                   false,
            'Disk.LoadPercent':            false,
            'Disk.TotalSpace':             false,
            'Disk.FreeSpace':              'Disk.FreeSpace',
            'FileDesp.LoadPercent':        false,
            'FileDesp.TotalNum':           false,
            'FileDesp.FreeNum':            false,
            'DiffLSNWithPrimary':          'DiffLSNWithPrimary'
         },
         'body': [],
         'options': {
            'width': {
               'ServiceStatus':  '80px',
               'IsPrimary':      '70px',
               'Role':           '70px'
            },
            'sort': {
               'BeginLSN.Offset':             true,
               'BeginLSN.Version':            true,
               'CurrentLSN.Offset':           true,
               'CurrentLSN.Version':          true,
               'CommittedLSN.Offset':         true,
               'CommittedLSN.Version':        true,
               'Ulimit.CoreFileSize':         true,
               'Ulimit.VirtualMemory':        true,
               'Ulimit.OpenFiles':            true,
               'Ulimit.NumProc':              true,
               'Ulimit.FileSize':             true,
               'Ulimit.StackSize':            true,
               'ErrNum.SDB_OOM':              true,
               'ErrNum.SDB_NOSPC':            true,
               'ErrNum.SDB_TOO_MANY_OPEN_FD': true,
               'Memory.LoadPercent':          true,
               'Memory.LoadPercentVM':        true,
               'Disk.Name':                   true,
               'Disk.LoadPercent':            true,
               'FileDesp.LoadPercent':        true,
               'FileDesp.TotalNum':           true,
               'FileDesp.FreeNum':            true
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
                  { 'key': 'Coord', 'value': 'coord' },
                  { 'key': 'Catalog', 'value': 'catalog' },
                  { 'key': 'Data',    'value': 'data' }
               ],
               'CompleteLSN':                 'number',
               'BeginLSN.Offset':             'number',
               'BeginLSN.Version':            'number',
               'CurrentLSN.Offset':           'number',
               'CurrentLSN.Version':          'number',
               'CommittedLSN.Offset':         'number',
               'CommittedLSN.Version':        'number',
               'Ulimit.CoreFileSize':         'number',
               'Ulimit.VirtualMemory':        'number',
               'Ulimit.OpenFiles':            'number',
               'Ulimit.NumProc':              'number',
               'Ulimit.FileSize':             'number',
               'Ulimit.StackSize':            'number',
               'ErrNum.SDB_OOM':              'number',
               'ErrNum.SDB_NOSPC':            'number',
               'ErrNum.SDB_TOO_MANY_OPEN_FD': 'number',
               'Memory.LoadPercent':          'number',
               'Memory.TotalRAM':             'indexof',
               'Memory.RssSize':              'indexof',
               'Memory.LoadPercentVM':        'number',
               'Memory.VMLimit':              'indexof',
               'Memory.VMSize':               'indexof',
               'Disk.Name':                   'number',
               'Disk.LoadPercent':            'number',
               'Disk.TotalSpace':             'indexof',
               'Disk.FreeSpace':              'indexof',
               'FileDesp.LoadPercent':        'number',
               'FileDesp.TotalNum':           'number',
               'FileDesp.FreeNum':            'number'
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
      //显示列 下拉菜单
      $scope.FieldDropdown = {
         'config': [],
         'callback': {}
      } ;

      //获取节点列表
      var getNodesList = function( dbList ){
         var data = { 'cmd': 'list groups' } ;
         SdbRest.DataOperation( data, {
            'success': function( groups ){
               var nodesList = [] ;
               $scope.GroupList = groups ;
               $.each( groups, function( index, groupInfo ){
                  if( groupInfo['Role'] == 2 )
                  {
                     groupInfo['Role'] = 'catalog' ;
                  }
                  else if( groupInfo['Role'] == 1 )
                  {
                     groupInfo['Role'] = 'coord' ;
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
                  nodesList[index]['NodeName'] = nodeInfo['HostName'] + ':' + nodeInfo['ServiceName'] ;
                  nodesList[index]['i'] = index ;

                  //统计节点信息
                  $.each( dbList, function( index2, dbInfo ){
                     if( nodeInfo['NodeName'] == dbInfo['NodeName'] )
                     {
                        nodesList[index] = $.extend( nodesList[index], dbInfo ) ;
                        return false ;
                     }
                  } ) ;

                  if( !isUndefined( nodesList[index]['Memory'] ) )
                  {
                     //转换单位为字节的字段
                     nodesList[index]['Memory']['TotalRAM'] = sizeConvert2( nodesList[index]['Memory']['TotalRAM'] ) ; 
                     nodesList[index]['Memory']['RssSize'] = sizeConvert2( nodesList[index]['Memory']['RssSize'] ) ; 
                     nodesList[index]['Memory']['VMLimit'] = sizeConvert2( nodesList[index]['Memory']['VMLimit'] ) ; 
                     nodesList[index]['Memory']['VMSize'] = sizeConvert2( nodesList[index]['Memory']['VMSize'] ) ; 
                     nodesList[index]['Disk']['TotalSpace'] = sizeConvert2( nodesList[index]['Disk']['TotalSpace'] ) ; 
                     nodesList[index]['Disk']['FreeSpace'] = sizeConvert2( nodesList[index]['Disk']['FreeSpace'] ) ; 
                  }


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
                              return false ;
                           }
                        } ) ;
                     }
                  } ) ;
               } ) ;
               $scope.NodeTable['body'] = nodesList ;
            
               //第一次创建字段列表
               if( isBuildKeyList == false )
               {
                  isBuildKeyList = true ;
                  if( $scope.NodeTable['body'].length > 0 )
                  {
                     //寻找一个运行正常的节点
                     $.each( $scope.NodeTable['body'], function( normalIndex, nodeInfo ){
                        if( isNaN( nodeInfo['Flag'] ) == true || nodeInfo['Flag'] == 0 )
                        {
                           //构造表格字段
                           $.each( nodeInfo, function( key, val ){
                              //过滤一些不需要显示的字段
                              if( key == 'i' ||
                                  key == 'ServiceName' ||
                                  key == 'BeginLSN' ||
                                  key == 'CurrentLSN' ||
                                  key == 'CommittedLSN' ||
                                  key == 'ErrNum' ||
                                  key == 'Ulimit' ||
                                  key == 'Memory' ||
                                  key == 'Disk' ||
                                  key == 'FileDesp' ||
                                  key == 'TransInfo' )
                              {
                                 return true ;
                              }

                              //构造表格标题
                              if( isUndefined( $scope.NodeTable['title'][key] ) )
                              {
                                 $scope.NodeTable['title'][key] = false ;
                              }

                              //构造表格排序字段
                               $scope.NodeTable['options']['sort'][key] = true ;

                              //构造表格过滤字段
                              if( isUndefined( $scope.NodeTable['options']['filter'][key] ) )
                              {
                                 if( isNumber( val ) == 'number' )
                                 {
                                    $scope.NodeTable['options']['filter'][key] = 'number' ;
                                 }
                                 else
                                 {
                                    $scope.NodeTable['options']['filter'][key] = 'indexof' ;
                                 }
                              }
                           } ) ;
                           //构造选择列显示的下拉菜单
                           $.each( $scope.NodeTable['title'], function( key ){
                              if( key == 'ServiceStatus' || key == 'Status' )
                                 return true ;
                              $scope.FieldDropdown['config'].push( { 'key': key, 'show': isString( $scope.NodeTable['title'][key] ) } ) ;
                           } ) ;
                           return false ;
                        }
                     } ) ;
                  }
               }
               if( $scope.Timer['callback']['GetStatus']() == 'start' || $scope.Timer['callback']['GetStatus']() == 'complete' ) //如果开了定时器，就开始
               {
                  $scope.Timer['callback']['Complete']() ;
               }

            },
            'failed': function( errorInfo ){
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  getNodesList( dbList ) ;
                  return true ;
               } ) ;
            }
         }, {
            'showLoading': false
         } ) ;
      }

      //获取DB快照
      var getDbList = function(){
         var sql  = 'SELECT * FROM $SNAPSHOT_HEALTH' ;
         SdbRest.Exec( sql, {
            'success': function( dbList ){
               getNodesList( dbList ) ;
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

      //打开 显示列 的下拉菜单
      $scope.OpenShowFieldDropdown = function( event ){
         $.each( $scope.FieldDropdown['config'], function( index, fieldInfo ){
            $scope.FieldDropdown['config'][index]['show'] = isString( $scope.NodeTable['title'][fieldInfo['key']] ) ;
         } ) ;
         $scope.FieldDropdown['callback']['Open']( event.currentTarget ) ;
      }

      //保存 显示列
      $scope.SaveField = function(){
         $.each( $scope.FieldDropdown['config'], function( index, fieldInfo ){
            $scope.NodeTable['title'][fieldInfo['key']] = fieldInfo['show'] ? fieldInfo['key'] : false ;
         } ) ;
         $scope.NodeTable['callback']['ShowCurrentPage']() ;
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
               $scope.IntervalTimeConfig = formVal ;
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
            $scope.Timer['callback']['Start']( getDbList )
         }
      }

       //跳转至资源
      $scope.GotoResources = function(){
         $location.path( '/Monitor/SDB-Resources/Session' ).search( { 'r': new Date().getTime() } ) ;
      } ;

      //跳转至主机列表
      $scope.GotoHostList = function(){
         $location.path( '/Monitor/SDB-Host/List/Index' ).search( { 'r': new Date().getTime() } ) ;
      } ;
      
      
      //跳转至节点列表
      $scope.GotoNodes = function(){
         $location.path( '/Monitor/SDB-Nodes/Nodes' ).search( { 'r': new Date().getTime() } ) ;
      } ;
    
      //跳转至节点信息
      $scope.GotoNode = function( HostName, ServiceName ){
         SdbFunction.LocalData( 'SdbHostName', HostName ) ;
         SdbFunction.LocalData( 'SdbServiceName', ServiceName ) ;
         $location.path( '/Monitor/SDB-Nodes/Node/Index' ).search( { 'r': new Date().getTime() } ) ;
      } ;

      //跳转至分区组信息
      $scope.GotoGroup = function( GroupName ){
         SdbFunction.LocalData( 'SdbGroupName', GroupName ) ;
         $location.path( '/Monitor/SDB-Nodes/Group/Index' ).search( { 'r': new Date().getTime() } ) ;
      } ;

      //跳转至主机信息
      $scope.GotoHostInfo = function( HostName ){
         SdbFunction.LocalData( 'SdbHostName', HostName ) ;
         $location.path( '/Monitor/SDB-Host/Info/Index' ).search( { 'r': new Date().getTime() } ) ;
      } ;

      //跳转至数据库操作
      $scope.GotoDatabase = function(){
         $location.path( '/Data/SDB-Database/Index' ).search( { 'r': new Date().getTime() } ) ;
      } ;

   } ) ;
}());