//@ sourceURL=Index.js
(function(){
   var sacApp = window.SdbSacManagerModule ;
   //控制器
   sacApp.controllerProvider.register( 'Monitor.SdbGroup.Index.Ctrl', function( $scope, $compile, $location, SdbRest, SdbFunction ){
      
      var clusterName = SdbFunction.LocalData( 'SdbClusterName' ) ;
      var moduleType = SdbFunction.LocalData( 'SdbModuleType' ) ;
      var moduleName = SdbFunction.LocalData( 'SdbModuleName' ) ;
      var moduleMode = SdbFunction.LocalData( 'SdbModuleMode' ) ;
      var groupName = SdbFunction.LocalData( 'SdbGroupName' ) ;
      if( clusterName == null || moduleType != 'sequoiadb' || moduleMode == null || moduleName == null )
      {
         $location.path( '/Transfer' ).search( { 'r': new Date().getTime() } ) ;
         return;
      }

      if( groupName == null )
      {
         $location.path( '/Monitor/SDB-Nodes/Groups' ).search( { 'r': new Date().getTime() } ) ;
         return;
      }

      //初始化
      //分区组名
      $scope.GroupName = groupName ;
      //分区组状态
      $scope.GroupStatus = 'Normal' ;
      //分区组所属域
      $scope.DomainList = '-' ;
      $scope.PrimaryNode = {} ;
      $scope.ErrNum = 0 ;
      $scope.ErrMsg = '' ;
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
      

      //状态信息
      var setStatusMsg = function(){
         if( $scope.ErrNum > 0 )
         {
            if( $scope.ErrNum == $scope.groupInfo['Group'].length )
            {
               $scope.GroupStatus = 'Error' ;
               $scope.ErrMsg = sprintf( $scope.autoLanguage( '分区组 ? 所有节点异常。' ), $scope.groupInfo['GroupName'] ) ;
            }
            else
            {
               $scope.GroupStatus = 'Warning' ;
               $scope.ErrMsg = sprintf( $scope.autoLanguage( '分区组 ? 有节点异常。' ), $scope.groupInfo['GroupName'] ) ;
            }
         }
         else
         {
            $scope.GroupStatus = 'Normal' ;
            $scope.ErrMsg = '' ;
         }
      }

      
      //获取CL列表
      var getClList = function(){
         var sql = 'select '
         var sql = 'select * from $SNAPSHOT_CL where Groups="' + groupName + '"' ;
         SdbRest.Exec( sql, { 
            'success': function( ClList ){
               $.each( ClList, function( clIndex, clInfo ){
                  if( typeof( clInfo['ErrNodes'] ) != 'undefined' )
                  {
                     $.each( clInfo['ErrNodes'], function( errIndex, errInfo ){
                        ++$scope.ErrNum ;
                        $.each( $scope.groupInfo['Group'], function( nodeIndex, nodeInfo ){
                           if( errInfo['NodeName'] == nodeInfo['NodeName'] )
                           {
                              $scope.groupInfo['Group'][nodeIndex]['ErrInfo'] = errInfo['Flag'] ;
                              $scope.groupInfo['Group'][nodeIndex]['TotalRecords'] = '-' ;
                           }
                        } ) ;
                     } ) ;
                  }
                  else
                  {
                     $.each( clInfo['Details'], function( detailIndex, detailInfo ){
                        $.each( $scope.groupInfo['Group'], function( nodeIndex, nodeInfo ){
                           if( nodeInfo['NodeName'] == detailInfo['NodeName'] )
                           {
                              $scope.groupInfo['Group'][nodeIndex]['TotalRecords'] += detailInfo['TotalRecords'] ;
                           }
                        } ) ;
                     } ) ;
                  }
               } ) ;
               $.each( $scope.groupInfo['Group'], function( index, nodeInfo ){
                  //记录停止的节点
                  if( $scope.groupInfo['Group'][index]['ErrInfo'] != false && $scope.StartNode['config']['select'].length == 0 )
                  {
                     $scope.StartNode['config']['value'] = $scope.groupInfo['Group'][index]['NodeName'] ;
                  }
                  if( $scope.groupInfo['Group'][index]['ErrInfo'] != false )
                  {
                     $scope.StartNode['config']['select'].push( {
                        'key': $scope.groupInfo['Group'][index]['NodeName'],
                        'value': $scope.groupInfo['Group'][index]['NodeName']
                     } ) ;
                  }
                     
                  //记录启动的节点
                  if( $scope.groupInfo['Group'][index]['ErrInfo'] == false && $scope.StopNode['config']['select'].length == 0 )
                     $scope.StopNode['config']['value'] = $scope.groupInfo['Group'][index]['NodeName'] ;
                  if( $scope.groupInfo['Group'][index]['ErrInfo'] == false )
                     $scope.StopNode['config']['select'].push( {
                        'key': $scope.groupInfo['Group'][index]['NodeName'],
                        'value': $scope.groupInfo['Group'][index]['NodeName']
                     } ) ;
               } ) ;
            },
            'failed': function( errorInfo ){
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  getClList() ;
                  return true ;
               } ) ;
            },
            'complete': function(){
               setStatusMsg() ;
            }
         } ) ;
      } ;

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
                     nodesInfo['ErrInfo'] = -15 ;
                     nodesInfo['TotalRecords'] = '-' ;
                     $scope.StartNode['config']['select'].push( {
                        'key': hostname + ':' + svcname,
                        'value': hostname + ':' + svcname
                     } ) ;
                     if( $scope.StartNode['config']['select'].length == 1 )
                     {
                        $scope.StartNode['config']['value'] = hostname + ':' + svcname ;
                     }
                     ++$scope.ErrNum ;
                  }
                  else
                  {
                     $scope.StopNode['config']['select'].push( {
                        'key': hostname + ':' + svcname,
                        'value': hostname + ':' + svcname
                     } ) ;
                     if( $scope.StopNode['config']['select'].length == 1 )
                     {
                        $scope.StopNode['config']['value'] = hostname + ':' + svcname ;
                     }
                  }
               }
               setStatusMsg() ;
            }, 
            'failed':function( errorInfo ){
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  getCoordStatus( nodesInfo, hostname, svcname ) ;
                  return true ;
               } ) ;
            }
         } ) ;
      }

      //获取分区组列表
      var getGroupList = function(){
         $scope.ErrNum = 0 ;
         $scope.StartNode['config']['select'] = [] ;
         $scope.StopNode['config']['select']  = [] ;
         var data = { 'cmd': 'list groups' } ;
         SdbRest.DataOperation( data, {
            'success': function( groups ){
               if( groups.length > 0 )
               {
                  $.each( groups, function( index, groupInfo ){
                     if( groupInfo['GroupName'] == groupName )
                     {
                        groupInfo['ErrNodes'] = [] ;
                        $.each( groupInfo['Group'], function( nodeIndex, nodeInfo ){
                           groupInfo['PrimaryNodeName'] = '-' ;
                           groupInfo['Group'][nodeIndex]['NodeName'] = nodeInfo['HostName'] + ':' + nodeInfo['Service'][0]['Name'] ;
                           groupInfo['Group'][nodeIndex]['TotalRecords'] = 0 ;
                           groupInfo['Group'][nodeIndex]['ErrInfo'] = false ;
                        } ) ;
                        $scope.groupInfo = groupInfo ;
                     }
                  } ) ;
                  if( groupName === 'SYSCoord' )
                  {
                     $.each( $scope.groupInfo['Group'], function( index, nodeInfo ){
                        getCoordStatus( nodeInfo, nodeInfo['HostName'], nodeInfo['Service'][0]['Name'] ) ;
                     } ) ;
                  }
                  else
                  {
                     $.each( $scope.groupInfo['Group'], function( index, nodeInfo ){
                        //获取主节点节点名
                        if( $scope.groupInfo['PrimaryNode'] == nodeInfo['NodeID'] && $scope.groupInfo['Role'] != 1 )
                        {
                           $scope.groupInfo['PrimaryNodeName'] = nodeInfo['HostName'] + ':' + nodeInfo['Service'][0]['Name'] ;
                           $scope.PrimaryNode = { 'hostName': nodeInfo['HostName'], 'serviceName': nodeInfo['Service'][0]['Name'] } ;
                        }
                     } ) ;
                     getClList() ;
                  }
               }
            },
            'failed': function( errorInfo ){
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  getGroupList() ;
                  return true ;
               } ) ;
            }
         } ) ;
      }

      //获取domain列表
      var getDomainList = function(){
         var data = { 'cmd': 'list domains' } ;
         SdbRest.DataOperation( data, {
            'success': function( domainList ){
               var groupInDomainList = [] ;
               $.each( domainList, function( index, domainInfo ){
                  $.each( domainInfo['Groups'], function( index2, groupInfo ){
                     if( groupInfo['GroupName'] == groupName )
                     {
                        groupInDomainList.push( domainInfo['Name'] ) ;
                        return false ;
                     }
                  } ) ;
                  $scope.DomainList = groupInDomainList.join() ;
               } ) ;
               $scope
            },
            'failed': function( errorInfo ){
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  getDomainList() ;
                  return true ;
               } ) ;
            }
         } ) ;
      }

      if( groupName != 'SYSCatalogGroup' && groupName != 'SYSCoord' )
         getDomainList() ;

      getGroupList() ;

      //启动节点
      var startNode = function( hostname, svcname ){
         var data = { 'cmd': 'start node', 'HostName': hostname, 'svcname': svcname } ;
         SdbRest.DataOperation( data, {
            'success': function(){
               getGroupList() ;
            },
            'falied': function( errorInfo ){
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
               getGroupList() ;
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
      

      //跳转至节点信息
      $scope.GotoNode = function( hostName, serviceName ){
         SdbFunction.LocalData( 'SdbHostName', hostName ) ;
         SdbFunction.LocalData( 'SdbServiceName', serviceName ) ;
         $location.path( '/Monitor/SDB-Nodes/Node/Index' ).search( { 'r': new Date().getTime() } ) ;
      } ;

      //跳转至主机信息
      $scope.GotoHost = function( hostName ){
         SdbFunction.LocalData( 'SdbHostName', hostName ) ;
         $location.path( '/Monitor/SDB-Host/Info/Index' ).search( { 'r': new Date().getTime() } ) ;
      } ;
      //跳转至域
      $scope.GotoDomains = function(){
         $location.path( '/Monitor/SDB-Resources/Domain' ).search( { 'r': new Date().getTime() } ) ;
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
         $location.path( '/Monitor/SDB-Nodes/Nodes' ).search( { 'r': new Date().getTime() } ) ;
      } ;
   } ) ;
}());