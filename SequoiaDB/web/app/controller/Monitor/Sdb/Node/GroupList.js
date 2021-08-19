//@ sourceURL=GroupList.js
(function(){
   var sacApp = window.SdbSacManagerModule ;
   //控制器
   sacApp.controllerProvider.register( 'Monitor.SdbOverview.Index.Ctrl', function( $scope, $compile, $rootScope, $location, SdbRest, SdbFunction ){
      
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
      //分区组列表
      $scope.GroupList = [] ;
      $scope.clList = [] ;
      $scope.NumConnects = 0 ;
      //新表格
      $scope.GroupTable = {
         'title': {
            'ErrNodes': $scope.autoLanguage( '状态' ),
            'GroupName': $scope.autoLanguage( '分区组名' ),
            'Group': $scope.autoLanguage( '节点数' ),
            'PrimaryNodeName': $scope.autoLanguage( '主节点' ),
            'TotalCL': $scope.autoLanguage( '集合数' ),
            'TotalRecords': $scope.autoLanguage( '记录数' )
         },
         'body': [],
         'options': {
            'width': {
               'ErrNodes': '100px'
            },
            'sort': {},
            'filter':{}
         },
         'callback': {}
      } ;

      //启动分区组 弹窗
      $scope.StartGroup = {
         'config': {
            'select': [],
            'value': ''
         },
         'callback': {}
      } ;

      //停止分区组 弹窗
      $scope.StopGroup = {
         'config': {
            'select': [],
            'value': ''
         },
         'callback': {}
      } ;


      //跳转至扩容
      $scope.GotoExtend = function(){
         $rootScope.tempData( 'Deploy', 'Model', 'Module' ) ;
         $rootScope.tempData( 'Deploy', 'Module', 'sequoiadb' ) ;
         $rootScope.tempData( 'Deploy', 'ModuleName', moduleName ) ;
         $rootScope.tempData( 'Deploy', 'ClusterName', clusterName ) ;
         $rootScope.tempData( 'Deploy', 'ExtendMode', 'horizontal' ) ;
         $rootScope.tempData( 'Deploy', 'DeployMod', 'distribution' ) ;
         $location.path( '/Deploy/SDB-ExtendConf' ).search( { 'r': new Date().getTime() } ) ;
      }

      //开启分区组
      var startGroup = function( groupName ){
         var data = { 'cmd': 'start group', 'name': groupName } ;
         SdbRest.DataOperation( data, {
            'success': function(){
               getGroupList() ;
            },
            'failed': function( errorInfo ){
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  startNode( groupName ) ;
                  return true ;
               } ) ;
            }
         } ) ;
      } ;

      //停止分区组
      var stopGroup = function( groupName ){
         var data = { 'cmd': 'stop group', 'name': groupName } ;
         SdbRest.DataOperation( data,{
            'success': function(){
               getGroupList() ;
            },
            'failed': function( errorInfo ){
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  stopGroup( groupName ) ;
                  return true ;
               } ) ;
            }
         } ) ;
      } ;

      //打开启动分区组的弹窗
      $scope.OpenStartGroup = function(){
         if( $scope.StartGroup['config']['select'].length > 0 )
         {
            //设置确定按钮
            $scope.StartGroup['callback']['SetOkButton']( $scope.autoLanguage( '确定' ), function(){
               var groupName = $scope.StartGroup['config']['value'] ;
               startGroup( groupName ) ;
               return true ;
            } ) ;

            //关闭窗口滚动条
            $scope.StartGroup['callback']['DisableBodyScroll']() ;
            //设置标题
            $scope.StartGroup['callback']['SetTitle']( $scope.autoLanguage( '启动分区组' ) ) ;
            $scope.StartGroup['callback']['Open']() ;
         }
      } ;

      //打开停止分区组的弹窗
      $scope.OpenStopGroup = function(){
         if( $scope.StopGroup['config']['select'].length > 0 )
         {
            //设置确定按钮
            $scope.StopGroup['callback']['SetOkButton']( $scope.autoLanguage( '确定' ), function(){
               var groupName = $scope.StopGroup['config']['value'] ;
               stopGroup( groupName ) ;
               return true ;
            } ) ;
            //关闭窗口滚动条
            $scope.StopGroup['callback']['DisableBodyScroll']() ;
            //设置标题
            $scope.StopGroup['callback']['SetTitle']( $scope.autoLanguage( '停止分区组' ) ) ;
            $scope.StopGroup['callback']['Open']() ;
         }
      }

      //获取coord节点状态
      var getCoordStatus = function( groupInfo, nodesInfo, hostname, svcname ){
         var sql = 'SELECT NodeName, TotalNumConnects FROM $SNAPSHOT_DB WHERE GLOBAL=false' ;
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
                     groupInfo['ErrNodes'].push( { 'NodeName': hostname + ':' + svcname, 'Flag': -15 } ) ;
                  }
               }
            }, 
            'failed':function( errorInfo ){
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  getCoordStatus( groupInfo, nodesInfo, hostname, svcname ) ;
                  return true ;
               } ) ;
            }
         },{
            'showLoading': false
         } ) ;
      }

      //获取coord连接数
      var getConnectionNum = function(){
         var sql = 'SELECT TotalNumConnects FROM $SNAPSHOT_DB WHERE Role = "coord"' ;
         SdbRest.Exec( sql, {
            'success': function( result ){
               if( result.length > 0 )
               {
                  $scope.NumConnects = 0 ;
                  $.each( result, function( index, info ){
                     if( typeof( info['ErrNodes'] ) == 'undefined' )
                     {
                        $scope.NumConnects += info['TotalNumConnects'] ;
                     }
                  } ) ;
               }
            }, 
            'failed':function( errorInfo ){
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  getConnectionNum() ;
                  return true ;
               } ) ;
            }
         },{
            'showLoading': false
         } ) ;
      }
      
      //获取集合信息
      var getClInfo = function(){
         $scope.StartGroup['config']['select'] = [] ;
         $scope.StopGroup['config']['select']  = [] ;
         var sql = 'SELECT * FROM $SNAPSHOT_CL' ;
         //获取CL信息
         SdbRest.Exec( sql, {
            'success': function( clList ){
               $scope.clList = clList ;
               $.each( $scope.GroupList, function( index, groupInfo ){
                  $scope.GroupList[index]['ErrNodes'] = [] ;
                  if( groupInfo['Role'] != 1 ) //非协调节点
                  {
                     groupInfo['PrimaryNodeName'] = '' ;
                     $.each( groupInfo['Group'], function( nodeIndex, nodeInfo ){
                        if( groupInfo['PrimaryNode'] == nodeInfo['NodeID'] )
                        {
                           $scope.GroupList[index]['PrimaryNodeName'] = nodeInfo['HostName'] + ':' + nodeInfo['Service']['0']['Name'] ;
                        }
                     } ) ;
                     $scope.GroupList[index]['TotalCL'] = 0 ;
                     $scope.GroupList[index]['TotalRecords'] = 0 ;
                  }
                  else //协调节点
                  {
                     //检查协调节点状态
                     $.each( groupInfo['Group'], function( nodeIndex, nodeInfo ){
                        getCoordStatus( groupInfo, nodeInfo, nodeInfo['HostName'], nodeInfo['Service'][0]['Name'] ) ;
                     } ) ;
                  }
                  $.each( $scope.clList, function( index2, clInfo ){
                     if( isArray( clInfo['ErrNodes'] ) == true )
                     {
                        $.each( clInfo['ErrNodes'], function( errIndex, errInfo ){
                           if( groupInfo['GroupName'] == errInfo['GroupName'] )
                           {
                              $scope.GroupList[index]['ErrNodes'].push( { 'NodeName': errInfo['NodeName'], 'Flag': errInfo['Flag'] } ) ;
                           }
                        } ) ;
                     }
                     else if( groupInfo['Role'] == 0 )
                     {
                        $.each( clInfo['Details'], function( index3, clDetail ){
                           if( clDetail['GroupName'] == groupInfo[ 'GroupName' ] &&
                               clDetail['NodeName'] == $scope.GroupList[index]['PrimaryNodeName'] )
                           {
                              ++$scope.GroupList[index]['TotalCL'] ;
                              $scope.GroupList[index]['TotalRecords'] += clDetail['TotalRecords'] ;
                           }
                        } ) ;
                     }
                  } ) ;
               } ) ;
               
               $.each( $scope.GroupList, function( groupIndex, groupInfo ){
                  //分区组所有节点故障时，只能启动节点
                  if( groupInfo['ErrNodes'].length == groupInfo['Group'].length && groupInfo['ErrNodes'].length > 0 )
                  {
                     if( $scope.StartGroup['config']['select'].length == 0 )
                     {
                        $scope.StartGroup['config']['value'] = groupInfo['GroupName']  ;
                     }
                     $scope.StartGroup['config']['select'].push( {
                        'key': groupInfo['GroupName'],
                        'value': groupInfo['GroupName']
                     } ) ;

                     $scope.GroupList[groupIndex]['PrimaryNodeName'] = ( groupInfo['ErrNodes'].length == groupInfo['Group'].length ) ? '-' : $scope.GroupList[groupIndex]['PrimaryNodeName'] ;
                  }
                  //分区组中有部分节点故障时，可以选择启动分区组或者停止分区组
                  else if( groupInfo['ErrNodes'].length > 0 && groupInfo['ErrNodes'].length < groupInfo['Group'].length )
                  {
                     if( groupInfo['Role'] != 1 && groupInfo['Group'].length > 0 )
                     {
                        if( $scope.StopGroup['config']['select'].length == 0 )
                        {
                           $scope.StopGroup['config']['value'] = groupInfo['GroupName']  ;
                        }
                        $scope.StopGroup['config']['select'].push( {
                           'key': groupInfo['GroupName'],
                           'value': groupInfo['GroupName']
                        } ) ;

                        if( $scope.StartGroup['config']['select'].length == 0 )
                        {
                           $scope.StartGroup['config']['value'] = groupInfo['GroupName']  ;
                        }
                        $scope.StartGroup['config']['select'].push( {
                           'key': groupInfo['GroupName'],
                           'value': groupInfo['GroupName']
                        } ) ;

                     }
                  }
                  //分区组没有节点错误时只能选择停止分区组
                  else
                  {
                     if( groupInfo['Role'] != 1 && groupInfo['Group'].length > 0 )
                     {
                        if( $scope.StopGroup['config']['select'].length == 0 )
                        {
                           $scope.StopGroup['config']['value'] = groupInfo['GroupName']  ;
                        }
                        $scope.StopGroup['config']['select'].push( {
                           'key': groupInfo['GroupName'],
                           'value': groupInfo['GroupName']
                        } ) ;
                     }
                  }
               } ) ;

               //表格信息
               $scope.GroupTable['body'] = [] ;
               $.each( $scope.GroupList, function( groupIndex, groupInfo ){
                  //将数据组信息放入表格body
                  if( groupInfo['Role'] == 0 )
                  {
                     $scope.GroupTable['body'].push( groupInfo ) ;
                  }
               } ) ;
            },
            'failed': function( errorInfo ){
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  $scope.getGroupList() ;
                  return true ;
               } ) ;
            }
         },{
            'showLoading': false
         } ) ;
      }

      //获取分区组列表
      var getGroupList = function(){
         var data = { 'cmd': 'list groups' } ;
         SdbRest.DataOperation( data, {
            'success':function( groups ){
               $scope.GroupList = groups ;
               getConnectionNum() ;
               getClInfo() ;
            },
            'failed': function( errorInfo ){
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  getGroupList() ;
                  return true ;
               } ) ;
            }
         },{
            'showLoading': false
         } ) ;
      }

      getGroupList() ;

      //跳转至监控主页
      $scope.GotoHosts = function(){
         $location.path( '/Monitor/SDB-Host/List/Index' ).search( { 'r': new Date().getTime() } ) ;
      } ;

      //跳转至分区组列表
      $scope.GotoGroups = function(){
         $location.path( '/Monitor/SDB-Nodes/Groups' ).search( { 'r': new Date().getTime() } ) ;
      } ;

      //跳转至分区组信息
      $scope.GotoGroup = function( GroupName ){
         SdbFunction.LocalData( 'SdbGroupName', GroupName ) ;
         $location.path( '/Monitor/SDB-Nodes/Group/Index' ).search( { 'r': new Date().getTime() } ) ;
      } ;

      //跳转至节点信息
      $scope.GotoNode = function( nodeName ){
         var temp = nodeName.split( ':' ) ;
         var hostName = temp[0] ;
         var serviceName = temp[1] ;
         SdbFunction.LocalData( 'SdbHostName', hostName ) ;
         SdbFunction.LocalData( 'SdbServiceName', serviceName ) ;
         $location.path( '/Monitor/SDB-Nodes/Node/Index' ).search( { 'r': new Date().getTime() } ) ;
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

