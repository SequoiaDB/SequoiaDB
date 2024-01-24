//@ sourceURL=Host.js
//"use strict" ;
(function(){
   var sacApp = window.SdbSacManagerModule ;
   //控制器
   sacApp.controllerProvider.register( 'Deploy.Task.Install.Ctrl', function( $scope, $compile, $location, $rootScope, SdbRest, SdbFunction ){

      //初始化
      $scope.ContainerBox = [ { offsetY: -70 }, { offsetY: -4 } ] ;
      $scope.IsFinish            = false ;
      $scope.TimeLeft            = '' ;
      $scope.BarColor            = 0 ;

      $scope.DeployType  = $rootScope.tempData( 'Deploy', 'Model' ) ;
      $scope.ModuleType  = $rootScope.tempData( 'Deploy', 'Module' ) ;
      var installTask    = $rootScope.tempData( 'Deploy', 'HostTaskID' ) ;
      var discoverConf   = $rootScope.tempData( 'Deploy', 'DiscoverConf' ) ;
      var syncConf       = $rootScope.tempData( 'Deploy', 'SyncConf' ) ;

      $scope.TaskInfo = [] ;
      //输出到表格的task数据
      $scope.NewTaskInfo = [] ;
      
      //第一次加载
      var firstTime = true ;

      //任务表格
      $scope.TaskTable = {
         'title': {
            'Status':         '',
            'HostName':       $scope.autoLanguage( '主机名' ),
            'IP':             $scope.autoLanguage( 'IP地址' ),
            'StatusDesc':     $scope.autoLanguage( '状态' ),
            'Flow':           $scope.autoLanguage( '描述' )
         },
         'options': {
            'width':{
               'Status': '24px',
               'HostName': '30%',
               'IP': '20%',
               'StatusDesc': '15%',
               'Flow': '35%'
            },
            'max': 50
         }
      } ;

      if( $scope.DeployType == null || $scope.ModuleType == null || installTask == null )
      {
         $location.path( '/Deploy/Index' ).search( { 'r': new Date().getTime() } ) ;
         return ;
      }

      if( $scope.DeployType != 'Task' )
      {
         if( discoverConf != null )
         {
            $scope.stepList = _Deploy.BuildDiscoverStep( $scope, $location, $scope['Url']['Method'], $scope.ModuleType ) ;
         }
         else if( syncConf != null )
         {
            $scope.stepList = _Deploy.BuildSyncStep( $scope, $location, $scope['Url']['Method'], $scope.ModuleType ) ;
         }
         else if( $scope.DeployType == 'Package' )
         {
            $scope.stepList = _Deploy.BuildDeployPackageStep( $scope, $location, $scope['Url']['Method'], $scope.DeployType ) ;
         }
         else
         {
            $scope.stepList = _Deploy.BuildSdbStep( $scope, $location, $scope.DeployType, $scope['Url']['Method'], $scope.ModuleType ) ;
         }
         if( $scope.stepList['info'].length == 0 )
         {
            $location.path( '/Deploy/Index' ).search( { 'r': new Date().getTime() } ) ;
            return ;
         }
      }

      
      //部署包上一步
      $scope.GotoPackage = function(){
         $location.path( '/Deploy/Package' ).search( { 'r': new Date().getTime() } ) ;
      }

      //安装主机上一步
      $scope.GotoAddHost = function(){
         $location.path( '/Deploy/AddHost' ).search( { 'r': new Date().getTime() } ) ;
      }

      //返回
      $scope.GotoDeploy2 = function(){
         $location.path( '/Deploy/Index' ).search( { 'r': new Date().getTime() } ) ;
      }

      //前往发现业务
      var gotoDiscover = function(){
         if( $scope.IsFinish == true )
         {
            var data = { 'cmd': 'discover business', 'ConfigInfo': JSON.stringify( discoverConf ) } ;
            SdbRest.OmOperation( data, {
               'success': function(){
                  $rootScope.tempData( 'Deploy', 'ModuleName', discoverConf['BusinessName'] ) ;
                  $rootScope.tempData( 'Deploy', 'ClusterName', discoverConf['ClusterName'] );
                  if ( $scope.ModuleType == 'sequoiadb' )
                  {
                     $location.path( '/Deploy/SDB-Discover' ).search( { 'r': new Date().getTime() } );
                  }
                  else if ( $scope.ModuleType == 'sequoiasql-mysql' )
                  {
                     var hostName = $rootScope.tempData( 'Deploy', 'ModuleHostName' ) ;

                     $.each( $scope.TaskInfo['ResultInfo'], function( index, info ){
                        if( hostName == info['IP'] )
                        {
                           hostName = info['HostName'] ;
                           return false ;
                        }
                     } ) ;

                     $rootScope.tempData( 'Deploy', 'ModuleHostName', hostName ) ;
                     $location.path( '/Deploy/MYSQL-Discover' ).search( { 'r': new Date().getTime() } );
                  }
               }, 
               'failed': function( errorInfo ){
                  _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                     gotoDiscover() ;
                     return true ;
                  } ) ;
               }
            } ) ;
         }
      }

      //前往同步业务
      var gotoSync = function(){
         $location.path( '/Deploy/SDB-Sync' ).search( { 'r': new Date().getTime() } ) ;
      }

      //完成
      $scope.GotoDeploy = function(){
         if( $scope.IsFinish == true )
         {
            $location.path( '/Deploy/Index' ).search( { 'r': new Date().getTime() } ) ;
         }
      }

      //下一步
      $scope.GotoNext = function(){
         if( $scope.IsFinish == true )
         {
            if( discoverConf != null )
            {
               gotoDiscover() ;
            }
            else if( syncConf != null )
            {
               gotoSync() ;
            }
            else
            {
               if( $scope.ModuleType == 'sequoiadb' )
               {
                  $location.path( '/Deploy/SDB-Conf' ).search( { 'r': new Date().getTime() } ) ;
               }
               else if( $scope.ModuleType == 'sequoiasql' )
               {
                  $location.path( '/Deploy/SSQL-Conf' ).search( { 'r': new Date().getTime() } ) ;
               }
               else if( $scope.ModuleType == 'zookeeper' )
               {
                  $location.path( '/Deploy/ZKP-Mod' ).search( { 'r': new Date().getTime() } ) ;
               }
               else
               {
                  $location.path( '/Deploy/Index' ).search( { 'r': new Date().getTime() } ) ;
               }
            }
         }
      }

      //获取日志 弹窗
      $scope.GetLogWindow = {
         'config': {},
         'callback': {}
      } ;

      //打开 获取日志 弹窗
      $scope.ShowGetLog = function(){
         var data = { 'cmd': 'get log', 'name': './task/' + installTask + '.log' } ;
         SdbRest.GetLog( data, function( logstr ){
            var browser = SdbFunction.getBrowserInfo() ;
            if( browser[0] == 'ie' && browser[1] == 7 )
            {
               logstr = logstr.replace( /\n/gi, '\n\r' ) ;
            }
            $scope.Logstr = logstr ;
         }, function(){
            _IndexPublic.createRetryModel( $scope, null, function(){
               $scope.ShowGetLog() ;
               return true ;
            }, $scope.autoLanguage( '错误' ), $scope.autoLanguage( '获取日志失败。' ) ) ;
         } ) ;
         $scope.GetLogWindow['callback']['SetTitle']( $scope.autoLanguage( '日志' ) ) ;
         $scope.GetLogWindow['callback']['SetCloseButton']( $scope.autoLanguage( '关闭' ), function(){
            $scope.GetLogWindow['callback']['Close']() ;
         } ) ;
         $scope.GetLogWindow['callback']['Open']() ;
      }

      //循环查询任务信息
      var queryTask = function( taskID ){
         $rootScope.tempData( 'Deploy', 'InstallTask', taskID ) ;
         var data = { 'cmd': 'query task', 'filter': JSON.stringify( { 'TaskID': taskID } ) } ;
         SdbRest.OmOperation( data, {
            'success': function( taskInfo ){
               $scope.TimeLeft = '' ;
               if( taskInfo.length > 0 )
               {
                  $scope.TaskInfo = taskInfo[0] ;
                  $scope.BarColor = 0 ;
                  $scope.TimeLeft = 0 ;
                  var errorNum = 0 ;
                  $.each( $scope.TaskInfo['ResultInfo'], function( index, hostInfo ){
                     if( hostInfo['errno'] != 0 )
                     {
                        ++errorNum ;
                        $scope.BarColor = 1 ;
                     }
                     if( hostInfo['Status'] == 4 )
                     {
                        return true ;
                     }
                     $scope.TimeLeft += 2 ;
                  } ) ;
                  if( $scope.TaskInfo['ResultInfo'].length == errorNum )
                  {
                     $scope.BarColor = 2 ;
                  }

                  if( $scope.TimeLeft == 0 )
                  {
                     $scope.TimeLeft = 1 ;
                  }

                  if( $scope.TaskInfo['Status'] == 4 )
                  {
                     $scope.BarColor = 3 ;
                     $scope.TimeLeft = '' ;
                     $scope.IsFinish = true ;
                     if( typeof( $scope.TaskInfo['errno'] ) == 'number' && $scope.TaskInfo['errno'] != 0 )
                     {
                        $scope.TaskInfo['Progress'] = 100 ;
                        $scope.BarColor = 2 ;
                        var errorNum = 0 ;
                        $.each( $scope.TaskInfo['ResultInfo'], function( index, hostInfo ){
                           if( $scope.TaskInfo['ResultInfo'][index]['errno'] == 0 )
                           {
                              $scope.TaskInfo['ResultInfo'][index]['errno'] = 1 ;
                              ++errorNum ;
                           }
                           $scope.TaskInfo['ResultInfo'][index]['Status'] = 4 ;
                           $scope.TaskInfo['ResultInfo'][index]['StatusDesc'] = 'FINISH' ;
                        } ) ;
                        //检查如果全失败，就禁止到下一步
                        if( errorNum >= $scope.TaskInfo['ResultInfo'].length )
                        {
                           $scope.IsFinish = false ;
                        }
                     }
                  }
                  else
                  {
                     if( $scope.TaskInfo['Status'] == 2 )
                     {
                        $scope.BarColor = 1 ;
                     }
                     $scope.TimeLeft = sprintf( $scope.autoLanguage( '?分钟' ), $scope.TimeLeft ) ;
                     if( $scope.TaskInfo['Progress'] == 100 )
                     {
                        $scope.TaskInfo['Progress'] = 90 ;
                     }
                  }

                  if( firstTime == true )
                  {
                     $scope.NewTaskInfo = $scope.TaskInfo ;
                  }
                  $.each( $scope.NewTaskInfo['ResultInfo'], function( index, hostInfo ){
                     if( hostInfo['Status'] == $scope.TaskInfo['ResultInfo'][index]['Status'] && hostInfo['errno'] == $scope.TaskInfo['ResultInfo'][index]['errno'] )
                     {
                        $scope.NewTaskInfo['ResultInfo'][index]['StatusDesc'] = $scope.TaskInfo['ResultInfo'][index]['StatusDesc'] ;
                        $scope.NewTaskInfo['ResultInfo'][index]['detail'] = $scope.TaskInfo['ResultInfo'][index]['detail'] ;
                        $scope.NewTaskInfo['ResultInfo'][index]['Flow'] = $scope.TaskInfo['ResultInfo'][index]['Flow'] ;
                     }
                     else
                     {
                        $scope.NewTaskInfo['ResultInfo'][index]['Status'] = $scope.TaskInfo['ResultInfo'][index]['Status'] ;
                        $scope.NewTaskInfo['ResultInfo'][index]['StatusDesc'] = $scope.TaskInfo['ResultInfo'][index]['StatusDesc'] ;
                        $scope.NewTaskInfo['ResultInfo'][index]['detail'] = $scope.TaskInfo['ResultInfo'][index]['detail'] ;
                        $scope.NewTaskInfo['ResultInfo'][index]['Flow'] = $scope.TaskInfo['ResultInfo'][index]['Flow'] ;
                        $scope.NewTaskInfo['ResultInfo'][index]['errno'] = $scope.TaskInfo['ResultInfo'][index]['errno'] ;
                     }
                  } ) ;
                  
                  firstTime = false ;
                  //$rootScope.bindResize() ;
                  $scope.$apply() ;

                  if( $scope.IsFinish == false )
                  {
                     SdbFunction.Timeout( function(){
                        queryTask( taskID ) ;
                     }, 1500 ) ;
                  }
               }
               else
               {
                  _IndexPublic.createRetryModel( $scope, null, function(){
                     queryTask( taskID ) ;
                     return true ;
                  }, $scope.autoLanguage( '获取任务信息失败' ), $scope.autoLanguage( '获取任务信息失败，该任务不存在。' ) ) ;
               }
            },
            'failed': function( errorInfo ){
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  queryTask( taskID ) ;
                  return true ;
               } ) ;
            }
         }, {
            'showLoading': false
         } ) ;
      }

      queryTask( installTask ) ;

   } ) ;
}());