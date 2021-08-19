//@ sourceURL=Session.js
(function(){
   var sacApp = window.SdbSacManagerModule ;
   //控制器
   sacApp.controllerProvider.register( 'Monitor.SdbResource.Session.Ctrl', function( $scope, $compile, $timeout, $location, SdbRest, SdbFunction ){
      
      var clusterName = SdbFunction.LocalData( 'SdbClusterName' ) ;
      var moduleType = SdbFunction.LocalData( 'SdbModuleType' ) ;
      var moduleMode = SdbFunction.LocalData( 'SdbModuleMode' ) ;
      var moduleName = SdbFunction.LocalData( 'SdbModuleName' ) ;
      if( clusterName == null || moduleType != 'sequoiadb' || moduleMode == null || moduleName == null )
      {
         $location.path( '/Transfer' ).search( { 'r': new Date().getTime() } ) ;
         return;
      }

      //初始化
      $scope.ModuleMode = moduleMode ;
      //会话类型
      $scope.SessionType = 'all' ;
      //定时器
      $scope.IntervalTimeConfig = {
         'interval': 5,
         'play': false
      } ;
      $scope.Timer = {
         'config': $scope.IntervalTimeConfig,
         'callback': {}
      } ;
      //实时刷新设置 弹窗
      $scope.CreateBrush = {
         'config': {},
         'callback': {}
      } ;
      //刷新状态
      $scope.RefreshType = $scope.autoLanguage( '启动刷新' ) ;
      //会话详细信息 弹窗
      $scope.SessionInfo = {
         'config': {},
         'callback': {}
      } ;
      //表格 显示列 下拉菜单
      $scope.FieldDropdown = {
         'config': [
            { 'key': 'Status',            'field': 'Status',            'show': true },
            { 'key': 'NodeName',          'field': 'NodeName',          'show': true },
            { 'key': 'SessionID',         'field': 'SessionID',         'show': true },
            { 'key': 'TID',               'field': 'TID',               'show': true },
            { 'key': 'Type',              'field': 'Type',              'show': true },
            { 'key': 'Classify',          'field': 'Classify',          'show': true },
            { 'key': 'Name',              'field': 'Name',              'show': false },
            { 'key': 'QueueSize',         'field': 'QueueSize',         'show': false },
            { 'key': 'ProcessEventCount', 'field': 'ProcessEventCount', 'show': false },
            { 'key': 'RelatedID',         'field': 'RelatedID',         'show': false },
            { 'key': 'Contexts.length',   'field': 'Contexts',          'show': true },
            { 'key': 'TotalDataRead',     'field': 'TotalDataRead',     'show': false },
            { 'key': 'TotalIndexRead',    'field': 'TotalIndexRead',    'show': false },
            { 'key': 'TotalDataWrite',    'field': 'TotalDataWrite',    'show': false },
            { 'key': 'TotalIndexWrite',   'field': 'TotalIndexWrite',   'show': false },
            { 'key': 'TotalUpdate',       'field': 'TotalUpdate',       'show': true },
            { 'key': 'TotalDelete',       'field': 'TotalDelete',       'show': true },
            { 'key': 'TotalInsert',       'field': 'TotalInsert',       'show': true },
            { 'key': 'TotalSelect',       'field': 'TotalSelect',       'show': false },
            { 'key': 'TotalRead',         'field': 'TotalRead',         'show': true },
            { 'key': 'TotalReadTime',     'field': 'TotalReadTime',     'show': false },
            { 'key': 'TotalWriteTime',    'field': 'TotalWriteTime',    'show': false },
            { 'key': 'ReadTimeSpent',     'field': 'ReadTimeSpent',     'show': false },
            { 'key': 'WriteTimeSpent',    'field': 'WriteTimeSpent',    'show': false },
            { 'key': 'ConnectTimestamp',  'field': 'ConnectTimestamp',  'show': false },
            { 'key': 'LastOpType',        'field': 'LastOpType',        'show': false },
            { 'key': 'LastOpBegin',       'field': 'LastOpBegin',       'show': false },
            { 'key': 'LastOpEnd',         'field': 'LastOpEnd',         'show': false },
            { 'key': 'LastOpInfo',        'field': 'LastOpInfo',        'show': false },
            { 'key': 'UserCPU',           'field': 'UserCPU',           'show': false },
            { 'key': 'SysCPU',            'field': 'SysCPU',            'show': false }
         ],
         'callback': {}
      } ;
      //会话类型 下拉菜单
      $scope.SessionTypeDropdown = {
         'SessionType': 'all',
         'config': [
            { 'key': 'all', 'field': $scope.autoLanguage( '所有会话' ) },
            { 'key': 'current', 'field': $scope.autoLanguage( '当前会话' ) }
         ],
         'callback': {}
      } ;
      //过滤非空闲的会话
      var filterSessionIdle = function( value ){
         return value != 'Idle' ;
      }
      //会话列表的表格
      $scope.SessionTable = {
         'title': {
            'Status':             'Status',
            'NodeName':           'NodeName',
            'SessionID':          'SessionID',
            'TID':                'TID',
            'Type':               'Type',
            'Classify':           'Classify',
            'Name':               false,
            'QueueSize':          false,
            'ProcessEventCount':  false,
            'RelatedID':          false,
            'Contexts.length':    'Contexts',
            'TotalDataRead':      false,
            'TotalIndexRead':     false,
            'TotalDataWrite':     false,
            'TotalIndexWrite':    false,
            'TotalUpdate':        'TotalUpdate',
            'TotalDelete':        'TotalDelete',
            'TotalInsert':        'TotalInsert',
            'TotalSelect':        false,
            'TotalRead':          'TotalRead',
            'TotalReadTime':      false,
            'TotalWriteTime':     false,
            'ReadTimeSpent':      false,
            'WriteTimeSpent':     false,
            'ConnectTimestamp':   false,
            'LastOpType':         false,
            'LastOpBegin':        false,
            'LastOpEnd':          false,
            'LastOpInfo':         false,
            'UserCPU':            false,
            'SysCPU':             false
         },
         'body': [],
         'options': {
            'width': {
               'Status': '90px',
               'Type': '120px',
               'Classify': '90px'
            },
            'sort': {
               'Status': true,
               'NodeName': true,
               'SessionID': true,
               'TID': true,
               'Type': true,
               'Classify': true,
               'Name': true,
               'QueueSize': true,
               'ProcessEventCount': true,
               'RelatedID': true,
               'Contexts.length': true,
               'TotalDataRead': true,
               'TotalIndexRead': true,
               'TotalDataWrite': true,
               'TotalIndexWrite': true,
               'TotalUpdate': true,
               'TotalDelete': true,
               'TotalInsert': true,
               'TotalSelect': true,
               'TotalRead': true,
               'TotalReadTime': true,
               'TotalWriteTime': true,
               'ReadTimeSpent': true,
               'WriteTimeSpent': true,
               'ConnectTimestamp': true,
               'LastOpType': true,
               'LastOpBegin': true,
               'LastOpEnd': true,
               'LastOpInfo': true,
               'UserCPU': true,
               'SysCPU': true
            },
            'max': 50,
            'filter': {
               'Status': [
                  { 'key': $scope.autoLanguage( '全部' ), 'value': '' },
                  { 'key': 'No Idle', 'value': filterSessionIdle },
                  { 'key': 'Running', 'value': 'Running' },
                  { 'key': 'Waiting', 'value': 'Waiting' },
                  { 'key': 'Idle', 'value': 'Idle' }
               ],
               'NodeName': 'indexof',
               'SessionID': 'number',
               'TID': 'number',
               'Type': 'indexof',
               'Classify': [
                  { 'key': $scope.autoLanguage( '全部' ), 'value': '' },
                  { 'key': $scope.autoLanguage( '外部会话' ), 'value': 'User' },
                  { 'key': $scope.autoLanguage( '系统会话' ), 'value': 'System' },
                  { 'key': $scope.autoLanguage( '后台任务' ), 'value': 'Task' }
               ],
               'Name': 'indexof',
               'QueueSize': 'number',
               'ProcessEventCount': 'number',
               'RelatedID': 'indexof',
               'Contexts.length': 'number',
               'TotalDataRead': 'number',
               'TotalIndexRead': 'number',
               'TotalDataWrite': 'number',
               'TotalIndexWrite': 'number',
               'TotalUpdate': 'number',
               'TotalDelete': 'number',
               'TotalInsert': 'number',
               'TotalSelect': 'number',
               'TotalRead': 'number',
               'TotalReadTime': 'number',
               'TotalWriteTime': 'number',
               'ReadTimeSpent': 'number',
               'WriteTimeSpent': 'number',
               'ConnectTimestamp': 'number',
               'LastOpType': 'indexof',
               'LastOpBegin': 'indexof',
               'LastOpEnd': 'indexof',
               'LastOpInfo': 'indexof',
               'UserCPU': 'number',
               'SysCPU': 'number'
            },
            'default': {
               'Status': filterSessionIdle,
               'Classify': 'User'
            }
         },
         'callback': {}
      } ;

      //获取会话列表
      var getSessionList = function(){
         var sql = 'SELECT * FROM $SNAPSHOT_SESSION' ;
         if( $scope.SessionType == 'current' )
         {
            sql = 'SELECT * FROM $SNAPSHOT_SESSION_CUR' ;
         }
         SdbRest.Exec( sql, {
            'success': function( SessionList ){
               $.each( $scope.SessionTable['body'], function( index ){
                  $scope.SessionTable['body'][index] = null ;
               } ) ;
               $scope.SessionTable['body'] = [] ;
               //分类Classify
               $.each( SessionList, function( index, value ){
                  if( isArray( value['ErrNodes'] ) ) //跳过错误的节点
                     return true ;
                  if( value['Type'] == 'Task' )
                  {
                     value['Classify'] = 'Task' ;
                  }
                  else if( value['Type'].indexOf( 'Agent' ) >= 0 )
                  {
                     value['Classify'] = 'User' ;
                  }
                  else
                  {
                     value['Classify'] = 'System' ;
                  }
                  if( typeof( value['SessionID'] ) != 'undefined' )
                  {
                     $scope.SessionTable['body'].push( value ) ;
                  }
               } ) ;
               SessionList = null ;
               if( $scope.Timer['callback']['GetStatus']() == 'start' || $scope.Timer['callback']['GetStatus']() == 'complete' ) //如果开了定时器，就开始
               {
                  $scope.Timer['callback']['Complete']() ;
               }
            },
            'failed': function( errorInfo ){
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  getSessionList() ;
                  return true ;
               } ) ;
            }
         }, { 'showLoading': false } ) ;
      } ;

      getSessionList() ;
      //中断会话
      var forceSession = function( sessionId, nodeName ){
         var hostname = nodeName.split( ':' ) ;
         var svcname = hostname[1] ;
         hostname = hostname[0] ;
         var data = { 'cmd': 'force session', 'SessionID': sessionId, 'Options': JSON.stringify( { 'HostName': hostname, 'svcname': svcname } ) } ;
         SdbRest.DataOperation( data, {
            'success': function(){
               $scope.SessionInfo['callback']['Close']() ;
               if( $scope.Timer['callback']['GetStatus']() == 'stop' )
               {
                  getSessionList() ;
               }
            },
            'failed': function( errorInfo ){
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  forceSession( sessionId, nodeName ) ;
                  return true ;
               } ) ;
            }
         } ) ;
      }

      //显示会话详细
      $scope.ShowSession = function( sessionID, nodeName ){
         var session = {} ;
         $.each( $scope.SessionTable['body'], function( index, sessionInfo ){
            if( sessionInfo['SessionID'] == sessionID && sessionInfo['NodeName'] == nodeName )
            {
               session = $.extend( true, {}, sessionInfo ) ;
               if( session['Contexts'].length > 0 )
               {
                  session['Contexts'] = session['Contexts'].join() ;
               }
               return false ;
            }
         } ) ;
         $scope.SessionInfo['config'] = session ;
         //设置确定按钮
         $scope.SessionInfo['callback']['SetOkButton']( $scope.autoLanguage( '断开会话' ), function(){
            forceSession( session['SessionID'], session['NodeName'] ) ;
            return false ;
         } ) ;
         //设置标题
         $scope.SessionInfo['callback']['SetTitle']( $scope.autoLanguage( '会话信息' ) ) ;
         //设置图标
         $scope.SessionInfo['callback']['SetIcon']( '' ) ;
         //打开窗口
         $scope.SessionInfo['callback']['Open']() ;
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

      //是否刷新
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
            $scope.Timer['callback']['Start']( getSessionList ) ;
         }
      }

      //打开 显示列 下拉菜单
      $scope.OpenShowFieldDropdown = function( event ){
         $.each( $scope.FieldDropdown['config'], function( index, fieldInfo ){
            $scope.FieldDropdown['config'][index]['show'] = typeof( $scope.SessionTable['title'][fieldInfo['key']] ) == 'string' ;
         } ) ;
         $scope.FieldDropdown['callback']['Open']( event.currentTarget ) ;
      }

      //保存 显示列
      $scope.SaveField = function(){
         $.each( $scope.FieldDropdown['config'], function( index, fieldInfo ){
            $scope.SessionTable['title'][fieldInfo['key']] = fieldInfo['show'] ? ( fieldInfo['key'] == 'Contexts.length' ? 'Contexts' : fieldInfo['key'] )  : false ;
         } ) ;
         //$scope.FieldDropdown['callback']['Close']() ;
         $scope.SessionTable['callback']['ShowCurrentPage']() ;
      }

      //打开 会话类型 下拉菜单
      $scope.OpenShowSessionTypeDropdown = function( event ){
         $scope.SessionTypeDropdown.SessionType = $scope.SessionType ;
         $scope.SessionTypeDropdown['callback']['Open']( event.currentTarget ) ;
      }

      //保存 会话类型
      $scope.SaveSessionType = function(){
         $scope.SessionType = $scope.SessionTypeDropdown.SessionType ;
         $scope.SessionTypeDropdown['callback']['Close']() ;
         if( $scope.Timer['callback']['GetStatus']() == 'stop' )
         {
            getSessionList() ;
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
      $scope.GotoNodeList = function(){
         if( moduleMode == 'distribution' )
         {
            $location.path( '/Monitor/SDB-Nodes/Nodes' ).search( { 'r': new Date().getTime() } ) ;
         }
         else
         {
            $location.path( '/Monitor/SDB-Nodes/Node/Index' ).search( { 'r': new Date().getTime() } ) ;
         }
      } ;

      //跳转至节点
      $scope.GotoNode = function( NodeName ){
         var temp = NodeName.split( ":" ) ;
         SdbFunction.LocalData( 'SdbHostName', temp[0] ) ;
         SdbFunction.LocalData( 'SdbServiceName', temp[1] ) ;
         $location.path( '/Monitor/SDB-Nodes/Node/Index' ).search( { 'r': new Date().getTime() } ) ;
      } ;
   } ) ;
}());