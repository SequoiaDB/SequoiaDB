//@ sourceURL=Transaction.js
(function(){
   var sacApp = window.SdbSacManagerModule ;
   //控制器
   sacApp.controllerProvider.register( 'Monitor.SdbResource.Transaction.Ctrl', function( $scope, $compile, SdbRest, $location, SdbFunction ){
      
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
      //会话详细信息 弹窗
      $scope.SessionInfo = {
         'config': {},
         'callback': {}
      } ;
      //事务详细信息 弹窗
      $scope.TransactionInfo = {
         'config': {},
         'callback': {}
      } ;
      //事务列表
      var transactionInfoList = [] ;
      //上下文列表的表格
      $scope.TransactionTable = {
         'title': {
            'TransactionID':        $scope.autoLanguage( '事务ID' ),
            'NodeName':             $scope.autoLanguage( '节点' ),
            'SessionID':            $scope.autoLanguage( '会话ID' ),
            'GroupName':            $scope.autoLanguage( '分区组' ),
            'CurrentTransLSN':      $scope.autoLanguage( '当前事务LSN' ),
            'IsRollback':           $scope.autoLanguage( '是否回滚' ),
            'WaitLockNum':             $scope.autoLanguage( '等待锁' ),
            'TransactionLocksNum':  $scope.autoLanguage( '锁总数' )
         },
         'body': [],
         'options': {
            'width': {},
            'sort': {
               'TransactionID':        true,
               'NodeName':             true,
               'SessionID':            true,
               'GroupName':            true,
               'CurrentTransLSN':      true,
               'IsRollback':           true,
               'WaitLockNum':             true,
               'TransactionLocksNum':  true
            },
            'max': 50,
            'filter': {
               'TransactionID':        'indexof',
               'NodeName':             'indexof',
               'SessionID':            'number',
               'GroupName':            'indexof',
               'CurrentTransLSN':      'number',
               'IsRollback': [
                  { 'key': $scope.autoLanguage( '全部' ), 'value': '' },
                  { 'key': $scope.autoLanguage( '是' ), 'value': true },
                  { 'key': $scope.autoLanguage( '否' ), 'value': false }
               ],
               'WaitLockNum':          'number',
               'TransactionLocksNum':  'number'
            }
         },
         'callback': {}
      } ;

      //获取事务列表
      var getTransaction = function(){
         var sql = 'SELECT * FROM $LIST_TRANS' ;
         SdbRest.Exec( sql, {
            'success': function( data ){
               //alert( JSON.stringify(data) ) ;
               var transactionList = [] ;
               $.each( data, function( index, transactionInfo ){
                  if( isArray( transactionInfo['ErrNodes'] ) == false )
                  {
                     transactionList.push( transactionInfo ) ;
                  }
               } ) ;
               if( transactionList.length > 0 )
               {
                  transactionInfoList = $.extend( true, transactionList ) ;
                  $.each( transactionList, function( index, transactionInfo ){
                     var waitLock = 0 ;
                     //等待锁数量
                     transactionList[index]['WaitLockNum'] = 0 ;
                     //弹窗使用的id标识
                     transactionList[index]['i'] = index ;

                     if( transactionInfo['WaitLock']['CSID'] > -1 || ( transactionInfo['WaitLock']['CLID'] > -1 && transactionInfo['WaitLock']['CLID'] < 65535 ) || transactionInfo['WaitLock']['recordID'] > -1 )
                     {
                        transactionList[index]['WaitLockNum'] = 1 ;
                     }
                  } ) ;
                  $scope.TransactionTable['body'] = transactionList ;
               }
            },
            'failed': function( errorInfo ){
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  getTransaction() ;
                  return true ;
               } ) ;
            }
         }, { 'showLoading': false } ) ;
      }
      
      getTransaction() ;

      //显示会话详细
      $scope.ShowSession = function( index ){
         var sessionInfo = {} ;
         var sessionID = transactionInfoList[index]['SessionID'] ;
         var nodeName = transactionInfoList[index]['NodeName'] ;
         var sql = 'SELECT * FROM $SNAPSHOT_SESSION WHERE SessionID = ' + sessionID + ' AND NodeName = "' + nodeName + '"' ;
         SdbRest.Exec( sql, {
            'success': function( sessionList ){
               if( sessionList.length > 0 )
               {
                  sessionInfo = sessionList[0] ;
                  //将上下文ID数组转成字符串
                  sessionInfo['Contexts'] = sessionInfo['Contexts'].join() ;
                  $scope.SessionInfo['config'] = sessionInfo ;
                  //设置标题
                  $scope.SessionInfo['callback']['SetTitle']( $scope.autoLanguage( '会话信息' ) ) ;
                  //设置图标
                  $scope.SessionInfo['callback']['SetIcon']( '' ) ;
                  //打开窗口
                  $scope.SessionInfo['callback']['Open']() ;
                  //异步打开弹窗，需要马上刷新$watch
                  $scope.$digest() ;
               }
            },
            'failed': function( errorInfo ){
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  $scope.SessionInfo['callback']['Close']() ;
                  $scope.ShowSession( nodeName, sessionID ) ;
                  return true ;
               } ) ;
            }
         }, { 'showLoading': false } ) ;
      }

      //显示事务详细
      $scope.ShowTrans = function( index ){
         $scope.TransactionInfo['config'] = transactionInfoList[index] ;
         $scope.TransactionInfo['callback']['SetTitle']( $scope.autoLanguage( '事务信息' ) ) ;
         $scope.TransactionInfo['callback']['SetIcon']('') ;
         $scope.TransactionInfo['callback']['Open']() ;
      }

      //跳转至分区组
      $scope.GotoGroup = function( groupName ){
         SdbFunction.LocalData( 'SdbGroupName', groupName ) ;
         $location.path( '/Monitor/SDB-Nodes/Group/Index' ).search( { 'r': new Date().getTime() } ) ;
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
   } ) ;
}());