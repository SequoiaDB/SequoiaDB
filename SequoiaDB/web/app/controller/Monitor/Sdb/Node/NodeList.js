//@ sourceURL=NodeList.js
(function(){
   var sacApp = window.SdbSacManagerModule ;
   //控制器
   sacApp.controllerProvider.register( 'Monitor.SdbOverview.Node.Ctrl', function( $scope, $compile, $location, $rootScope, SdbRest, SdbFunction ){
      
      var clusterName = SdbFunction.LocalData( 'SdbClusterName' ) ;
      var moduleType = SdbFunction.LocalData( 'SdbModuleType' ) ;
      var moduleMode = SdbFunction.LocalData( 'SdbModuleMode' ) ;
      var moduleName = SdbFunction.LocalData( 'SdbModuleName' ) ;
      if( clusterName == null || moduleType != 'sequoiadb' || moduleMode == null || moduleName == null )
      {
         $location.path( '/Transfer' ).search( { 'r': new Date().getTime() } ) ;
         return ;
      }

      if( moduleMode != 'distribution' )  //只支持集群模式
      {
         $location.path( '/Monitor/SDB/Index' ).search( { 'r': new Date().getTime() } ) ;
         return ;
      }

      //初始化
      //节点列表的表格
      $scope.NodeTable = {
         'title': {
            'Status':       $scope.autoLanguage( '状态' ),
            'NodeName':     $scope.autoLanguage( '节点名' ),
            'HostName':     $scope.autoLanguage( '主机名' ),
            'GroupName':    $scope.autoLanguage( '分区组' ),
            'IsPrimary':    $scope.autoLanguage( '主节点' ),
            'Role':         $scope.autoLanguage( '角色' ),
            'TotalCL':      $scope.autoLanguage( '集合数' ),
            'TotalRecords': $scope.autoLanguage( '记录数' ),
            'TotalLobs':    $scope.autoLanguage( 'Lob数' ),
            'LSN':          false
         },
         'body': [],
         'options': {
            'width': {
               'Status':      '80px',
               'IsPrimary':   '70px',
               'Role':        '70px'
            },
            'sort': {
               'Status':       true,
               'NodeName':     true,
               'HostName':     true,
               'GroupName':    true,
               'IsPrimary':    true,
               'Role':         true,
               'TotalCL':      true,
               'TotalRecords': true,
               'TotalLobs':    true,
               'LSN':          false
            },
            'max': 50,
            'filter': {
               'Status': [
                  { 'key': $scope.autoLanguage( '全部' ), 'value': '' },
                  { 'key': $scope.autoLanguage( '正常' ), 'value': true },
                  { 'key': $scope.autoLanguage( '异常' ), 'value': false }
               ],
               'NodeName':     'indexof',
               'HostName':     'indexof',
               'GroupName':    'indexof',
               'IsPrimary': [
                  { 'key': $scope.autoLanguage( '全部' ), 'value': '' },
                  { 'key': 'true', 'value': true },
                  { 'key': 'false', 'value': false }
               ],
               'Role': [
                  { 'key': $scope.autoLanguage( '全部' ), 'value': '' },
                  { 'key': 'Coord',   'value': 'coord' },
                  { 'key': 'Catalog', 'value': 'catalog' },
                  { 'key': 'Data',    'value': 'data' }
               ],
               'TotalCL':      'number',
               'TotalRecords': 'number',
               'TotalLobs':    'number'
            }
         }
      } ;
      //集合列表
      var clList = [] ;
      //启动节点的窗口
      $scope.StartNode = {
         'config': {
            'select': [],
            'value': ''
         },
         'callback': {}
      } ;
      //停止节点的窗口
      $scope.StopNode = {
         'config': {
            'select': [],
            'value': ''
         },
         'callback': {}
      } ;

      //启动节点
      var startNode = function( hostname, svcname ){
         var data = { 'cmd': 'start node', 'HostName': hostname, 'svcname': svcname } ;
         SdbRest.DataOperation( data, {
            'success': function(){
               getClList() ;
            },
            'failed': function( errorInfo ){
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  startNode( hostname, svcname ) ;
                  return true ;
               } ) ;
            }
         } ) ;
      }

      //停止节点
      var stopNode = function( hostname, svcname ){
         var data = { 'cmd': 'stop node', 'HostName': hostname, 'svcname': svcname } ;
         SdbRest.DataOperation( data, {
            'success': function(){
               getClList() ;
            },
            'failed': function( errorInfo ){
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  stopNode( hostname, svcname ) ;
                  return true ;
               } ) ;
            }
         } ) ;
      }

      //打开 启动节点 窗口
      $scope.OpenStartNodeWindow = function(){
         if( $scope.StartNode['config']['select'].length > 0 )
         {
            //设置确定按钮
            $scope.StartNode['callback']['SetOkButton']( $scope.autoLanguage( '确定' ), function(){
               var hostname = $scope.StartNode['config']['value'].split( ':' ) ;
               var svcname  = hostname[1] ;
               hostname = hostname[0] ;
               startNode( hostname, svcname ) ;
               return true ;
            } ) ;
            //关闭窗口滚动条
            $scope.StartNode['callback']['DisableBodyScroll']() ;
            //设置标题
            $scope.StartNode['callback']['SetTitle']( $scope.autoLanguage( '启动节点' ) ) ;
            $scope.StartNode['callback']['Open']() ;
         }
      }

      //打开 停止节点 窗口
      $scope.OpenStopNodeWindow = function(){
         if( $scope.StopNode['config']['select'].length > 0 )
         {
            //设置确定按钮
            $scope.StopNode['callback']['SetOkButton']( $scope.autoLanguage( '确定' ), function(){
               var hostname = $scope.StopNode['config']['value'].split( ':' ) ;
               var svcname  = hostname[1] ;
               hostname = hostname[0] ;
               stopNode( hostname, svcname ) ;
               return true ;
            } ) ;
            //关闭窗口滚动条
            $scope.StopNode['callback']['DisableBodyScroll']() ;
            //设置标题
            $scope.StopNode['callback']['SetTitle']( $scope.autoLanguage( '停止节点' ) ) ;
            $scope.StopNode['callback']['Open']() ;
         }
      }

      //获取coord节点状态
      var getCoordStatus = function( nodesInfo, hostname, svcname ){
         var sql = 'SELECT NodeName FROM $SNAPSHOT_DB WHERE GLOBAL=false' ;
         SdbRest.Exec( sql, {
            'before': function( jqXHR ){
               jqXHR.setRequestHeader( 'SdbHostName', hostname ) ;
               jqXHR.setRequestHeader( 'SdbServiceName', svcname ) ;
            },
            'success': function( dbInfo ){
               if( dbInfo.length > 0 )
               {
                  dbInfo = dbInfo[0] ;
                  if( dbInfo['NodeName'] != hostname + ':' + svcname ) //om虽然连接成功，但是已经切换到其他coord节点了，所以还是失败的
                  {
                     nodesInfo['Status'] = false ;
                     nodesInfo['Flag'] = -15 ;
                     nodesInfo['TotalRecords'] = '-' ;
                     nodesInfo['TotalLobs'] = '-' ;
                     nodesInfo['TotalCL'] = '-' ;
                     $scope.StartNode['config']['select'].push( {
                        'key': nodesInfo['Role'] == 'data' ? ( nodesInfo['GroupName'] + ' ' + nodesInfo['NodeName'] ) : ( nodesInfo['Role'] + ' ' + nodesInfo['NodeName'] ),
                        'value': nodesInfo['NodeName']
                     } ) ;
                     if( $scope.StartNode['config']['select'].length == 1 )
                     {
                        $scope.StartNode['config']['value'] = hostname + ':' + svcname ;
                     }
                  }
                  else
                  {
                     $scope.StopNode['config']['select'].push( {
                        'key': nodesInfo['Role'] == 'data' ? ( nodesInfo['GroupName'] + ' ' + nodesInfo['NodeName'] ) : ( nodesInfo['Role'] + ' ' + nodesInfo['NodeName'] ),
                        'value': nodesInfo['NodeName']
                     } ) ;
                     if( $scope.StopNode['config']['select'].length == 1 )
                     {
                        $scope.StopNode['config']['value'] = hostname + ':' + svcname ;
                     }
                  }
               }
            }, 
            'failed':function( errorInfo ){
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  getCoordStatus( nodesInfo, hostname, svcname ) ;
                  return true ;
               } ) ;
            }
         },{
            'showLoading': false
         } ) ;
      }

      //获取节点列表
      var getNodesList = function(){
         var data = { 'cmd': 'list groups' } ;
         SdbRest.DataOperation( data, {
            'success': function( groups ){
               var nodesList = [] ;
               $scope.GroupList = groups ;
               $.each( groups, function( index, groupInfo ){
                  if( groupInfo['Role'] == 2 )
                     groupInfo['Role'] = 'catalog' ;
                  else if( groupInfo['Role'] == 1 )
                     groupInfo['Role'] = 'coord' ;
                  else if( groupInfo['Role'] == 0 )
                     groupInfo['Role'] = 'data' ;
                  $.each( groupInfo['Group'], function( index2, nodeInfo ){
                     var isPrimary = false ;
                     if( groupInfo['Role'] == 'coord' )
                        isPrimary = '-' ;
                     else if( nodeInfo['NodeID'] == groupInfo['PrimaryNode'] )
                        isPrimary = true ;
                     nodesList.push( {
                        'HostName': nodeInfo['HostName'],
                        'ServiceName': nodeInfo['Service']['0']['Name'],
                        'GroupName': groupInfo['GroupName'],
                        'Role': groupInfo['Role'],
                        'NodeID': nodeInfo['NodeID'],
                        'Status': true,
                        'IsPrimary': isPrimary
                     } ) ;
                  } ) ;
               } ) ;
               $.each( nodesList, function( index, nodeInfo ){
                  nodesList[index]['NodeName'] = nodeInfo['HostName'] + ':' + nodeInfo['ServiceName'] ;
                  nodesList[index]['TotalRecords'] = '-' ;
                  nodesList[index]['TotalLobs'] = '-' ;
                  nodesList[index]['TotalCL'] = '-' ;
                  //统计数据节点的数据
                  if( nodeInfo['Role'] == 'data' )
                  {
                     nodesList[index]['TotalRecords'] = 0 ;
                     nodesList[index]['TotalLobs'] = 0 ;
                     nodesList[index]['TotalCL'] = 0 ;
                     $.each( clList, function( index3, ClInfo ){
                        if( nodesList[index]['NodeName'] == ClInfo['NodeName'] )
                        {
                           nodesList[index]['TotalCL'] += 1 ;
                           nodesList[index]['TotalRecords'] += ClInfo['TotalRecords'] ;
                           nodesList[index]['TotalLobs'] += ClInfo['TotalLobs'] ;
                        }
                     } ) ;
                  }
                  else if( nodeInfo['Role'] == 'coord' )
                  {
                     getCoordStatus( nodeInfo, nodeInfo['HostName'], nodeInfo['ServiceName'] ) ;
                  }
                  //如果节点错误，那么数据为空
                  if( clList.length > 0 )
                  {
                     $.each( clList, function( nodeIndex2, nodeInfo2 ){
                        if( isArray( nodeInfo2['ErrNodes'] ) )
                        {
                           $.each( nodeInfo2['ErrNodes'], function( nodeIndex, errNodeInfo ){
                              if( nodesList[index]['NodeName'] == errNodeInfo['NodeName'] )
                              {
                                 nodesList[index]['Status'] = false ;
                                 nodesList[index]['Flag'] = errNodeInfo['Flag'] ;
                                 nodesList[index]['TotalRecords'] = '-' ;
                                 nodesList[index]['TotalLobs'] = '-' ;
                                 nodesList[index]['TotalCL'] = '-' ;
                                 return false ;
                              }
                           } ) ;
                        }
                     } ) ;
                  }
                  if( nodesList[index]['Role'] !== 'coord' )   //coord节点另外处理
                  {
                     //记录停止的节点
                     if( nodesList[index]['Status'] == false && $scope.StartNode['config']['select'].length == 0 )
                        $scope.StartNode['config']['value'] = nodesList[index]['NodeName'] ;
                     if( nodesList[index]['Status'] == false )
                        $scope.StartNode['config']['select'].push( {
                           'key': nodesList[index]['Role'] == 'data' ? ( nodesList[index]['GroupName'] + ' ' + nodesList[index]['NodeName'] ) : ( nodesList[index]['Role'] + ' ' + nodesList[index]['NodeName'] ),
                           'value': nodesList[index]['NodeName']
                        } ) ;

                     //记录启动的节点
                     if( nodesList[index]['Status'] == true && $scope.StopNode['config']['select'].length == 0 )
                        $scope.StopNode['config']['value'] = nodesList[index]['NodeName'] ;
                     if( nodesList[index]['Status'] == true )
                        $scope.StopNode['config']['select'].push( {
                           'key': nodesList[index]['Role'] == 'data' ? ( nodesList[index]['GroupName'] + ' ' + nodesList[index]['NodeName'] ) : ( nodesList[index]['Role'] + ' ' + nodesList[index]['NodeName'] ),
                           'value': nodesList[index]['NodeName']
                        } ) ;
                  }
               } ) ;
               $scope.NodeTable['body'] = nodesList ;
            }, 
            'faild': function( errorInfo ){
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  getNodesList() ;
                  return true ;
               } ) ;
            }
         },{
            'showLoading': false
         } ) ;
      } ;

      //获取CL快照
      var getClList = function(){
         $scope.StartNode['config']['select'] = [] ;
         $scope.StopNode['config']['select']  = [] ;
         nodesList = [] ;
         var sql = 'SELECT t1.Name,t1.Details.TotalRecords, t1.Details.TotalLobs,t1.Details.NodeName, t1.ErrNodes from (SELECT Name, Details, ErrNodes FROM $SNAPSHOT_CL WHERE Global = true split By Details) As t1' ;
         SdbRest.Exec( sql, {
            'success': function( list ){
               clList = list ;
               getNodesList() ;
            },
            'failed': function( errorInfo ){
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  getClList() ;
                  return true ;
               } ) ;
            }
         },{
            'showLoading': false
         } ) ;
      } ;

      $scope.GotoSync = function(){
         $rootScope.tempData( 'Deploy', 'ModuleName',  moduleName ) ;
         $rootScope.tempData( 'Deploy', 'ClusterName', clusterName ) ;
         $location.path( '/Deploy/SDB-Sync' ).search( { 'r': new Date().getTime() } ) ;
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

      getClList() ;

   } ) ;
}());