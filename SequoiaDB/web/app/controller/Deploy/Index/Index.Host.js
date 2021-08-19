//@ sourceURL=Deploy.Index.Host.Ctrl.js
//"use strict" ;
(function(){
   var sacApp = window.SdbSacManagerModule ;
   //主机控制器
   sacApp.controllerProvider.register( 'Deploy.Index.Host.Ctrl', function( $scope, $rootScope, $location, SdbRest, SdbSignal, SdbSwap, Loading ){
      //主机表格
      $scope.HostListTable = {
         'title': {
            'Check':            '',
            'Error.Flag':       $scope.autoLanguage( '状态' ),
            'HostName':         $scope.autoLanguage( '主机名' ),
            'IP':               $scope.autoLanguage( 'IP地址' ),
            'BusinessName':     $scope.autoLanguage( '服务' ),
            'Packages':         $scope.autoLanguage( '包' )
         },
         'body': [],
         'options': {
            'width': {
               'Check':          '30px',
               'Error.Flag':     '60px',
               'HostName':       '25%',
               'IP':             '25%',
               'BusinessName':   '25%',
               'Packages':       '25%'
            },
            'sort': {
               'Check':                 false,
               'Error.Flag':            true,
               'HostName':              true,
               'IP':                    true,
               'BusinessName':          true,
               'Packages':              true
            },
            'max': 50,
            'filter': {
               'Check':             null,
               'Error.Flag':        'indexof',
               'HostName':          'indexof',
               'IP':                'indexof',
               'BusinessName':      'indexof',
               'Packages':          'indexof'
            }
         },
         'callback': {}
      } ;

      //查询主机
      SdbSignal.on( 'queryHost', function(){
         var data = { 'cmd': 'query host' } ;
         SdbRest.OmOperation( data, {
            'success': function( hostList ){

               SdbSwap.hostList = hostList ;
               $.each( SdbSwap.hostList, function( index ){
                  SdbSwap.hostList[index]['Error'] = {} ;
                  SdbSwap.hostList[index]['Error']['Flag'] = 0 ;
               } ) ;
               $scope.HostNum = 0 ;
               var hostTableContent = [] ;
               $.each( SdbSwap.hostList, function( index2, hostInfo ){
                  if( hostInfo['ClusterName'] == $scope.ClusterList[ $scope.CurrentCluster ]['ClusterName'] )
                  {
                     hostTableContent.push( hostInfo )
                     ++$scope.HostNum ;
                  }
               } ) ;

               SdbSignal.commit( 'updateHostNum', $scope.HostNum ) ;
               $scope.HostListTable['body'] = hostTableContent ;
            },
            'failed': function( errorInfo ){
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  SdbSignal.commit( 'queryHost' ) ;
                  return true ;
               } ) ;
            }
         } ) ;
      } ) ;

      //更新主机表格
      SdbSignal.on( 'updateHostTable', function( result ){
         $scope.HostListTable['body'] = result ;
      } ) ;

      //全选
      $scope.SelectAll = function(){
         $.each( SdbSwap.hostList, function( index ){
            if( $scope.ClusterList[ $scope.CurrentCluster ]['ClusterName'] == SdbSwap.hostList[index]['ClusterName'] )
            {
               SdbSwap.hostList[index]['checked'] = true ;
            }
         } ) ;
      }

      //反选
      $scope.Unselect = function(){
         $.each( SdbSwap.hostList, function( index ){
            if( $scope.ClusterList[ $scope.CurrentCluster ]['ClusterName'] == SdbSwap.hostList[index]['ClusterName'] )
            {
               SdbSwap.hostList[index]['checked'] = !SdbSwap.hostList[index]['checked'] ;
            }
         } ) ;
      }

      //添加主机
      $scope.AddHost = function(){
         if( $scope.ClusterList.length > 0 )
         {
            $rootScope.tempData( 'Deploy', 'Model', 'Host' ) ;
            $rootScope.tempData( 'Deploy', 'Module', 'None' ) ;
            $rootScope.tempData( 'Deploy', 'ClusterName', $scope.ClusterList[ $scope.CurrentCluster ]['ClusterName'] ) ;
            $rootScope.tempData( 'Deploy', 'InstallPath', $scope.ClusterList[ $scope.CurrentCluster ]['InstallPath'] ) ;
            $location.path( '/Deploy/ScanHost' ).search( { 'r': new Date().getTime() } ) ;
         }
      }

      SdbSignal.on( 'addHost', function(){
         $scope.AddHost() ;
      } ) ;

      //删除主机
      $scope.RemoveHost = function(){
         if( $scope.ClusterList.length > 0 )
         {
            var hostList = [] ;
            $.each( SdbSwap.hostList, function( index ){
               if( SdbSwap.hostList[index]['checked'] == true && $scope.ClusterList[ $scope.CurrentCluster ]['ClusterName'] == SdbSwap.hostList[index]['ClusterName'] )
               {
                  hostList.push( { 'HostName': SdbSwap.hostList[index]['HostName'] } ) ;
               }
            } ) ;
            if( hostList.length > 0 )
            {
               var data = {
                  'cmd': 'remove host',
                  'HostInfo': JSON.stringify( { 'ClusterName': $scope.ClusterList[ $scope.CurrentCluster ]['ClusterName'], 'HostInfo': hostList } )
               } ;
               SdbRest.OmOperation( data, {
                  'success': function( taskInfo ){
                     $rootScope.tempData( 'Deploy', 'Model', 'Task' ) ;
                     $rootScope.tempData( 'Deploy', 'Module', 'None' ) ;
                     $rootScope.tempData( 'Deploy', 'HostTaskID', taskInfo[0]['TaskID'] ) ;
                     $location.path( '/Deploy/Task/Host' ).search( { 'r': new Date().getTime() } ) ;
                  },
                  'failed': function( errorInfo ){

                     var module = null ;
                     if( errorInfo['detail'].indexOf( 'failed to remove host, the host has business' ) >= 0 )
                     {
                        var hostName = errorInfo['detail'].replace( "failed to remove host, the host has business: host=", "" ) ;
                        $.each( $scope.ModuleList, function( index, moduleInfo ){
                           $.each( moduleInfo['Location'], function( index, hostInfo ){
                              if( hostInfo['HostName'] == hostName )
                              {
                                 module = moduleInfo ;
                                 return false ;
                              }
                           } ) ;
                           if( !isNull( module ) )
                           {
                              return false ;
                           }
                        } ) ;
                     }

                     if( !isNull( module ) )
                     {
                        var context = null ;

                        if( module['BusinessType'] == "sequoiadb" )
                        {
                           context = sprintf( $scope.autoLanguage( '卸载失败，该主机还有 ? 的节点在运行，请同步 ? 配置或删除 ?，再卸载主机。' ), module['BusinessName'], module['BusinessName'], module['BusinessName'] ) ;
                        }
                        else
                        {
                           context = sprintf( $scope.autoLanguage( '卸载失败，该主机还有 ? 实例在运行，请删除实例后再卸载主机。' ), module['BusinessName'], module['BusinessName'] ) ;
                        }

                        _IndexPublic.ErrorTipsModel( $scope, context ) ;
                     }
                     else
                     {
                        _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                           $scope.RemoveHost() ;
                           return true ;
                        } ) ;
                     }
                  }
               } ) ;
            }
            else
            {
               $scope.Components.Confirm.type = 3 ;
               $scope.Components.Confirm.context = $scope.autoLanguage( '至少选择一台主机。' ) ;
               $scope.Components.Confirm.isShow = true ;
               $scope.Components.Confirm.noClose = true ;
            }
         }
      }

      //解绑主机
      $scope.UnbindHost = function(){
         if( $scope.ClusterList.length > 0 )
         {
            var hostList = { "HostInfo": [] } ;
            var hostListTips = '' ;
            $.each( SdbSwap.hostList, function( index ){
               if( SdbSwap.hostList[index]['checked'] == true && $scope.ClusterList[ $scope.CurrentCluster ]['ClusterName'] == SdbSwap.hostList[index]['ClusterName'] )
               {
                  hostList['HostInfo'].push( { 'HostName': SdbSwap.hostList[index]['HostName'] } ) ;
               }
            } ) ;
            if( hostList['HostInfo'].length > 0 )
            {
               var clusterName = $scope.ClusterList[ $scope.CurrentCluster ]['ClusterName'] ;
               var data = { 'cmd': 'unbind host', 'ClusterName': clusterName, 'HostInfo': JSON.stringify( hostList ) } ;
               SdbRest.OmOperation( data, {
                  'success': function( taskInfo ){
                     $scope.Components.Confirm.type = 4 ;
                     $scope.Components.Confirm.context = sprintf( $scope.autoLanguage( '主机解绑成功。' ) ) ;
                     $scope.Components.Confirm.isShow = true ;
                     $scope.Components.Confirm.noClose = true ;
                     $scope.Components.Confirm.normalOK = true ;
                     $scope.Components.Confirm.okText = $scope.autoLanguage( '确定' ) ;
                     $scope.Components.Confirm.ok = function(){
                        $scope.Components.Confirm.normalOK = false ;
                        $scope.Components.Confirm.isShow = false ;
                        $scope.Components.Confirm.noClose = false ;
                        SdbSignal.commit( 'queryHost' ) ;
                     }
                  },
                  'failed': function( errorInfo ){
                     var module = null ;
                     if( errorInfo['detail'].indexOf( 'failed to unbind host, the host has business: host=' ) >= 0 )
                     {
                        var hostName = errorInfo['detail'].replace( "failed to unbind host, the host has business: host=", "" ) ;
                        $.each( $scope.ModuleList, function( index, moduleInfo ){
                           $.each( moduleInfo['Location'], function( index, hostInfo ){
                              if( hostInfo['HostName'] == hostName )
                              {
                                 module = moduleInfo ;
                                 return false ;
                              }
                           } ) ;
                           if( !isNull( module ) )
                           {
                              return false ;
                           }
                        } ) ;
                     }

                     if( !isNull( module ) )
                     {
                        var context = null ;

                        if( module['BusinessType'] == "sequoiadb" )
                        {
                           context = sprintf( $scope.autoLanguage( '解绑失败，该主机还有 ? 的节点在运行，请同步 ? 配置或删除 ?，再解绑主机。' ), module['BusinessName'], module['BusinessName'], module['BusinessName'] ) ;
                        }
                        else
                        {
                           context = sprintf( $scope.autoLanguage( '解绑失败，该主机还有 ? 实例在运行，请删除实例后再解绑主机。' ), module['BusinessName'], module['BusinessName'] ) ;
                        }

                        _IndexPublic.ErrorTipsModel( $scope, context ) ;
                     }
                     else
                     {
                        _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                           $scope.UnbindHost() ;
                           return true ;
                        } ) ;
                     }
                  }
               } ) ;
            }
            else
            {
               $scope.Components.Confirm.type = 3 ;
               $scope.Components.Confirm.context = $scope.autoLanguage( '至少选择一台主机。' ) ;
               $scope.Components.Confirm.isShow = true ;
               $scope.Components.Confirm.noClose = true ;
            }
         }
      }
      
      //逐个更新主机信息
      var updateHostsInfo = function( hostList, index, success ) {
         if( index == hostList.length )
         {
            setTimeout( success ) ;
            return ;
         }

         if( hostList[index]['Flag'] != 0 )
         {
            updateHostsInfo( hostList, index + 1, success ) ;
            return ;
         }

         var hostInfo = {
            'HostInfo' : [
               {
                  'HostName': hostList[index]['HostName'],
                  'IP': hostList[index]['IP']
               }   
            ]
         }
         var data = { 'cmd': 'update host info', 'HostInfo': JSON.stringify( hostInfo ) } ;
         SdbRest.OmOperation( data, {
            'success': function( scanInfo ){
               hostList[index]['Status'] = $scope.autoLanguage( '更新主机信息成功。' ) ;
               SdbSwap.hostList[ hostList[index]['SourceIndex'] ]['IP'] = hostList[index]['IP'] ;
               updateHostsInfo( hostList, index + 1, success ) ;
            },
            'failed': function( errorInfo ){
               Loading.close() ;
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  Loading.create() ;
                  updateHostsInfo( hostList, index, success ) ;
                  return true ;
               } ) ;
            }
         }, {
            'showLoading': false
         } ) ;
      }

      //逐个扫描主机
      var scanHosts = function( hostList, index, success ){
      
         if( index == hostList.length )
         {
            success() ;
            return ;
         }

         if( hostList[index]['IP'] == SdbSwap.hostList[ hostList[index]['SourceIndex'] ]['IP'] )
         {
            hostList[index]['Status'] = $scope.autoLanguage( 'IP地址没有改变，跳过。' ) ;
            scanHosts( hostList, index + 1, success ) ;
            return ;
         }

         hostList[index]['Status'] = $scope.autoLanguage( '正在检测...' ) ;

         var scanHostInfo = [ {
            'IP': hostList[index]['IP'],
            'SshPort': hostList[index]['SshPort'],
            'AgentService': hostList[index]['AgentService']
         } ] ;
         var clusterName = $scope.ClusterList[$scope.CurrentCluster]['ClusterName'] ;
         var clusterUser = $scope.ClusterList[$scope.CurrentCluster]['SdbUser'] ;
         var clusterPwd  = $scope.ClusterList[$scope.CurrentCluster]['SdbPasswd'] ;
         var hostInfo = {
            'ClusterName': clusterName,
            'HostInfo': scanHostInfo,
            'User': clusterUser,
            'Passwd': clusterPwd,
            'SshPort': '-',
            'AgentService': '-'
         } ;
         var data = { 'cmd': 'scan host', 'HostInfo': JSON.stringify( hostInfo ) } ;
         SdbRest.OmOperation( data, {
            'success':function( scanInfo ){
               if( scanInfo[0]['errno'] == -38 || scanInfo[0]['errno'] == 0 )
               {
                  if( scanInfo[0]['HostName'] == hostList[index]['HostName'] )
                  {
                     hostList[index]['Flag'] = 0 ;
                     hostList[index]['Status'] = $scope.autoLanguage( '匹配成功。' ) ;
                  }
                  else
                  {
                     hostList[index]['Status'] = $scope.sprintf( $scope.autoLanguage( '主机名匹配错误，IP地址?的主机名是?。' ), scanInfo[0]['IP'], scanInfo[0]['HostName'] ) ;
                  }
               }
               else
               {
                  hostList[index]['Status'] = $scope.autoLanguage( '错误' ) + ': ' + scanInfo[0]['detail'] ;
               }
               scanHosts( hostList, index + 1, success ) ;
            },
            'failed': function( errorInfo ){
               Loading.close() ;
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  Loading.create() ;
                  scanHosts( hostList, index, success ) ;
                  return true ;
               } ) ;
            }
         }, {
            'showLoading': false
         } ) ;
      }

      //更新主机IP 弹窗
      $scope.UpdateHostIP = {
         'config': {},
         'callback': {}
      } ;

      //打开 更新主机IP 弹窗
      var showUpdateHostIP = function(){
         if( $scope.ClusterList.length > 0 )
         {
            $scope.UpdateHostList = [] ;
            $.each( SdbSwap.hostList, function( index ){
               if( SdbSwap.hostList[index]['checked'] == true && $scope.ClusterList[ $scope.CurrentCluster ]['ClusterName'] == SdbSwap.hostList[index]['ClusterName'] )
               {
                  $scope.UpdateHostList.push( {
                     'HostName': SdbSwap.hostList[index]['HostName'],
                     'IP': SdbSwap.hostList[index]['IP'],
                     'SshPort': SdbSwap.hostList[index]['SshPort'],
                     'AgentService': SdbSwap.hostList[index]['AgentService'],
                     'Flag': -1,
                     'Status': ( SdbSwap.hostList[index]['Error']['Flag'] == 0 ? '' : $scope.autoLanguage( '错误' ) + ': ' + SdbSwap.hostList[index]['Error']['Message'] ),
                     'SourceIndex': index
                  } ) ;
               }
            } ) ;
            if( $scope.UpdateHostList.length > 0 )
            {
               //设置确定按钮
               $scope.UpdateHostIP['callback']['SetOkButton']( $scope.autoLanguage( '确定' ), function(){
                  Loading.create() ;
                  scanHosts( $scope.UpdateHostList, 0, function(){
                     updateHostsInfo( $scope.UpdateHostList, 0, function(){
                        $scope.$apply() ;
                        Loading.cancel() ;
                     } ) ;
                  } ) ;
                  return false ;
               } ) ;
               //设置标题
               $scope.UpdateHostIP['callback']['SetTitle']( $scope.autoLanguage( '更新主机信息' ) ) ;
               //设置图标
               $scope.UpdateHostIP['callback']['SetIcon']( '' ) ;
               //打开窗口
               $scope.UpdateHostIP['callback']['Open']() ;
            }
            else
            {
               $scope.Components.Confirm.type = 3 ;
               $scope.Components.Confirm.context = $scope.autoLanguage( '至少选择一台主机。' ) ;
               $scope.Components.Confirm.isShow = true ;
               $scope.Components.Confirm.noClose = true ;
            }
         }
      }

      //前往部署安装包
      var gotoDeployPackage = function(){
         var hostList = [] ;
         $.each( SdbSwap.hostList, function( index ){
            if( SdbSwap.hostList[index]['checked'] == true && $scope.ClusterList[ $scope.CurrentCluster ]['ClusterName'] == SdbSwap.hostList[index]['ClusterName'] )
            {
               hostList.push(
                  {
                     'HostName': SdbSwap.hostList[index]['HostName'],
                     'IP': SdbSwap.hostList[index]['IP'],
                     'User': SdbSwap.hostList[index]['User'],
                     'Packages': SdbSwap.hostList[index]['Packages']
                  } 
               ) ;
            }
         } ) ;
         if( hostList.length > 0 )
         {
            $rootScope.tempData( 'Deploy', 'Model',  'Package' ) ;
            $rootScope.tempData( 'Deploy', 'Module', 'None' ) ;
            $rootScope.tempData( 'Deploy', 'HostList', hostList ) ;
            $rootScope.tempData( 'Deploy', 'ClusterName', $scope.ClusterList[ $scope.CurrentCluster ]['ClusterName'] ) ;
            $location.path( '/Deploy/Package' ).search( { 'r': new Date().getTime() } ) ;
         }
         else
         {
            $scope.Components.Confirm.type = 3 ;
            $scope.Components.Confirm.context = $scope.autoLanguage( '至少选择一台主机。' ) ;
            $scope.Components.Confirm.isShow = true ;
            $scope.Components.Confirm.noClose = true ;
         }
      }

      //删除主机 下拉菜单
      $scope.RemoveHostDropdown = {
         'config': [],
         'OnClick': function( index ){
            if( index == 0 )
            {
               $scope.RemoveHost() ;
            }
            else if( index == 1 )
            {
               $scope.UnbindHost() ;
            }
            $scope.RemoveHostDropdown['callback']['Close']() ;
         },
         'callback': {}
      }

      //打开 删除主机下拉菜单
      $scope.OpenRemoveHost = function( event ){
         if( $scope.ClusterList.length > 0 )
         {
            $scope.RemoveHostDropdown['config'] = [] ;
            $scope.RemoveHostDropdown['config'].push( { 'key': $scope.autoLanguage( '卸载主机' ) } ) ;
            $scope.RemoveHostDropdown['config'].push( { 'key': $scope.autoLanguage( '解绑主机' ) } ) ;
            $scope.RemoveHostDropdown['callback']['Open']( event.currentTarget ) ;
         }
      }

      //编辑主机 下拉菜单
      $scope.EditHostDropdown = {
         'config': [],
         'OnClick': function( index ){
            if( index == 0 )
            {
               gotoDeployPackage() ;
            }
            else if( index == 1 )
            {
               showUpdateHostIP() ;
            }
            $scope.EditHostDropdown['callback']['Close']() ;
         },
         'callback': {}
      }

      //打开 编辑主机下拉菜单
      $scope.OpenEditHostDropdown = function( event ){
         if( $scope.ClusterList.length > 0 && $scope.HostNum > 0 )
         {
            $scope.EditHostDropdown['config'] = [] ;
            $scope.EditHostDropdown['config'].push( { 'key': $scope.autoLanguage( '部署包' ) } ) ;
            $scope.EditHostDropdown['config'].push( { 'key': $scope.autoLanguage( '更新主机信息' ) } ) ;
            $scope.EditHostDropdown['callback']['Open']( event.currentTarget ) ;
         }
      }
   } ) ;
}());