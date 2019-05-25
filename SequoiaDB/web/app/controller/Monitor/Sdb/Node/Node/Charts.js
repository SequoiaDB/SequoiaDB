//@ sourceURL=Charts.js
(function(){
   var sacApp = window.SdbSacManagerModule ;
   //控制器
   sacApp.controllerProvider.register( 'Monitor.SdbNode.Charts.Ctrl', function( $scope, SdbRest, $location, SdbFunction ){
      
      _IndexPublic.checkMonitorEdition( $location ) ; //检测是不是企业版

      var clusterName = SdbFunction.LocalData( 'SdbClusterName' ) ;
      var moduleType = SdbFunction.LocalData( 'SdbModuleType' ) ;
      var moduleMode = SdbFunction.LocalData( 'SdbModuleMode' ) ;
      var moduleName = SdbFunction.LocalData( 'SdbModuleName' ) ;
      var hostName = SdbFunction.LocalData( 'SdbHostName' ) ;
      var svcname = SdbFunction.LocalData( 'SdbServiceName' ) ;
      if( clusterName == null || moduleType != 'sequoiadb' || moduleMode == null || moduleName == null )
      {
         $location.path( '/Transfer' ).search( { 'r': new Date().getTime() } ) ;
         return;
      }

      if( moduleMode == 'distribution' && ( hostName == null || svcname == null ) )
      {
         $location.path( '/Monitor/SDB/Index' ).search( { 'r': new Date().getTime() } ) ;
         return;
      }

      //初始化
      //节点角色（集群模式才有效）
      var nodeRole ;
      $scope.ModuleMode = moduleMode ;
      var chartInfo = {
         'Transaction': 0,
         'Sessions': 0,
         'Contexts': 0
      } ;
      //用来记录数据获取状态, true: 数据获取了, false: 还没有获取, null: 获取失败
      var updateStatus = {
         'Transaction': false,
         'Sessions': false,
         'Contexts': false
      } ;

      //图表初始化
      $scope.charts = {};
      $scope.charts['Sessions'] = {} ;
      $scope.charts['Sessions']['options'] = window.SdbSacManagerConf.TotalSessionsEchart ;
      $scope.charts['Contexts'] = {} ;
      $scope.charts['Contexts']['options'] = window.SdbSacManagerConf.TotalContextsEchart ;
      $scope.charts['Transactions'] = {} ;
      $scope.charts['Transactions']['options'] = window.SdbSacManagerConf.TotalTransactionsEchart ;

      var updateToCharts = function(){
         var isUpdate = true ;
         $.each( updateStatus, function( key, status ){
            if( status == null ) //这个消息报错了，忽略过去
               return true ;
            else if( status == false )
               isUpdate = false ;
         } ) ;
         if( isUpdate == true )  //全部都收到消息了，更新图表
         {
            $scope.charts['Transactions']['value'] = [ [ 0, chartInfo['Transaction'], true, false ] ] ;
            $scope.charts['Sessions']['value'] = [ [ 0, chartInfo['Sessions'], true, false ] ] ;
            $scope.charts['Contexts']['value'] = [ [ 0, chartInfo['Contexts'], true, false ] ] ;
            $.each( updateStatus, function( key, status ){  //重置状态
               if( status == true )
                  updateStatus[key] = false ;
            } ) ;
            $scope.$digest() ;
         }
      }

      //获取Context数量
      var getContextsNum = function(){
         var sql = '' ;
         if( moduleMode == 'standalone' )
         {
            sql = 'select count(t1.Contexts.ContextID) as ContextNum from (select * from $SNAPSHOT_CONTEXT split by Contexts) as t1' ;
         }
         else
         {
            if( nodeRole == 1 )
            {
               sql = 'select count(t1.Contexts.ContextID) as ContextNum from (select * from $SNAPSHOT_CONTEXT where GLOBAL=false split by ErrNodes) as t1 where t1.ErrNodes is null split by t1.Contexts' ;
            }
            else
            {
               sql = 'select count(t1.Contexts.ContextID) as ContextNum from (select * from $SNAPSHOT_CONTEXT where HostName="' + hostName +'" and svcname="'+ svcname + '" split by ErrNodes) as t1 where t1.ErrNodes is null split by t1.Contexts' ;
            }
         }
         SdbRest.Exec( sql, {
            'before': function( jqXHR ){
               if( nodeRole == 1 )
               {
                  jqXHR.setRequestHeader( 'SdbHostName', hostName ) ;
                  jqXHR.setRequestHeader( 'SdbServiceName', svcname ) ;
               }
            },
            'success': function( num ){
               chartInfo['Contexts'] = num[0]['ContextNum'] ;
               updateStatus['Contexts'] = true ;
               updateToCharts() ;
            },
            'failed': function( errorInfo ){
               chartInfo['Contexts'] = 0 ;
               updateStatus['Contexts'] = null ;
               updateToCharts() ;
               getContextsNum() ;
            }
         }, {
            'showLoading': false,
            'delay': 5000,
            'loop': 'success'
         } ) ;
      }

      //获取Session数量
      var getSessionsNum = function(){
         var sql = '' ;
         if( moduleMode == 'standalone' )
         {
            sql = 'select count(SessionID) as SessionNum from $SNAPSHOT_SESSION' ;
         }
         else
         {
            if( nodeRole == 1 )
            {
               sql = 'select count(t1.SessionID) as SessionNum from (select * from $SNAPSHOT_SESSION where GLOBAL=false split by ErrNodes) as t1 where t1.ErrNodes is null' ;
            }
            else
            {
               sql = 'select count(t1.SessionID) as SessionNum from (select * from $SNAPSHOT_SESSION where HostName="' + hostName +'" and svcname="'+ svcname + '" split by ErrNodes) as t1 where t1.ErrNodes is null' ;
            }
         }
         SdbRest.Exec( sql, {
            'before': function( jqXHR ){
               if( nodeRole == 1 )
               {
                  jqXHR.setRequestHeader( 'SdbHostName', hostName ) ;
                  jqXHR.setRequestHeader( 'SdbServiceName', svcname ) ;
               }
            },
            'success': function( num ){
               chartInfo['Sessions'] = num[0]['SessionNum'] ;
               updateStatus['Sessions'] = true ;
               updateToCharts() ;
            },
            'failed': function( errorInfo ){
               chartInfo['Sessions'] = 0 ;
               updateStatus['Sessions'] = null ;
               updateToCharts() ;
               getSessionsNum() ;
            }
         }, {
            'showLoading': false,
            'delay': 5000,
            'loop': 'success'
         } ) ;
      }

      //获取事务快照
      var getTransInfo = function(){
         var sql = '' ;
         if( moduleMode == 'standalone' )
         {
            sql = 'select count(TransactionID) as transNum from $SNAPSHOT_TRANS' ;
         }
         else
         {
            if( nodeRole == 1 )
            {
               sql = 'select count(t1.TransactionID) as transNum from (select * from $SNAPSHOT_TRANS where GLOBAL=false split by ErrNodes) as t1 where t1.ErrNodes is null' ;
            }
            else
            {
               sql = 'select count(t1.TransactionID) as transNum from (select * from $SNAPSHOT_TRANS where HostName="' + hostName +'" and svcname="'+ svcname + '" split by ErrNodes) as t1 where t1.ErrNodes is null' ;
            }
         }
         SdbRest.Exec( sql, {
            'before': function( jqXHR ){
               if( nodeRole == 1 )
               {
                  jqXHR.setRequestHeader( 'SdbHostName', hostName ) ;
                  jqXHR.setRequestHeader( 'SdbServiceName', svcname ) ;
               }
            },
            'success': function( num ){
               chartInfo['Transaction'] = num[0]['transNum'] ;
               updateStatus['Transaction'] = true ;
               updateToCharts() ;
            },
            'failed': function( errorInfo ){
               chartInfo['Transaction'] = 0 ;
               updateStatus['Transaction'] = null ;
               updateToCharts() ;
               getTransInfo() ;
            }
         }, {
            'showLoading': false,
            'delay': 5000,
            'loop': 'success'
         } ) ;
      }

      //获取节点角色
      var getNodeRole = function(){
         var sql = 'select t1.Role from (select * from $LIST_GROUP split by Group) as t1 where t1.Group.HostName="' + hostName +'" and t1.Group.Service.0.Name="'+ svcname + '"' ;
         SdbRest.Exec( sql, {
            'success': function( roleInfo ){
               if( roleInfo.length > 0 )
               {
                  nodeRole = roleInfo[0]['Role'] ;
                  getTransInfo() ;
                  getSessionsNum() ;
                  getContextsNum() ;
               }
            },
            'failed': function( errorInfo ){
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  getNodeRole() ;
                  return true ;
               } ) ;
            }
         },{
            'showLoading': false
         } ) ;
      }

      if( moduleMode == 'distribution' )
      {
         getNodeRole() ;
      }
      else
      {
         getTransInfo() ;
         getSessionsNum() ;
         getContextsNum() ;
      }
     
      //跳转至资源
      $scope.GotoResources = function(){
         $location.path( '/Monitor/SDB-Resources/Session' ).search( { 'r': new Date().getTime() } ) ;
      } ;

      //跳转至主机列表
      $scope.GotoHosts = function(){
         $location.path( '/Monitor/SDB-Host/List/Index' ).search( { 'r': new Date().getTime() } ) ;
      } ;
      
      
      //跳转至节点列表
      $scope.GotoNodes = function(){
         if( moduleMode == 'distribution' )
         {
            $location.path( '/Monitor/SDB-Nodes/Nodes' ).search( { 'r': new Date().getTime() } ) ;
         }
         else
         {
            $location.path( '/Monitor/SDB-Nodes/Node/Index' ).search( { 'r': new Date().getTime() } ) ;
         }
      } ;
   } ) ;

}());