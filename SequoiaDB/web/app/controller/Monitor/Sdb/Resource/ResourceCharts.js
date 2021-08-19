//@ sourceURL=ResourceCharts.js
(function(){
   var sacApp = window.SdbSacManagerModule ;
   //控制器
   sacApp.controllerProvider.register( 'Monitor.SdbResource.ResourceCharts.Ctrl', function( $scope, $location, $timeout, SdbRest, SdbFunction ){
      
      var clusterName = SdbFunction.LocalData( 'SdbClusterName' ) ;
      var moduleType = SdbFunction.LocalData( 'SdbModuleType' ) ;
      var moduleName = SdbFunction.LocalData( 'SdbModuleName' ) ;
      var moduleMode = SdbFunction.LocalData( 'SdbModuleMode' ) ;
      if( clusterName == null || moduleType != 'sequoiadb' || moduleMode == null || moduleName == null )
      {
         $location.path( '/Transfer' ).search( { 'r': new Date().getTime() } ) ;
         return;
      }

      //初始化
      $scope.ModuleMode =  moduleMode ;
      $scope.DbInfo = {} ;

      $scope.charts = {} ; 
      $scope.charts['Sessions'] = {} ;
      $scope.charts['Sessions']['options'] = window.SdbSacManagerConf.TotalSessionsEchart ;

      $scope.charts['Contexts'] = {} ;
      $scope.charts['Contexts']['options'] = window.SdbSacManagerConf.TotalContextsEchart ;

      $scope.charts['Procedures'] = {} ;
      $scope.charts['Procedures']['options'] = window.SdbSacManagerConf.TotalProceduresEchart ;

      $scope.charts['Transactions'] = {} ;
      $scope.charts['Transactions']['options'] = window.SdbSacManagerConf.TotalTransactionsEchart ;

      var SessionNum = 0 ;
      var ContextNum = 0 ;
      var TransNum = 0 ;
      var chartInfo = {
         'Sessions': 0,
         'Contexts': 0,
         'Procedures': 0,
         'Transaction': 0
      } ;
      //用来记录数据获取状态, true: 数据获取了, false: 还没有获取, null: 获取失败
      var updateStatus = {
         'Transaction': false,
         'Sessions': false,
         'Contexts': false,
         'Procedures': moduleMode == 'distribution' ? false : null
      } ;


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
            $scope.charts['Sessions']['value']     = [ [ 0, chartInfo['Sessions'],     true, false ] ] ;
            $scope.charts['Procedures']['value']   = [ [ 0, chartInfo['Procedures'],   true, false ] ] ;
            $scope.charts['Contexts']['value']     = [ [ 0, chartInfo['Contexts'],     true, false ] ] ;
            $scope.charts['Transactions']['value'] = [ [ 0, chartInfo['Transaction'],  true, false ] ] ;
            $.each( updateStatus, function( key, status ){  //重置状态
               if( status == true )
                  updateStatus[key] = false ;
            } ) ;
            $scope.$digest() ;
         }
      }

      //获取存储过程数量
      var getProceduresNum = function(){
         var data = { 'cmd':'list procedures' } ;
         SdbRest.DataOperation2( clusterName, moduleName, data, {
            'success': function( proceduresList ){
               chartInfo['Procedures'] = proceduresList.length ;
               updateStatus['Procedures'] = true ;
               updateToCharts() ;
            },
            'failed': function( errorInfo ){
               updateStatus['Procedures'] = null ;
               updateToCharts() ;
               $timeout( getProceduresNum, 1000 ) ;
            }
         }, {
            'showLoading': false,
            'delay': 5000,
            'loop': 'success'
         } ) ;
      }

      //获取Session数量
      var getSessionsNum = function(){
         var sql ;
         if( moduleMode == 'standalone' )
            sql = 'select count(SessionID) as SessionNum from $SNAPSHOT_SESSION' ;
         else
            sql = 'select count(t1.SessionID) as SessionNum from (select * from $SNAPSHOT_SESSION split by ErrNodes) as t1 where t1.ErrNodes is null' ;
         SdbRest.Exec( sql, {
            'success': function( Num ){
               chartInfo['Sessions'] = Num[0]['SessionNum'] ;
               updateStatus['Sessions'] = true ;
               updateToCharts() ;
            },
            'failed': function( errorInfo ){
               updateStatus['Sessions'] = null ;
               updateToCharts() ;
               $timeout( getSessionsNum, 1000 ) ;
            }
         }, {
            'showLoading': false,
            'delay': 5000,
            'loop': 'success'
         } ) ;
      }
      

      //获取Context数量
      var getContextsNum = function(){
         var sql ;
         if( moduleMode == 'standalone' )
            sql = 'select count(SessionID) as ContextNum from $SNAPSHOT_CONTEXT' ;
         else
            sql = 'select count(t1.Contexts.ContextID) as ContextNum from (select * from $SNAPSHOT_CONTEXT split by ErrNodes) as t1 where t1.ErrNodes is null split by t1.Contexts' ;
         SdbRest.Exec( sql, {
            'success': function( Num ){
               chartInfo['Contexts'] = Num[0]['ContextNum'] ;
               updateStatus['Contexts'] = true ;
               updateToCharts() ;
            },
            'failed': function( errorInfo ){
               updateStatus['Contexts'] = null ;
               updateToCharts() ;
               $timeout( getContextsNum, 1000 ) ;
               
            }
         }, {
            'showLoading': false,
            'delay': 5000,
            'loop': 'success'
         } ) ;
      }
      

      //获取事务数量
      var getTransactionsNum = function(){
         var sql ;
         if( moduleMode == 'standalone' )
            sql = 'select count(TransactionID) as TransNum from $SNAPSHOT_TRANS' ;
         else
            sql = 'select count(t1.TransactionID) as TransNum from (select * from $SNAPSHOT_TRANS split by ErrNodes) as t1 where t1.ErrNodes is null' ;
         SdbRest.Exec( sql, {
            'success': function( Num ){
               chartInfo['Transaction'] = Num[0]['TransNum'] ;
               updateStatus['Transaction'] = true ;
               updateToCharts() ;
            },
            'failed': function( errorInfo ){
               updateStatus['Transaction'] = null ;
               updateToCharts() ;
               $timeout( getTransactionsNum, 1000 ) ;
            }
         }, {
            'showLoading': false,
            'delay': 5000,
            'loop': 'success'
         } ) ;
      }

      if( moduleMode == 'distribution' )
      {
         getProceduresNum() ;
      }

      getSessionsNum() ;
      getContextsNum() ;
      getTransactionsNum() ;

      //跳转至监控主页
      $scope.GotoModule = function(){
         $location.path( '/Monitor/Index' ).search( { 'r': new Date().getTime() } ) ;
      } ;

      //跳转至分区组列表
      $scope.GotoGroups = function(){
         $location.path( '/Monitor/SDB-Nodes/Groups' ).search( { 'r': new Date().getTime() } ) ;
      } ;

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