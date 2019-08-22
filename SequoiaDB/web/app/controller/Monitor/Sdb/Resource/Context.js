//@ sourceURL=Context.js
(function(){
   var sacApp = window.SdbSacManagerModule ;
   //控制器
   sacApp.controllerProvider.register( 'Monitor.SdbResource.Context.Ctrl', function( $scope, $compile, $location, SdbRest, SdbFunction ){
      
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
      //上下文类型
      $scope.ContextType = 'all' ;
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
      //上下文详细信息 弹窗
      $scope.ContextInfo = {
         'config': {},
         'callback': {}
      } ;
      //表格 显示列 下拉菜单
      $scope.FieldDropdown = {
         'config': [
            { 'key': 'NodeName',       'show': true },
            { 'key': 'ContextID',      'show': true },
            { 'key': 'SessionID',      'show': true },
            { 'key': 'Type',           'show': true },
            { 'key': 'Description',    'show': true },
            { 'key': 'DataRead',       'show': true },
            { 'key': 'IndexRead',      'show': true },
            { 'key': 'QueryTimeSpent', 'show': true },
            { 'key': 'StartTimestamp', 'show': true }
         ],
         'callback': {}
      } ;
      //上下文类型 下拉菜单
      $scope.ContextTypeDropdown = {
         'ContextType': 'all',
         'config': [
            { 'key': 'all', 'field': $scope.autoLanguage( '所有上下文' ) },
            { 'key': 'current', 'field': $scope.autoLanguage( '当前上下文' ) }
         ],
         'callback': {}
      } ;
      //上下文列表的表格
      $scope.ContextTable = {
         'title': {
            'NodeName':    'NodeName',
            'ContextID':   'ContextID',
            'SessionID':   'SessionID',
            'Type':        'Type',
            'Description': 'Description',
            'DataRead':    'DataRead',
            'IndexRead':   'IndexRead',
            'QueryTimeSpent': 'QueryTimeSpent',
            'StartTimestamp': 'StartTimestamp'
         },
         'body': [],
         'options': {
            'width': {},
            'sort': {
               'NodeName':    true,
               'ContextID':   true,
               'SessionID':   true,
               'Type':        true,
               'Description': true,
               'DataRead':    true,
               'IndexRead':   true,
               'QueryTimeSpent': true,
               'StartTimestamp': true
            },
            'max': 50,
            'filter': {
               'NodeName':    'indexof',
               'ContextID':   'number',
               'SessionID':   'number',
               'Type':        'indexof',
               'Description': 'indexof',
               'DataRead':    'number',
               'IndexRead':   'number',
               'QueryTimeSpent': 'number',
               'StartTimestamp': 'indexof'
            }
         },
         'callback': {}
      } ;

      //获取上下文
      var getContextList = function(){
         var sql = 'SELECT * FROM $SNAPSHOT_CONTEXT' ;
         if( $scope.ContextType == 'current' )
         {
            sql = 'SELECT * FROM $SNAPSHOT_CONTEXT_CUR' ;
         }
         SdbRest.Exec( sql, {
            'success': function( data ){
               var contextList = [] ;
               $.each( data, function( index, sessionInfo ){
                  if( isArray( sessionInfo['Contexts'] ) )
                  {
                     $.each( sessionInfo['Contexts'], function( contextIndex, contextInfo ){
                        contextInfo['SessionID'] = sessionInfo['SessionID'] ;
                        contextInfo['NodeName'] = sessionInfo['NodeName'] ;
                        contextList.push( contextInfo ) ;
                     } ) ;
                  }
               } ) ;
               $scope.ContextTable['body'] = contextList ;
               if( $scope.Timer['callback']['GetStatus']() == 'start' || $scope.Timer['callback']['GetStatus']() == 'complete' ) //如果开了定时器，就开始
               {
                  $scope.Timer['callback']['Complete']() ;
               }
            },
            'failed': function( errorInfo ){
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  getContextList() ;
                  return true ;
               } ) ;
            }
         }, { 'showLoading': false } ) ;
      } ;

      getContextList() ;

      //显示上下文详细
      $scope.ShowContext = function( contextID, nodeName ){
         var context = {} ;
         $.each( $scope.ContextTable['body'], function( index, contextInfo ){
            if( contextInfo['ContextID'] == contextID && contextInfo['NodeName'] == nodeName )
            {
               context = contextInfo ;
               return false ;
            }
         } ) ;
         $scope.ContextInfo['config'] = context ;
         //设置标题
         $scope.ContextInfo['callback']['SetTitle']( $scope.autoLanguage( '上下文信息' ) ) ;
         //设置图标
         $scope.ContextInfo['callback']['SetIcon']( '' ) ;
         //打开窗口
         $scope.ContextInfo['callback']['Open']() ;
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
            $scope.Timer['callback']['Start']( getContextList ) ;
         }
      }


      //打开 显示列 下拉菜单
      $scope.OpenShowFieldDropdown = function( event ){
         $.each( $scope.FieldDropdown['config'], function( index, fieldInfo ){
            $scope.FieldDropdown['config'][index]['show'] = typeof( $scope.ContextTable['title'][fieldInfo['key']] ) == 'string' ;
         } ) ;
         $scope.FieldDropdown['callback']['Open']( event.currentTarget ) ;
      }

      //保存 显示列
      $scope.SaveField = function(){
         $.each( $scope.FieldDropdown['config'], function( index, fieldInfo ){
            $scope.ContextTable['title'][fieldInfo['key']] = fieldInfo['show'] ? fieldInfo['key'] : false ;
         } ) ;
         $scope.ContextTable['callback']['ShowCurrentPage']() ;
      }

      //打开 上下文类型 下拉菜单
      $scope.OpenShowContextTypeDropdown = function( event ){
         $scope.ContextTypeDropdown.ContextType = $scope.ContextType ;
         $scope.ContextTypeDropdown['callback']['Open']( event.currentTarget ) ;
      }

      //保存 上下文类型
      $scope.SaveContextType = function(){
         $scope.ContextType = $scope.ContextTypeDropdown.ContextType ;
         $scope.ContextTypeDropdown['callback']['Close']() ;
         if( $scope.Timer['callback']['GetStatus']() == 'stop' )
         {
            getContextList() ;
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
      $scope.GotoNodeList = function(){
         if( moduleMode == 'distribution' )
         {
            $location.path( '/Monitor/SDB-Nodes/Nodes' ).search( { 'r': new Date().getTime() } ) ;
         }
         else
         {
            $location.path( '/Monitor/SDB-Nodes/Node/Index' ).search( { 'r': new Date().getTime() } ) ;
         }
      }

      //跳转至节点
      $scope.GotoNode = function( NodeName ){
         var temp = NodeName.split( ":" ) ;
         SdbFunction.LocalData( 'SdbHostName', temp[0] ) ;
         SdbFunction.LocalData( 'SdbServiceName', temp[1] ) ;
         $location.path( '/Monitor/SDB-Nodes/Node/Index' ).search( { 'r': new Date().getTime() } ) ;
      }

   } ) ;
}());