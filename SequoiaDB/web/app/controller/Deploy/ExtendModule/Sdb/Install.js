//@ sourceURL=Install.js
(function(){
   var sacApp = window.SdbSacManagerModule ;
   //控制器

   sacApp.controllerProvider.register( 'Deploy.SDB.ExtendInstall.Ctrl', function( $scope, $location, $rootScope, SdbRest, SdbFunction, SdbSwap, SdbSignal ){

      $scope.ContainerBox = [ { offsetY: -70 }, { offsetY: -4 } ] ;

      SdbSwap.taskID     = $rootScope.tempData( 'Deploy', 'ModuleTaskID' ) ;
      $scope.DeployType  = $rootScope.tempData( 'Deploy', 'Model' ) ;
      $scope.ModuleType  = $rootScope.tempData( 'Deploy', 'Module' ) ;
      if( $scope.ModuleType == null || $scope.DeployType == null || SdbSwap.taskID == null )
      {
         $location.path( '/Deploy/Index' ).search( { 'r': new Date().getTime() } ) ;
         return ;
      }

      if( $scope.DeployType != 'Task' )
      {
         $scope.stepList = _Deploy.BuildSdbExtStep( $scope, $location, $scope['Url']['Action'], 'sequoiadb' ) ;
         if( $scope.DeployType != 'Task' && $scope.stepList['info'].length == 0 )
         {
            $location.path( '/Deploy/Index' ).search( { 'r': new Date().getTime() } ) ;
            return ;
         }
      }

      //初始化
      var barColor = 0 ;
      SdbSwap.TaskInfo = {} ;
      $scope.IsError  = false ;
      $scope.IsFinish = false ;

      //循环查询任务信息
      var queryTask = function( taskID ){
         var data = { 'cmd': 'query task', 'filter': JSON.stringify( { 'TaskID': taskID } ) } ;
         SdbRest.OmOperation( data, {
            'success': function( taskInfo ){
               var timeLeft = 0 ;
               if( taskInfo.length > 0 )
               {
                  var errorNum = 0 ;
                  taskInfo = taskInfo[0] ;
                  timeLeft = 0 ;
                  $.each( taskInfo['ResultInfo'], function( index, nodeInfo ){
                     if( nodeInfo['errno'] != 0 )
                     {
                        ++errorNum ;
                        barColor = 1 ;
                     }
                     if( nodeInfo['Status'] == 4 )
                     {
                        return true ;
                     }
                     if( nodeInfo['role'] == 'coord' )
                     {
                        timeLeft += 0.5 ;
                     }
                     else
                     {
                        timeLeft += 1 ;
                     }
                  } ) ;
                  if( taskInfo['ResultInfo'].length == errorNum )
                  {
                     barColor = 2 ;
                  }

                  if( timeLeft == 0 )
                  {
                     timeLeft = 1 ;
                  }

                  SdbSignal.commit( 'UpdateTaskInfo', { 'TaskInfo': taskInfo, 'TimeLeft': timeLeft } ) ;
                  SdbSignal.commit( 'UpdateTaskProgress', { 'TaskInfo': taskInfo, 'ProgressBarColor': barColor } ) ;
                  
                  if( taskInfo['Status'] == 4 )
                  {
                     $scope.IsFinish = true ;
                     if( typeof( taskInfo['errno'] ) == 'number' && taskInfo['errno'] != 0 )
                     {
                        $scope.IsError = true ;
                     }
                  }
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

      queryTask( SdbSwap.taskID ) ;

      //返回
      $scope.GotoPrev = function(){
         if( $scope.DeployType == 'Task' )
         {
            $location.path( '/Deploy/Index' ).search( { 'r': new Date().getTime() } ) ;
         }
         else
         {
            $location.path( '/Deploy/SDB-Extend' ).search( { 'r': new Date().getTime() } ) ;
         }
      }

      //确定
      $scope.GotoDeploy = function(){
         if( $scope.IsFinish == true )
         {
            $location.path( '/Deploy/Index' ).search( { 'r': new Date().getTime() } ) ;
         }
      }
   } ) ;
   
   sacApp.controllerProvider.register( 'Deploy.SDB.ExtendInstall.TaskInfo.Ctrl', function( $scope, SdbRest, SdbSwap, SdbSignal ){
      $scope.TimeLeft = '' ;
      //获取日志 弹窗
      $scope.GetLogWindow = {
         'config': {},
         'callback': {}
      } ;
      $scope.TaskInfo = {} ;

      SdbSignal.on( 'UpdateTaskInfo', function( signalInfo ){
         $scope.TaskInfo = signalInfo['TaskInfo'] ;
         $scope.TimeLeft = signalInfo['TimeLeft'] ;
         if( $scope.TaskInfo['Status'] == 4 )
         {
            $scope.TimeLeft = '' ;
         }
         else
         {
            $scope.TimeLeft = sprintf( $scope.autoLanguage( '?分钟' ), parseInt( $scope.TimeLeft ) ) ;
         }
      } ) ;

      //打开 获取日志 弹窗
      $scope.ShowGetLog = function(){
         var data = { 'cmd': 'get log', 'name': './task/' + SdbSwap.taskID + '.log' } ;
         SdbRest.GetLog( data, function( logstr ){
            $scope.Logstr = logstr ;
         }, function(){
            _IndexPublic.createRetryModel( $scope, null, function(){
               $scope.ShowGetLog() ;
               return true ;
            }, $scope.autoLanguage( '错误' ), $scope.autoLanguage( '获取日志失败。' ) ) ;
         } ) ;
         $scope.GetLogWindow['callback']['SetOkButton']( $scope.autoLanguage( '确定' ) ) ;
         $scope.GetLogWindow['callback']['SetTitle']( $scope.autoLanguage( '日志' ) ) ;
         $scope.GetLogWindow['callback']['Open']() ;
      }
   } ) ;

   sacApp.controllerProvider.register( 'Deploy.SDB.ExtendInstall.TaskTable.Ctrl', function( $scope, SdbSwap, SdbSignal ){
      $scope.BarColor = 0 ;
      //任务表格
      $scope.TaskTable = {
         'title': {
            'Status':         '',
            'HostName':       $scope.autoLanguage( '主机名' ),
            'svcname':        $scope.autoLanguage( '服务名' ),
            'role':           $scope.autoLanguage( '角色' ),
            'datagroupname':  $scope.autoLanguage( '分区组' ),
            'StatusDesc':     $scope.autoLanguage( '状态' ),
            'Flow':           $scope.autoLanguage( '描述' )
         },
         'body': [],
         'options': {
            'width': {
               'Status': '24px',
               'HostName': '25%',
               'svcname': '15%',
               'role': '100px',
               'datagroupname': '15%',
               'StatusDesc': '10%',
               'Flow': '35%'
            },
            'max': 50
         }
      } ;
      $scope.Progress = 0 ;

      var firstTime = true ;

      //更新任务进度
      var updateTaskProgress = function( result ){
         //输出到表格task的数据
         var taskInfo = result['TaskInfo'] ;
         $scope.BarColor = result['ProgressBarColor']
         $scope.Progress = taskInfo['Progress'] ;
         if( taskInfo['Status'] == 4 )
         {
            $scope.BarColor = 3 ;
            $scope.TimeLeft = '' ;
            if( typeof( taskInfo['errno'] ) == 'number' && taskInfo['errno'] != 0 )
            {
               $scope.Progress = 100 ;
               $scope.BarColor = 2 ;
               $.each( taskInfo['ResultInfo'], function( index, hostInfo ){
                  if( taskInfo['ResultInfo'][index]['errno'] == 0 )
                  {
                     taskInfo['ResultInfo'][index]['errno'] = 1 ;
                  }
                  taskInfo['ResultInfo'][index]['Status'] = 4 ;
                  taskInfo['ResultInfo'][index]['StatusDesc'] = 'FINISH' ;
               } ) ;
            }
         }
         else
         {
            if( taskInfo['Status'] == 2 )
            {
               $scope.BarColor = 1 ;
            }
            $scope.TimeLeft = sprintf( $scope.autoLanguage( '?分钟' ), parseInt( $scope.TimeLeft ) ) ;
            if( $scope.Progress == 100 )
            {
               $scope.Progress = 90 ;
            }
         }
                  
         if( firstTime == true )
         {
            $scope.TaskTable['body'] = taskInfo['ResultInfo'] ;
         }
         $.each( $scope.TaskTable['body'], function( index, hostInfo ){
            if( hostInfo['Status'] == taskInfo['ResultInfo'][index]['Status'] && hostInfo['errno'] == taskInfo['ResultInfo'][index]['errno'] )
            {
               hostInfo['StatusDesc'] = taskInfo['ResultInfo'][index]['StatusDesc'] ;
               hostInfo['detail']     = taskInfo['ResultInfo'][index]['detail'] ;
               hostInfo['Flow']       = taskInfo['ResultInfo'][index]['Flow'] ;
            }
            else
            {
               hostInfo['Status']     = taskInfo['ResultInfo'][index]['Status'] ;
               hostInfo['StatusDesc'] = taskInfo['ResultInfo'][index]['StatusDesc'] ;
               hostInfo['detail']     = taskInfo['ResultInfo'][index]['detail'] ;
               hostInfo['Flow']       = taskInfo['ResultInfo'][index]['Flow'] ;
               hostInfo['errno']      = taskInfo['ResultInfo'][index]['errno'] ;
            }
         } ) ;

         firstTime = false ;
         //$scope.$apply() ;
      }

      SdbSignal.on( 'UpdateTaskProgress', function( signalInfo ){
         updateTaskProgress( signalInfo ) ;
      } ) ;

   } ) ;
}());