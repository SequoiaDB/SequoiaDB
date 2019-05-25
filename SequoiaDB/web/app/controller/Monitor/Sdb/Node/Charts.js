//@ sourceURL=Charts.js
(function(){
   var sacApp = window.SdbSacManagerModule ;
   //控制器
   sacApp.controllerProvider.register( 'Monitor.SdbOverview.NodesCharts.Ctrl', function( $scope, $location, SdbRest, SdbFunction ){
      
      _IndexPublic.checkMonitorEdition( $location ) ; //检测是不是企业版

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
      //图表
      $scope.charts = {} ; 
      $scope.charts['Insert'] = {} ;
      $scope.charts['Insert']['options'] = window.SdbSacManagerConf.RecordInsertEchart ;

      $scope.charts['Update'] = {} ;
      $scope.charts['Update']['options'] = window.SdbSacManagerConf.RecordUpdateEchart ;

      $scope.charts['Delete'] = {} ;
      $scope.charts['Delete']['options'] = window.SdbSacManagerConf.RecordDeleteEchart ;

      $scope.charts['Query'] = {} ;
      $scope.charts['Query']['options'] = window.SdbSacManagerConf.RecordReadEchart ;

      //获取数据库信息
      var getDbList = function(){
         var isFirstBuild = true ;
         var lastInfo = {} ;
         var sql = 'select sum(TotalInsert) as TotalInsert, sum(TotalUpdate) as TotalUpdate, sum(TotalDelete) as TotalDelete, sum(TotalRead) as TotalRead, sum(ReplUpdate) as ReplUpdate, sum(ReplDelete) as ReplDelete, sum(ReplInsert) as ReplInsert, ErrNodes from $SNAPSHOT_DB where role="data" group by ErrNodes' ;
         SdbRest.Exec( sql, {
            'success': function( dbList ){
               var currentInfo = { 'TotalInsert':0, 'TotalUpdate': 0, 'TotalDelete':0, 'TotalRead':0 } ;
               $.each( dbList, function( index, dbInfo ){
                  if( isArray( dbInfo['ErrNodes'] ) == false )
                  {
                     currentInfo['TotalInsert'] += dbInfo['TotalInsert'] - dbInfo['ReplInsert'] ;
                     currentInfo['TotalUpdate'] += dbInfo['TotalUpdate'] - dbInfo['ReplUpdate'] ;
                     currentInfo['TotalDelete'] += dbInfo['TotalDelete'] - dbInfo['ReplDelete'] ;
                     currentInfo['TotalRead']   += dbInfo['TotalRead'] ;
                  }
               } ) ;

               if( isFirstBuild == false )
               {
                  var tempInsert = 0 ;
                  var tempUpdate = 0 ;
                  var tempDelete = 0 ;
                  var tempRead = 0 ;

                  if( currentInfo['TotalInsert'] > lastInfo['TotalInsert'] )
                  {
                     tempInsert = ( currentInfo['TotalInsert'] - lastInfo['TotalInsert'] ) / 5 ;
                  }
                  if( currentInfo['TotalUpdate'] > lastInfo['TotalUpdate'] )
                  {
                     tempUpdate = ( currentInfo['TotalUpdate'] - lastInfo['TotalUpdate'] ) / 5 ;
                  }
                  if( currentInfo['TotalDelete'] > lastInfo['TotalDelete'] )
                  {
                     tempDelete = ( currentInfo['TotalDelete'] - lastInfo['TotalDelete'] ) / 5 ;
                  }
                  if( currentInfo['TotalRead'] > lastInfo['TotalRead'] )
                  {
                     tempRead = ( currentInfo['TotalRead'] - lastInfo['TotalRead'] ) / 5 ;
                  }

                  $scope.charts['Insert']['value'] = [ [ 0, tempInsert, true, false ] ] ;
                  $scope.charts['Update']['value'] = [ [ 0, tempUpdate, true, false ] ] ;
                  $scope.charts['Delete']['value'] = [ [ 0, tempDelete, true, false ] ] ;
                  $scope.charts['Query']['value']  = [ [ 0, tempRead, true, false ] ] ;
               }
               lastInfo['TotalInsert'] = currentInfo['TotalInsert'] ;
               lastInfo['TotalUpdate'] = currentInfo['TotalUpdate'] ;
               lastInfo['TotalDelete'] = currentInfo['TotalDelete'] ;
               lastInfo['TotalRead']   = currentInfo['TotalRead'] ;
               isFirstBuild = false ;
            }
         }, {
            'showLoading': false,
            'delay': 5000,
            'loop': true
         } ) ;
      }

      getDbList() ;

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
         $location.path( '/Monitor/SDB-Nodes/Nodes' ).search( { 'r': new Date().getTime() } ) ;
      }

   } ) ;

}());