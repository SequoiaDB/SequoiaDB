(function(){
   var sacApp = window.SdbSacManagerModule ;
   //控制器
   sacApp.controllerProvider.register( 'Monitor.SdbNode.Index.Ctrl', function( $scope, $compile, SdbRest, $location, SdbFunction ){
      $scope.ClusterName = SdbFunction.LocalData( 'SdbClusterName' ) ;
      $scope.ModuleType = SdbFunction.LocalData( 'SdbModuleType' ) ;
      $scope.ModuleName = SdbFunction.LocalData( 'SdbModuleName' ) ;
      var HostName = SdbFunction.LocalData( 'SdbHostName' ) ;
      var svcname = SdbFunction.LocalData( 'SdbServiceName' ) ;
      $scope.TotalRecords = 0 ;
      $scope.TotalLobs = 0 ;
      $scope.TotalCl = 0 ;
      var s = 0 ;
      var sql = '' ;

      //节点信息，后期根据 跳转函数 获取节点名（主机名+端口号）
      

      var getChartInfo = function(){
         var chartInfo = { 'TotalInsert':0, 'TotalUpdate': 0, 'TotalDelete':0, 'TotalRead':0 } ;
         var SumInfo = {} ;
         sql = 'SELECT TotalInsert, TotalRead, TotalDelete, TotalUpdate FROM $SNAPSHOT_DB WHERE HostName="' + HostName +'" AND svcname="'+ svcname + '"' ;
         SdbRest.Exec( sql, { 
            'success': function( DbInfo ){
               DbInfo = DbInfo[0] ;
               if( typeof( SumInfo['TotalInsert'] ) == 'undefined' )
               {
                  SumInfo['TotalInsert'] = DbInfo['TotalInsert'] ;
                  SumInfo['TotalUpdate'] = DbInfo['TotalUpdate'] ;
                  SumInfo['TotalDelete'] = DbInfo['TotalDelete'] ;
                  SumInfo['TotalRead'] = DbInfo['TotalRead'] ;
               }
               else
               {
                  $scope.charts['Insert']['value'] = [ [ 0, ( DbInfo['TotalInsert'] - SumInfo['TotalInsert'] )/5, true, false ] ] ;
                  $scope.charts['Update']['value'] = [ [ 0, ( DbInfo['TotalUpdate'] - SumInfo['TotalUpdate'] )/5, true, false ] ] ;
                  $scope.charts['Delete']['value'] = [ [ 0, ( DbInfo['TotalDelete'] - SumInfo['TotalDelete'] )/5, true, false ] ] ;
                  $scope.charts['Query']['value'] = [ [ 0, ( DbInfo['TotalRead'] - SumInfo['TotalRead'] )/5, true, false ] ] ;

                  SumInfo['TotalInsert'] = DbInfo['TotalInsert'] ;
                  SumInfo['TotalUpdate'] = DbInfo['TotalUpdate'] ;
                  SumInfo['TotalDelete'] = DbInfo['TotalDelete'] ;
                  SumInfo['TotalRead'] = DbInfo['TotalRead'] ;
               }

            },
            'failed': function( errorInfo ){
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  getDbInfo() ;
                  return true ;
               } ) ;
            }
         }, {
            'showLoading': false,
            'delay': 5000,
            'loop': true
         } ) ;
      } ;

      var getDbInfo = function(){
         sql = 'SELECT NodeName, Role, IsPrimary, GroupName, HostName, Disk, NodeID, ServiceStatus, TotalInsert, TotalRead, TotalDelete, TotalUpdate FROM $SNAPSHOT_DB WHERE HostName="' + HostName +'" AND svcname="'+ svcname + '"' ;
         SdbRest.Exec( sql, {
            'success': function( DbInfo ){
               $scope.nodeInfo = DbInfo[0] ;
               //LSN无法获取
               $scope.nodeInfo['TotalRecords'] = $scope.TotalRecords ;
               $scope.nodeInfo['TotalLobs'] = $scope.TotalLobs ;
               $scope.nodeInfo['TotalCl'] = $scope.TotalCl ;
               getChartInfo()
            }, 
            'failed':function( errorInfo ){
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  getDbInfo() ;
                  return true ;
               } ) ;
            }
         } ) ;
      } ;

      var getClList = function(){
         sql = 'SELECT t1.Name, t1.Details.TotalRecords, t1.Details.TotalLobs, t1.Details.NodeName FROM (SELECT Name, Details FROM $SNAPSHOT_CL WHERE HostName="' + HostName +'" AND svcname="'+ svcname + '"' + ' SPLIT By Details) As t1'
         SdbRest.Exec( sql, {
            'success': function( ClList ){
               $.each( ClList, function( index, ClInfo ){
                  $scope.TotalRecords += ClInfo['TotalRecords'] ;
                  $scope.TotalLobs += ClInfo['TotalLobs'] ;
               } ) ;
               $scope.TotalCl = ClList.length ;

               $scope.ClList = ClList ;
               getDbInfo()
            },
            'failed': function( errorInfo ){
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  getClInfo() ;
                  return true ;
               } ) ;
            }
         } ) ;
      } ;

      getClList()
      

      $scope.charts = {};
      $scope.charts['Insert'] = {} ;
      $scope.charts['Insert']['options'] = window.SdbSacManagerConf.RecordInsertEchart ;

      $scope.charts['Update'] = {} ;
      $scope.charts['Update']['options'] = window.SdbSacManagerConf.RecordUpdateEchart ;

      $scope.charts['Delete'] = {} ;
      $scope.charts['Delete']['options'] = window.SdbSacManagerConf.RecordDeleteEchart ;

      $scope.charts['Query'] = {} ;
      $scope.charts['Query']['options'] = window.SdbSacManagerConf.RecordReadEchart ;

      //跳转至分区组
      $scope.GotoGroup = function( GroupName ){
         SdbFunction.LocalData( 'SdbGroupName', GroupName ) ;
         $location.path( '/Monitor/SDB-Group/Index' ) ;
      } ;

      //跳转至资源
      $scope.GotoResources = function(){
         $location.path( '/Monitor/SDB-Resources/Domain' ) ;
      } ;

      //跳转至主机列表
      $scope.GotoHosts = function(){
         $location.path( '/Monitor/Host-List/Index' ) ;
      } ;
      
      
      //跳转至节点列表
      $scope.GotoNodes = function(){
         $location.path( '/Monitor/SDB-Nodes/Nodes' ) ;
      } ;
   } ) ;
}());