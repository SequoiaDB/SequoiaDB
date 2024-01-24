//@ sourceURL=History.js
(function(){
   var sacApp = window.SdbSacManagerModule ;
   //控制器
   sacApp.controllerProvider.register( 'System.History.Ctrl', function( $scope, SdbRest ){
      
      var sdbList = {
         'add business': $scope.autoLanguage( '添加分布式存储集群' ),
         'remove business': $scope.autoLanguage( '卸载分布式存储集群' ),
         'discover business': $scope.autoLanguage( '发现分布式存储集群' ),
         'unbind business': $scope.autoLanguage( '移除分布式存储集群' ),
         'restart business': $scope.autoLanguage( '重启分布式存储集群' ),
         'sync business configure': $scope.autoLanguage( '同步配置' ),
         'extend business': $scope.autoLanguage( '扩容' ),
         'shrink business': $scope.autoLanguage( '减容' ),
         'set business authority': $scope.autoLanguage( '设置' ),
         'remove business authority': $scope.autoLanguage( '删除' ),
         'create relationship': $scope.autoLanguage( '配置实例存储' ),
         'remove relationship': $scope.autoLanguage( '移除实例存储' )
      }

      var sqlList = {
         'add business': $scope.autoLanguage( '添加数据库实例' ),
         'remove business': $scope.autoLanguage( '卸载数据库实例' ),
         'discover business': $scope.autoLanguage( '发现数据库实例' ),
         'unbind business': $scope.autoLanguage( '移除数据库实例' ),
         'restart business': $scope.autoLanguage( '重启数据库实例' ),
      }

      function queryHistory(){
         SdbRest.OmOperation( { 'cmd': 'query history','sort': JSON.stringify( {'Time':-1} )  }, {
            'success': function( result ){
               $.each( result, function( index, info ){
                  info['Index'] = index ;
                  info['Time'] = info['Time']['$timestamp'].slice( 0, info['Time']['$timestamp'].lastIndexOf( "." ) ) ;
                  var timeIndex = info['Time'].lastIndexOf( "-" ) ;
                  var time1 = info['Time'].slice( 0, timeIndex ) ;
                  var time2 = info['Time'].slice( timeIndex+1 ) ;
                  time2 = time2.replace( /\./g, ':' ) ;
                  info['Time'] = time1 + ' ' + time2 ;

                  info['hasContent'] = false ;
                  switch( info['ExecType'] ){
                     case 'login':
                        info['Desc'] = sprintf( $scope.autoLanguage( '用户 ? 登陆' ), info['Detail']['User'] ) ;
                        break ;
                     case 'change passwd':
                        info['Desc'] = sprintf( $scope.autoLanguage( '用户 ? 修改密码' ), info['User'] ) 
                        break ;
                     case 'set settings':
                        info['Desc'] = sprintf( $scope.autoLanguage( '用户 ? 设置系统配置' ), info['User'] ) 
                        break ;
                     case 'create cluster' :
                        info['Desc'] = sprintf( $scope.autoLanguage( '用户 ? ? ?' ), info['User'], $scope.autoLanguage( '创建集群' ), info['Detail']['ClusterName'] ) ;
                        break ;
                     case 'remove cluster' :
                        info['Desc'] = sprintf( $scope.autoLanguage( '用户 ? ? ?' ), info['User'], $scope.autoLanguage( '删除集群' ), info['Detail']['ClusterName'] ) ;
                        break ;
                     case 'add business':
                     case 'remove business':
                     case 'discover business':
                     case 'unbind business':
                     case 'restart business':
                        info['hasContent'] = true ;
                        if( info['Detail']['BusinessType'] == 'sequoiadb' )
                        {
                           var type = sdbList[info['ExecType']] ;
                        }
                        else
                        {
                           var type = sqlList[info['ExecType']] ;
                        }
                        info['Desc'] = sprintf( $scope.autoLanguage( '用户 ? ? ?' ), info['User'], type, info['Detail']['BusinessName'] ) ;
                        break ;
                     case 'set business authority':
                     case 'remove business authority':
                        var type = sdbList[info['ExecType']] ;
                        info['Desc'] = sprintf( $scope.autoLanguage( '用户 ? ? 了 ? 的鉴权' ), info['User'], type, info['Detail']['BusinessName'] ) ;
                        break ;
                     case 'create relationship':
                        info['Desc'] = sprintf( $scope.autoLanguage( '用户 ? 添加实例存储 ?' ), info['User'], info['Detail']['Name'] ) ;
                        break ;
                     case 'remove relationship':
                        info['Desc'] = sprintf( $scope.autoLanguage( '用户 ? 删除实例存储 ?' ), info['User'], info['Detail']['Name'] ) ;
                        break ;
                     case 'grant sysconf':
                        info['hasContent'] = true ;
                        info['Name'] = info['Detail']['Name'] ;
                        info['Desc'] = sprintf( $scope.autoLanguage( '用户 ? 修改集群 ? 资源授权' ), info['User'], info['Detail']['ClusterName'] ) ;
                        break ;
                     case 'update host info':
                        info['hasContent'] = true ;
                        info['Desc'] = sprintf( $scope.autoLanguage( '用户 ? 更新了 ? 台主机' ), info['User'], info['Detail']['Hosts'].length ) ;
                        break ;
                     case 'scan host':
                        info['hasContent'] = true ;
                        info['Desc'] = sprintf( $scope.autoLanguage( '用户 ? 扫描了 ? 台主机' ), info['User'], info['Detail']['Hosts'].length ) ;
                        break ;
                     case 'check host':
                        info['hasContent'] = true ;
                        info['Desc'] = sprintf( $scope.autoLanguage( '用户 ? 检查了 ? 台主机' ), info['User'], info['Detail']['Hosts'].length ) ;
                        break ;
                     case 'add host':
                        info['hasContent'] = true ;
                        info['Desc'] = sprintf( $scope.autoLanguage( '用户 ? 添加了 ? 台主机' ), info['User'], info['Detail']['Hosts'].length ) ;
                        break ;
                     case 'unbind host':
                        info['hasContent'] = true ;
                        info['Desc'] = sprintf( $scope.autoLanguage( '用户 ? 移除了 ? 台主机' ), info['User'], info['Detail']['Hosts'].length ) ;
                        break ;
                     case 'remove host':
                        info['hasContent'] = true ;
                        info['Desc'] = sprintf( $scope.autoLanguage( '用户 ? 卸载了 ? 台主机' ), info['User'], info['Detail']['Hosts'].length ) ;
                        break ;
                     case 'deploy package':
                        info['hasContent'] = true ;
                        info['Desc'] = sprintf( $scope.autoLanguage( '用户 ? 在集群 ? 部署 ?' ), info['User'], info['Detail']['ClusterName'], info['Detail']['PackageName'] ) ;
                        break ;
                     case 'update business config':
                        if( info['Detail']['BusinessType'] == 'sequoiadb' )
                        {
                           info['Desc'] = sprintf( $scope.autoLanguage( '用户 ? 修改分布式存储集群 ? 的配置' ), info['User'], info['Detail']['BusinessName'] ) ;
                        }
                        else
                        {
                           info['Desc'] = sprintf( $scope.autoLanguage( '用户 ? 修改数据库实例 ? 的配置' ), info['User'], info['Detail']['BusinessName'] ) ;
                        }
                        break ;
                     case 'delete business config':
                        if( info['Detail']['BusinessType'] == 'sequoiadb' )
                        {
                           info['Desc'] = sprintf( $scope.autoLanguage( '用户 ? 删除分布式存储集群 ? 的配置' ), info['User'], info['Detail']['BusinessName'] ) ;
                        }
                        else
                        {
                           info['Desc'] = sprintf( $scope.autoLanguage( '用户 ? 删除数据库实例 ? 的配置' ), info['User'], info['Detail']['BusinessName'] ) ;
                        }
                        break ;
                     case 'sync business configure':
                        if( info['Detail']['BusinessType'] == 'sequoiadb' )
                        {
                           info['Desc'] = sprintf( $scope.autoLanguage( '用户 ? 同步分布式存储集群 ? 的配置' ), info['User'], info['Detail']['BusinessName'] ) ;
                        }
                        else
                        {
                           info['Desc'] = sprintf( $scope.autoLanguage( '用户 ? 同步数据库实例 ? 的配置' ), info['User'], info['Detail']['BusinessName'] ) ;
                        }
                        break ;
                     case 'extend business':
                        info['hasContent'] = true ;
                        info['Desc'] = sprintf( $scope.autoLanguage( '用户 ? 在分布式存储集群 ? 扩容了 ? 节点' ), info['User'], info['Detail']['BusinessName'] , info['Detail']['Nodes'].length ) ;
                        break ;
                     case 'shrink business':
                        info['hasContent'] = true ;
                        info['Desc'] = sprintf( $scope.autoLanguage( '用户 ? 在分布式存储集群 ? 减容了 ? 节点' ), info['User'], info['Detail']['BusinessName'] , info['Detail']['Nodes'].length ) ;
                        break ;
                  }
                  if( !isUndefined( info['Detail']['ClusterName'] ) )
                  {
                     info['ClusterName'] = info['Detail']['ClusterName'] ;
                  }
                  if( !isUndefined( info['Detail']['BusinessName'] ) )
                  {
                     info['BusinessName'] = info['Detail']['BusinessName'] ;
                  }
                  if( !isUndefined( info['Detail']['BusinessType'] ) )
                  {
                     info['BusinessType'] = info['Detail']['BusinessType'] ;
                  }
                  if( !isEmpty( info['Detail']['DeployMod'] ) )
                  {
                     info['DeployMod'] = info['Detail']['DeployMod'] ;
                  }
                  
                  if( !isUndefined( info['Detail']['Hosts'] ) )
                  {
                     var isFirst = true ;
                     info['Hosts'] = '' ;
                     $.each( info['Detail']['Hosts'], function( index, info2 ){
                        var host = isUndefined( info2['IP'] ) ? info2['HostName'] : info2['IP'] ;
                        if( isFirst )
                        {
                           isFirst = false ;
                           info['Hosts'] = host ;
                        }
                        else
                        {
                           info['Hosts'] = info['Hosts'] + ',' + host  ;
                        }
                     } ) ;
                  }
                  if( !isUndefined( info['Detail']['Nodes'] ) )
                  {
                     var isFirst = true ;
                     info['Nodes'] = '' ;
                     $.each( info['Detail']['Nodes'], function( index, info2 ){
                        var port = isUndefined( info2['svcname'] ) ? info2['port'] : info2['svcname'] ;
                        if( isFirst )
                        {
                           isFirst = false ;
                           info['Nodes'] = info2['HostName'] + ':' + port ;
                        }
                        else
                        {
                           info['Nodes'] = info['Nodes'] + ',' + info2['HostName'] + ':' + port ;
                        }
                     } ) ;
                  }
                  if( !isUndefined( info['Detail']['PackageName'] ) )
                  {
                     info['PackageName'] = info['Detail']['PackageName'] ;
                  }
                  if( !isUndefined( info['Detail']['Enforced'] ) )
                  {
                     info['Enforced'] = info['Detail']['Enforced'] == 1 ? true : false ;
                  }
               } ) ;
              
               $scope.HistoryTable['body'] = result ;
            },
            'failed': function( errorInfo ){
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  queryHistory() ;
                  return true ;
               } ) ;
            }
         }, {
            'showLoading': true
         } ) ;
      }
      queryHistory() ;

      $scope.HistoryTable = {
         'title': {
            'Time': $scope.autoLanguage( '时间' ),
            'Success': $scope.autoLanguage( '执行结果' ),
            'Desc': $scope.autoLanguage( '描述' )
         },
         'body': [],
         'options': {
            'width': {
               'Time': '240px',
               'Success': '100px'
            },
            'max': 30,
            'filter': {
               'Time':  'indexof',
               
               'Success': [
                  { 'key': $scope.autoLanguage( '全部' ), 'value': '' },
                  { 'key': $scope.autoLanguage( '成功' ), 'value': true },
                  { 'key': $scope.autoLanguage( '失败' ), 'value': false }
               ],
               'Desc':  'indexof'
            }
         }
      } ;


      $scope.HisInfo = {
         'config': {},
         'callback': {}
      } ;


      //显示详细
      $scope.OpenInfo = function( index ){
         var info = $scope.HistoryTable['body'][index] ;
         $scope.HisInfo['config'] = info ;
         //设置标题
         $scope.HisInfo['callback']['SetTitle']( $scope.autoLanguage( '详细信息' ) ) ;
         //设置图标
         $scope.HisInfo['callback']['SetIcon']( '' ) ;
         //打开窗口
         $scope.HisInfo['callback']['Open']() ;
      }

   } ) ;
}());