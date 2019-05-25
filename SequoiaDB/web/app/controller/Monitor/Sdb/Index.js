//@ sourceURL=Index.js
(function(){
   var sacApp = window.SdbSacManagerModule ;
   //控制器
   sacApp.controllerProvider.register( 'Monitor.Preview.Index.Ctrl', function( $scope, $compile, $location, SdbRest, SdbFunction ){
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
      $scope.SysConfig  = window.Config ;    //数据库版本
      $scope.ModuleMode = moduleMode ;       //集群模式
      $scope.ModuleName = moduleName ;       //业务名
      var standaloneHostName = '' ;          //如果是单机版，则需要记录当前的节点主机和端口
      var standaloneSvcname = '' ;
      $scope.DBStatus ;                      //数据库当前健康状态， normal:正常, warning:有节点故障, error:数据库不可用
      $scope.ErrorMsg = '' ;                 //数据库状态的文字描述
      $scope.charts = {} ;                   //图表
      $scope.charts['Host'] = {} ;           //主机信息的图表
      $scope.charts['Module'] = {} ;         //业务信息的图表
      $scope.charts['Module']['options'] = window.SdbSacManagerConf.RecordInsertEchart ;
      $scope.charts['Module']['value'] = [ 0, 0, true, false ] ;
      $scope.moduleInfo = {                  //同步到界面的信息
         'version': '-',
         'sessions': '-',
         'domains': '-',
         'groups': '-',
         'nodes': '-',
         'cl': '-',
         'records': '-',
         'lobs': '-',
         'cpuUse': 0,
         'memoryUse': 0,
         'diskUse': 0
      } ;
      $scope.HostNum = '-' ;                 //主机数量
      $scope.DiskNum = '-' ;                 //磁盘数量
      $scope.chooseCharts = 'Insert' ;       //默认展示图表的类型
      $scope.chartName = 'Record Insert' ;   //业务信息的图表名字
      
      $scope.ErrResult = [] ;                //显示在界面的告警列表
      $scope.Time = '' ;                     //当前时间的字符串
      var hostJson = [] ;                    //数据库的主机列表
      var errNodes = [] ;                    //错误的节点列表
      var errMsgs = [] ;                     //执行查询，返回失败的信息列表
      var databaseStatus = [] ;              //数据库每个组的状态

      //获取时间
      var getTime = function(){
         var date = new Date();
         var year = date.getFullYear() ;
         var month = date.getMonth() + 1 ;
         var day = date.getDate() ;
         var hour = date.getHours() ;
         var minute = date.getMinutes() ;
         var second = date.getSeconds() ;
         $scope.Time = year + '-' + pad( month, 2 ) + '-' + pad( day, 2 ) + ' ' + pad( hour, 2 ) + ':' + pad( minute, 2 ) + ':' + pad( second, 2 ) ;
      }
      getTime() ;

      //获取集合列表
      var getClList = function(){
         var cls = [] ;
         var sql = 'SELECT t1.Name,t1.Details.TotalRecords,t1.Details.TotalLobs FROM (SELECT Name, Details FROM $SNAPSHOT_CL WHERE NodeSelect = "master" SPLIT By Details) As t1' ;
         SdbRest.Exec( sql, {
            'success': function( clList ){
               $scope.moduleInfo['cl'] = clList.length ;
               $scope.moduleInfo['lobs'] = 0 ;
               $scope.moduleInfo['records'] = 0 ;
               $.each( clList, function( index, clInfo ){
                  if( clInfo['Name'] != null && clInfo['TotalRecords'] != null )
                  {
                     $scope.moduleInfo['lobs'] += clInfo['TotalLobs'] ;
                     $scope.moduleInfo['records'] += clInfo['TotalRecords'] ;
                  }
               } ) ;
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
      getClList() ;

      //设置系统状态信息
      var setStatusMsg = function(){
         getTime() ; //获取时间
         $scope.ErrResult = [] ;
         //错误节点
         $.each( errNodes, function( index, nodeInfo ){
            $scope.ErrResult.push( { 'info': sprintf( $scope.autoLanguage( '? [error] - 节点错误: ?，错误码 ?。' ), $scope.Time, nodeInfo['NodeName'], nodeInfo['Flag'] ), 'type': 'error' } ) ;
         } ) ;

         //cpu
         if( $scope.moduleInfo['cpuUse'] > 90 )
         {
            $scope.ErrResult.push( { 'info': sprintf( $scope.autoLanguage( '? [warning] - CPU使用超过90%。' ), $scope.Time ), 'type': 'warning' } ) ;
         }

         //memory
         if( $scope.moduleInfo['memoryUse'] > 90 )
         {
            $scope.ErrResult.push( { 'info': sprintf( $scope.autoLanguage( '? [warning] - 内存使用超过90%。' ), $scope.Time ), 'type': 'warning' } ) ;
         }

         //disk
         if( $scope.moduleInfo['diskUse'] > 90 )
         {
            $scope.ErrResult.push( { 'info': sprintf( $scope.autoLanguage( '? [warning] - 磁盘使用超过90%。' ), $scope.Time ), 'type': 'warning' } ) ;
         }

         //判断系统状态
         $scope.ErrorMsg = '' ;
         $scope.DBStatus = 'normal' ;
         $.each( databaseStatus, function( index, groupStatus ){
            var errNodeNum = 0 ;
            $.each( groupStatus['ServiceStatus'], function( index2, nodeStatus ){
               if( nodeStatus == false )
               {
                  ++errNodeNum ;
                  $scope.DBStatus = 'warning' ;
                  $scope.ErrorMsg = sprintf( $scope.autoLanguage( '分区组 ? 有节点异常。' ), groupStatus['GroupName'] ) ;

                  if( groupStatus['NodeName'] && groupStatus['NodeName'].length > index2 &&
                      groupStatus['Status'] && groupStatus['Status'].length > index2 )
                  {
                     $scope.ErrResult.push( {
                        'info': sprintf( $scope.autoLanguage( '? [warning] - 节点 ? 正在 ?。' ),
                                         $scope.Time,
                                         groupStatus['NodeName'][index2],
                                         groupStatus['Status'][index2] ),
                        'type': 'warning' } ) ;
                  }
               }
            } ) ;
            if( errNodeNum == groupStatus['ServiceStatus'].length )
            {
               $scope.DBStatus = 'error' ;
               $scope.ErrorMsg = sprintf( $scope.autoLanguage( '分区组 ? 所有节点异常。' ), groupStatus['GroupName'] ) ;
            }
         } ) ;

         //查询信息错误

         if( errMsgs.length > 0 && $scope.DBStatus != 'error' )
         {
            $scope.DBStatus = 'error' ;
         }

         $.each( errMsgs, function( index, msgInfo ){
            $scope.ErrorMsg = msgInfo['description'] ;
            $scope.ErrResult.push( { 'info': sprintf( '? [error] - ?, errno: ?。', $scope.Time, msgInfo['description'], msgInfo['errno'] ), 'type': 'error' } ) ;
         } ) ;
         errMsgs = [] ;
      }

      //获取数据库状态
      var getDBStatus = function(){
         var groupList = [] ;
         var getGroupListIndex = function( groupName ){
            var groupIndex = -1 ;
            $.each( groupList, function( index, groupInfo ){
               if( groupInfo['GroupName'] == groupName )
               {
                  groupIndex = index ;
                  return false ;
               }
            } ) ;
            return groupIndex ;
         }
         var sql = 'select push(ServiceStatus) as ServiceStatus, push( Status ) as Status, push( NodeName ) as NodeName, GroupName, ErrNodes from $SNAPSHOT_DB group by GroupName order by ErrNodes asc' ;
         SdbRest.Exec( sql, {
            'before': function(){
               groupList = [] ;
            },
            'success': function( nodeList ){
               $.each( nodeList, function( index, nodeInfo ){
                  if( nodeInfo['ErrNodes'] == null )
                  {
                     groupList.push( nodeInfo ) ;
                  }
                  else
                  {
                     $.each( nodeInfo['ErrNodes'], function( index2, errNodeInfo ){
                        var groupIndex = getGroupListIndex( errNodeInfo['GroupName'] ) ;
                        if( groupIndex >= 0 )
                        {
                           groupList[groupIndex]['ServiceStatus'].push( false ) ;
                        }
                        else
                        {
                           groupList.push( { 'ServiceStatus': [ false ], 'GroupName': errNodeInfo['GroupName'], 'ErrNodes': null } ) ;
                        }
                     } ) ;
                  }
               } ) ;
               databaseStatus = groupList ;
            },
            'failed': function( errorInfo ){
               errMsgs.push( errorInfo ) ;
            }
         },{
            'showLoading': false,
            'delay': 5000,
            'loop': true
         } ) ;
      }
      if( moduleMode == 'distribution' )
         getDBStatus() ;

      //获取数据库性能信息
      var getDbList = function(){
         var SumInfo = {} ;
         var sql = 'select sum(TotalInsert) as TotalInsert, sum(TotalUpdate) as TotalUpdate, sum(TotalDelete) as TotalDelete, sum(TotalRead) as TotalRead, sum(ReplUpdate) as ReplUpdate, sum(ReplDelete) as ReplDelete, sum(ReplInsert) as ReplInsert, ErrNodes from $SNAPSHOT_DB where role="data" group by ErrNodes' ;
         SdbRest.Exec( sql, {
            'success': function( dbList ){
               errNodes = [] ;
               $scope.DbInfo = { 'TotalInsert':0, 'TotalUpdate': 0, 'TotalDelete':0, 'TotalRead':0 } ;
               $.each( dbList, function( index, DbInfo ){
                  if( DbInfo['ErrNodes'] !== null )
                  {
                     $.each( DbInfo['ErrNodes'], function( errIndex, errInfo ){
                        errNodes.push( errInfo ) ;
                     } )
                  }
                  else
                  {
                     $scope.DbInfo['TotalInsert'] += DbInfo['TotalInsert'] - DbInfo['ReplInsert'] ;
                     $scope.DbInfo['TotalUpdate'] += DbInfo['TotalUpdate'] - DbInfo['ReplUpdate'] ;
                     $scope.DbInfo['TotalDelete'] += DbInfo['TotalDelete'] - DbInfo['ReplDelete'] ;
                     $scope.DbInfo['TotalRead']   += DbInfo['TotalRead'] ;
                  }
               } ) ;

               if( typeof( SumInfo['TotalInsert'] ) == 'undefined' )
               {
                  SumInfo['TotalInsert'] = $scope.DbInfo['TotalInsert'] ;
                  SumInfo['TotalUpdate'] = $scope.DbInfo['TotalUpdate'] ;
                  SumInfo['TotalDelete'] = $scope.DbInfo['TotalDelete'] ;
                  SumInfo['TotalRead'] = $scope.DbInfo['TotalRead'] ;
               }
               else
               {
                  var diff = 0 ;
                  if( $scope.chooseCharts == 'Insert' )
                  {
                     diff = ( $scope.DbInfo['TotalInsert'] - SumInfo['TotalInsert'] ) / 5 ;
                  }
                  else if( $scope.chooseCharts == 'Read' )
                  {
                     diff = ( $scope.DbInfo['TotalRead'] - SumInfo['TotalRead'] ) / 5 ;
                  }
                  else if( $scope.chooseCharts == 'Delete' )
                  {
                     diff = ( $scope.DbInfo['TotalDelete'] - SumInfo['TotalDelete'] ) / 5 ;
                  }
                  else if( $scope.chooseCharts == 'Update' )
                  {
                     diff = ( $scope.DbInfo['TotalUpdate'] - SumInfo['TotalUpdate'] ) / 5 ;
                  }
                  if( diff < 0 )
                     diff = 0 ;
                  $scope.charts['Module']['value'] = [ [ 0, diff, true, false ] ] ;
                  SumInfo['TotalInsert'] = $scope.DbInfo['TotalInsert'] ;
                  SumInfo['TotalUpdate'] = $scope.DbInfo['TotalUpdate'] ;
                  SumInfo['TotalDelete'] = $scope.DbInfo['TotalDelete'] ;
                  SumInfo['TotalRead'] = $scope.DbInfo['TotalRead'] ;
               }
            },
            'failed': function( errorInfo ){
               errMsgs.push( errorInfo ) ;
            }
         },{
            'showLoading': false,
            'delay': 5000,
            'loop': true
         } ) ;
      } ;
      getDbList() ;
      
      //获取主机性能快照
      var queryHostSnapshot = function(){
         var data = {
            'cmd': 'query host status',
            'HostInfo': JSON.stringify( { 'HostInfo': hostJson } )
         }
         var lastCPU = null ;
         SdbRest.OmOperation( data, {
            'success': function( info ){
               if( info.length > 0 )
               {
                  //cpu
                  if( lastCPU === null )
                  {
                     lastCPU = getCpuUsePercent( info[0]['HostInfo'] ) ;
                  }
                  else
                  {
                     $scope.moduleInfo['cpuUse'] = getCpuUsePercent( info[0]['HostInfo'], lastCPU ) ;
                     $scope.charts['Host']['CPU'] = { 'percent': $scope.moduleInfo['cpuUse'], 'style': { 'progress': { 'background': '#FF9933' } } } ;
                  }
               
                  //memory
                  $scope.moduleInfo['memoryUse'] = getMemoryUsePercent( info[0]['HostInfo'] ) ;
                  $scope.charts['Host']['Memory'] = { 'percent': $scope.moduleInfo['memoryUse'], 'style': { 'progress': { 'background': '#D9534F' } } } ;

                  //disk
                  $scope.moduleInfo['diskUse'] = getDiskUsePercent( info[0]['HostInfo'] ) ;
                  $scope.charts['Host']['Disk'] = { 'percent': $scope.moduleInfo['diskUse'] } ;
               }
            },
            'complete': function(){
               setStatusMsg() ;
            }
         }, {
            'showLoading': false,
            'delay': 5000,
            'loop': true
         } ) ;
      }

      //获取磁盘数
      var queryDisk = function( HostList ){
         var data = {
            'cmd': 'query host',
            'filter': JSON.stringify( { '$or': HostList } ),
            'selector': JSON.stringify( { 'HostName': 1, 'Disk': 1 } )
         } ;
         SdbRest.OmOperation( data, {
            'success': function( hostList ){
               $scope.DiskNum = 0 ;
               $.each( hostList, function( index, value ){
                  $scope.DiskNum = $scope.DiskNum + value['Disk'].length ;
               } ) ;
            },
            'failed': function( errorInfo ){
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  queryDisk( HostList ) ;
                  return true ;
               } ) ;
            }
         } ) ;
      }

      //获取主机列表
      var queryHost = function(){
         var sql = 'select HostName, ServiceName from $SNAPSHOT_DB group by HostName' ;
         SdbRest.Exec( sql, {
            'success': function( hostList ){
               $scope.HostNum = 0 ;
               $.each( hostList, function( index, hostInfo ){
                  if( typeof( hostInfo['HostName'] ) == 'string' )
                  {
                     ++$scope.HostNum ;
                     hostJson.push( { 'HostName': hostInfo['HostName'] } ) ;
                     standaloneHostName = hostInfo['HostName'] ;
                     standaloneSvcname  = hostInfo['ServiceName'] ;
                  }               
               } ) ;
               queryDisk( hostJson ) ;
               queryHostSnapshot() ;
            }, 
            'failed': function( errorInfo ){
               errMsgs.push( errorInfo ) ;
            }
         } ) ;
      }
      queryHost() ;

      //获取集群版本
      var getVersion = function(){
         var sql = 'select Version from $SNAPSHOT_DB group by Version' ;
         SdbRest.Exec( sql, {
            'success': function( versionList ){
               $scope.moduleInfo['version'] = '' ;
               var versionRemoval = {} ;
               $.each( versionList, function( index, versionInfo ){
                  if( typeof( versionInfo['Version'] ) == 'object' )
                  {
                     var versionStr = versionInfo['Version']['Major'] + '.' + versionInfo['Version']['Minor'] ;
                     if( versionInfo['Version']['Fix'] > 0 )
                     {
                        versionStr += '.' + versionInfo['Version']['Fix'] ;
                     }
                     if( versionRemoval[versionStr] === 1 )
                     {
                        return true ;
                     }
                     if( index > 0 )
                     {
                        $scope.moduleInfo['version'] += ', ' ;
                     }
                     versionRemoval[versionStr] = 1 ;
                     $scope.moduleInfo['version'] += versionStr ;
                  }
               } ) ;
            },
            'failed': function( errorInfo ){
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  getVersion() ;
                  return true ;
               } ) ;
            }
         },{
            'showLoading': false
         } ) ;
      }
      getVersion() ;

      //获取会话数
      var getSessions = function(){
         var sql = 'select count(SessionID) as SessionNum from $SNAPSHOT_SESSION' ;
         SdbRest.Exec( sql, {
            'success': function( SessionNum ){
               $scope.moduleInfo['sessions'] = SessionNum[0]['SessionNum'] ;
            },
            'failed': function( errorInfo ){
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  getSessions() ;
                  return true ;
               } ) ;
            }
         },{
            'showLoading': false
         } ) ;
      }
      getSessions();

      //获取组信息
      var getGroups = function(){
         var data = { 'cmd': 'list groups' } ;
         SdbRest.DataOperation( data, {
            'success': function( groups ){
               $scope.moduleInfo['groups'] = groups.length ;
               $scope.moduleInfo['nodes'] = 0 ;
               $.each( groups, function( index, value ){
                  $scope.moduleInfo['nodes'] += value['Group'].length ;
               } )
            },
            'failed': function( errorInfo ){
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  getGroups() ;
                  return true ;
               } ) ;
            }
         } ) ;
      } ;
      if( moduleMode == 'distribution' )
         getGroups() ;

      //获取域数量
      var getDomains = function(){
         var data = { 'cmd': 'list domains' } ;
         SdbRest.DataOperation( data, {
            'success': function( domains ){
               $scope.moduleInfo['domains'] = domains.length ;
            },
            'failed': function( errorInfo ){
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  getDomains() ;
                  return true ;
               } ) ;
            }
         } ) ;
      } ;
      if( moduleMode == 'distribution' )
         getDomains();
         
      //选择图表
      $scope.changeCharts = function( type ){
         $scope.chooseCharts = type ;
         $scope.charts['Module'] = {} ;
         if( type == 'Insert' )
         {
            $scope.chartName = 'Record Insert' ;
            $scope.charts['Module']['options'] = window.SdbSacManagerConf.RecordInsertEchart ;
         }
         else if( type == 'Read' )
         {
            $scope.chartName = 'Record Read' ;
            $scope.charts['Module']['options'] = window.SdbSacManagerConf.RecordReadEchart ;
         }
         else if( type == 'Delete' )
         {
            $scope.chartName = 'Record Delete' ;
            $scope.charts['Module']['options'] = window.SdbSacManagerConf.RecordDeleteEchart ;
         }
         else if( type == 'Update' )
         {
            $scope.chartName = 'Record Update' ;
            $scope.charts['Module']['options'] = window.SdbSacManagerConf.RecordUpdateEchart ;
         }
      } ;

      //图表下拉选项
      $scope.DropdownMenu = [ 
         { 'html': $compile( '<div style="padding:5px 10px" ng-click="changeCharts(\'Insert\')">Record Insert</div>' )( $scope ) },
         { 'html': $compile( '<div style="padding:5px 10px" ng-click="changeCharts(\'Read\')">Record Read</div>' )( $scope ) },
         { 'html': $compile( '<div style="padding:5px 10px" ng-click="changeCharts(\'Delete\')">Record Delete</div>' )( $scope ) },
         { 'html': $compile( '<div style="padding:5px 10px" ng-click="changeCharts(\'Update\')">Record Update</div>' )( $scope ) }
      ] ;
     
      //跳转至资源
      $scope.GotoResources = function(){
         if( window.Config['Edition'] != 'Enterprise' )
         {
            _IndexPublic.createCommunityModel( $scope ) ;
            return ;
         }
         $location.path( '/Monitor/SDB-Resources/Session' ).search( { 'r': new Date().getTime() } ) ;
      } ;

      //跳转至分区组列表
      $scope.GotoGroups = function(){
         $location.path( '/Monitor/SDB-Nodes/Groups' ).search( { 'r': new Date().getTime() } ) ;
      } ;

      //跳转至主机列表
      $scope.GotoHostList = function(){
         if( window.Config['Edition'] != 'Enterprise' )
         {
            _IndexPublic.createCommunityModel( $scope ) ;
            return ;
         }
         $location.path( '/Monitor/SDB-Host/List/Index' ).search( { 'r': new Date().getTime() } ) ;
      } ;

      //跳转至会话列表
      $scope.GotoSessions = function(){
         $location.path( '/Monitor/SDB-Resources/Session' ).search( { 'r': new Date().getTime() } ) ;
      } ;

      //跳转至域列表
      $scope.GotoDomains = function(){
         $location.path( '/Monitor/SDB-Resources/Domain' ).search( { 'r': new Date().getTime() } ) ;
      } ;

      //跳转至节点列表
      $scope.GotoNodeList = function(){
         if( window.Config['Edition'] != 'Enterprise' )
         {
            _IndexPublic.createCommunityModel( $scope ) ;
            return ;
         }
         if( moduleMode == 'distribution' )
         {
            $location.path( '/Monitor/SDB-Nodes/Nodes' ).search( { 'r': new Date().getTime() } ) ;
         }
         else
         {
            $location.path( '/Monitor/SDB-Nodes/Node/Index' ).search( { 'r': new Date().getTime() } ) ;
         }
      } ;

      //跳转至数据库操作
      $scope.GotoDatabase = function(){
         $location.path( '/Data/SDB-Database/Index' ).search( { 'r': new Date().getTime() } ) ;
      } ;

   } ) ;

}());

