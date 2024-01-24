//@ sourceURL=Index.js
(function(){
   var sacApp = window.SdbSacManagerModule ;
   //控制器
   sacApp.controllerProvider.register( 'Monitor.SdbNode.Index.Ctrl', function( $scope, $compile, SdbRest, $location, SdbFunction ){
      
      var clusterName = SdbFunction.LocalData( 'SdbClusterName' ) ;
      var moduleType = SdbFunction.LocalData( 'SdbModuleType' ) ;
      var moduleMode = SdbFunction.LocalData( 'SdbModuleMode' ) ;
      var moduleName = SdbFunction.LocalData( 'SdbModuleName' ) ;
      var hostname = SdbFunction.LocalData( 'SdbHostName' ) ;
      var svcname = SdbFunction.LocalData( 'SdbServiceName' ) ;
      if( clusterName == null || moduleType != 'sequoiadb' || moduleMode == null || moduleName == null )
      {
         $location.path( '/Transfer' ).search( { 'r': new Date().getTime() } ) ;
         return;
      }

      if( moduleMode == 'distribution' && ( hostname == null || svcname == null ) )
      {
         $location.path( '/Monitor/SDB/Index' ).search( { 'r': new Date().getTime() } ) ;
         return;
      }

      //初始化
      $scope.ModuleMode = moduleMode ;
      //节点角色（集群模式才有效）
      var nodeRole ;
      $scope.TotalRecords = 0 ;
      $scope.TotalLobs = 0 ;
      $scope.TotalCl = 0 ;

      $scope.charts = {};
      $scope.charts['Insert'] = {} ;
      $scope.charts['Insert']['options'] = window.SdbSacManagerConf.RecordInsertEchart ;

      $scope.charts['Update'] = {} ;
      $scope.charts['Update']['options'] = window.SdbSacManagerConf.RecordUpdateEchart ;

      $scope.charts['Delete'] = {} ;
      $scope.charts['Delete']['options'] = window.SdbSacManagerConf.RecordDeleteEchart ;

      $scope.charts['Query'] = {} ;
      $scope.charts['Query']['options'] = window.SdbSacManagerConf.RecordReadEchart ;

      //图表信息
      var getChartInfo = function(){
         var sql ;
         var isInit = false ;
         var lastInfo = {} ;
         if( nodeRole == 1 )
            sql = 'SELECT TotalInsert, TotalRead, TotalDelete, TotalUpdate FROM $SNAPSHOT_DB WHERE GLOBAL=false' ;
         else if( moduleMode == 'distribution' )
            sql = 'SELECT TotalInsert, TotalRead, TotalDelete, TotalUpdate FROM $SNAPSHOT_DB WHERE HostName="' + hostname +'" AND svcname="'+ svcname + '"' ;
         else
            sql = 'SELECT TotalInsert, TotalRead, TotalDelete, TotalUpdate FROM $SNAPSHOT_DB' ;
         SdbRest.Exec( sql, {
            'before': function( jqXHR ){
               if( nodeRole == 1 )
               {
                  jqXHR.setRequestHeader( 'SdbHostName', hostname ) ;
                  jqXHR.setRequestHeader( 'SdbServiceName', svcname ) ;
               }
            },
            'success': function( data ){
               if( data.length == 0 )
               {
                  return ;
               }
               dbInfo = data[0] ;
               if( isInit == true )
               {
                  var tempInsert = 0 ;
                  var tempUpdate = 0 ;
                  var tempDelete = 0 ;
                  var tempRead = 0 ;

                  if( dbInfo['TotalInsert'] > lastInfo['TotalInsert'] )
                  {
                     tempInsert = ( dbInfo['TotalInsert'] - lastInfo['TotalInsert'] ) / 5 ;
                  }
                  if( dbInfo['TotalUpdate'] > lastInfo['TotalUpdate'] )
                  {
                     tempUpdate = ( dbInfo['TotalUpdate'] - lastInfo['TotalUpdate'] ) / 5 ;
                  }
                  if( dbInfo['TotalDelete'] > lastInfo['TotalDelete'] )
                  {
                     tempDelete = ( dbInfo['TotalDelete'] - lastInfo['TotalDelete'] ) / 5 ;
                  }
                  if( dbInfo['TotalRead'] > lastInfo['TotalRead'] )
                  {
                     tempRead = ( dbInfo['TotalRead'] - lastInfo['TotalRead'] ) / 5 ;
                  }

                  $scope.charts['Insert']['value'] = [ [ 0, tempInsert, true, false ] ] ;
                  $scope.charts['Update']['value'] = [ [ 0, tempUpdate, true, false ] ] ;
                  $scope.charts['Delete']['value'] = [ [ 0, tempDelete, true, false ] ] ;
                  $scope.charts['Query']['value']  = [ [ 0, tempRead, true, false ] ] ;
               }
               lastInfo['TotalInsert'] = dbInfo['TotalInsert'] ;
               lastInfo['TotalUpdate'] = dbInfo['TotalUpdate'] ;
               lastInfo['TotalDelete'] = dbInfo['TotalDelete'] ;
               lastInfo['TotalRead']   = dbInfo['TotalRead'] ;
               isInit = true ;
            }
         }, {
            'showLoading': false,
            'delay': 5000,
            'loop': true
         } ) ;
      } ;

      //获取数据库快照
      var getDbInfo = function(){
         var sql ;
         if( nodeRole == 1 )
            sql = 'SELECT NodeName, Role, IsPrimary, GroupName, HostName, Disk, NodeID, ServiceStatus, TotalInsert, TotalRead, TotalDelete, TotalUpdate FROM $SNAPSHOT_DB WHERE GLOBAL=false' ;
         else if( moduleMode == 'distribution' )
            sql = 'SELECT NodeName, Role, IsPrimary, GroupName, HostName, Disk, NodeID, ServiceStatus, CurrentLSN, CompleteLSN, LSNQueSize, TotalInsert, TotalRead, TotalDelete, TotalUpdate FROM $SNAPSHOT_DB WHERE HostName="' + hostname +'" AND svcname="'+ svcname + '"' ;
         else
            sql = 'SELECT NodeName, Role, IsPrimary, GroupName, HostName, Disk, NodeID, ServiceStatus, TotalInsert, TotalRead, TotalDelete, TotalUpdate FROM $SNAPSHOT_DB' ;
         SdbRest.Exec( sql, {
            'before': function( jqXHR ){
               if( nodeRole == 1 )
               {
                  jqXHR.setRequestHeader( 'SdbHostName', hostname ) ;
                  jqXHR.setRequestHeader( 'SdbServiceName', svcname ) ;
               }
            },
            'success': function( dbInfo ){
               if( dbInfo.length > 0 )
               {
                  $scope.nodeInfo = dbInfo[0] ;
                  if( isArray( $scope.nodeInfo['ErrNodes'] ) == false ) //连接正常
                  {
                     if( moduleMode == 'distribution' &&
                         nodeRole == 1 &&
                         $scope.nodeInfo['NodeName'] != hostname + ':' + svcname ) //om虽然连接成功，但是已经切换到其他coord节点了，所以还是失败的
                     {
                        $scope.nodeInfo['Flag'] = -15 ;
                        $scope.nodeInfo['NodeName'] = hostname + ':' + svcname ;
                        $scope.nodeInfo['HostName'] = hostname ;
                        $scope.nodeInfo['GroupName'] = 'SYSCoord' ;
                        $scope.nodeInfo['Role'] = 'coord' ;
                        $scope.nodeInfo['NodeID'] = '-' ;
                        $scope.nodeInfo['Disk'] = { 'DatabasePath': '-' } ;
                     }  
                     else
                     {
                        $scope.nodeInfo['Flag'] = 0 ;
                        $scope.nodeInfo['TotalRecords'] = $scope.TotalRecords ;
                        $scope.nodeInfo['TotalLobs'] = $scope.TotalLobs ;
                        $scope.nodeInfo['TotalCl'] = $scope.TotalCl ;
                        $scope.nodeInfo['NodeID'] = $scope.nodeInfo['NodeID'][1] ;
                        getChartInfo() ;
                     }
                  }
                  else
                  {
                     $scope.nodeInfo['Flag'] = $scope.nodeInfo['ErrNodes'][0]['Flag'] ;
                     $scope.nodeInfo['NodeName'] = $scope.nodeInfo['ErrNodes'][0]['NodeName'] ;
                     $scope.nodeInfo['HostName'] = $scope.nodeInfo['ErrNodes'][0]['HostName'] ;
                     $scope.nodeInfo['GroupName'] = $scope.nodeInfo['ErrNodes'][0]['GroupName'] ;
                     if( $scope.nodeInfo['GroupName'] == 'SYSCoord' )
                        $scope.nodeInfo['Role'] = 'coord' ;
                     else if( $scope.nodeInfo['GroupName'] == 'SYSCatalogGroup' )
                        $scope.nodeInfo['Role'] = 'catalog' ;
                     else
                        $scope.nodeInfo['Role'] = 'data' ;
                     $scope.nodeInfo['NodeID'] = $scope.nodeInfo['ErrNodes'][0]['NodeID'] ;
                     $scope.nodeInfo['Disk'] = { 'DatabasePath': '-' } ;
                     $scope.nodeInfo['IsPrimary'] = 'unknow' ;
                     $scope.nodeInfo['TotalRecords'] = '-' ;
                     $scope.nodeInfo['TotalLobs'] = '-' ;
                     $scope.nodeInfo['TotalCl'] = '-' ;
                  }
               }
            }, 
            'failed':function( errorInfo ){
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  getDbInfo() ;
                  return true ;
               } ) ;
            }
         },{ 'showLoading': false } ) ;
      }

      //获取集合列表
      var getClList = function(){
         var sql ;
         if( nodeRole == 1 )
            sql = 'SELECT t1.ErrNodes, t1.Name, t1.Details.TotalRecords, t1.Details.TotalLobs, t1.Details.NodeName FROM (SELECT Name, Details, ErrNodes FROM $SNAPSHOT_CL WHERE GLOBAL=false SPLIT By Details) As t1' ;
         else if( moduleMode == 'distribution' )
            sql = 'SELECT t1.ErrNodes, t1.Name, t1.Details.TotalRecords, t1.Details.TotalLobs, t1.Details.NodeName FROM (SELECT Name, Details, ErrNodes FROM $SNAPSHOT_CL WHERE HostName="' + hostname +'" AND svcname="'+ svcname + '" SPLIT By Details) As t1' ;
         else
            sql = 'SELECT t1.ErrNodes, t1.Name, t1.Details.TotalRecords, t1.Details.TotalLobs, t1.Details.NodeName FROM (SELECT Name, Details, ErrNodes FROM $SNAPSHOT_CL SPLIT By Details) As t1' ;
         SdbRest.Exec( sql, {
            'before': function( jqXHR ){
               if( nodeRole == 1 )
               {
                  jqXHR.setRequestHeader( 'SdbHostName', hostname ) ;
                  jqXHR.setRequestHeader( 'SdbServiceName', svcname ) ;
               }
            },
            'success': function( clList ){
               $.each( clList, function( index, clInfo ){
                  if( isArray( clInfo ) == false )
                  {
                     $scope.TotalRecords += clInfo['TotalRecords'] ;
                     $scope.TotalLobs += clInfo['TotalLobs'] ;
                  }
               } ) ;
               $scope.TotalCl = clList.length ;
               $scope.ClList = clList ;
               getDbInfo() ;
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
      }

      //获取节点角色
      var getNodeRole = function(){
         var sql = 'select t1.Role from (select * from $LIST_GROUP split by Group) as t1 where t1.Group.HostName="' + hostname +'" and t1.Group.Service.0.Name="'+ svcname + '"' ;
         SdbRest.Exec( sql, {
            'success': function( roleInfo ){
               if( roleInfo.length > 0 )
               {
                  nodeRole = roleInfo[0]['Role'] ;
                  getClList() ;
               }
            },
            'failed': function( errorInfo ){
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  getNodeRole() ;
                  return true ;
               } ) ;
            }
         },{ 'showLoading': false } ) ;
      }

      if( moduleMode == 'distribution' )
         getNodeRole() ;
      else
         getClList() ;
      
      //跳转至分区组
      $scope.GotoGroup = function( GroupName ){
         SdbFunction.LocalData( 'SdbGroupName', GroupName ) ;
         $location.path( '/Monitor/SDB-Nodes/Group/Index' ).search( { 'r': new Date().getTime() } ) ;
      }

      //跳转至资源
      $scope.GotoResources = function(){
         $location.path( '/Monitor/SDB-Resources/Session' ).search( { 'r': new Date().getTime() } ) ;
      }

      //跳转至主机列表
      $scope.GotoHosts = function(){
         $location.path( '/Monitor/SDB-Host/List/Index' ).search( { 'r': new Date().getTime() } ) ;
      }

      //跳转至主机信息
      $scope.GotoHostInfo = function( HostName ){
         SdbFunction.LocalData( 'SdbHostName', HostName ) ;
         $location.path( '/Monitor/SDB-Host/Info/Index' ).search( { 'r': new Date().getTime() } ) ;
      }
      
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
      }

   } ) ;
}());