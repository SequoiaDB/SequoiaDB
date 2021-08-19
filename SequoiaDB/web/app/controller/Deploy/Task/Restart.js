//@ sourceURL=Task.js
//"use strict" ;
(function(){
   var sacApp = window.SdbSacManagerModule ;
   //控制器
   sacApp.controllerProvider.register( 'Deploy.Task.Restart.Ctrl', function( $scope, $compile, $location, $rootScope, SdbRest, SdbFunction ){

      //初始化
      $scope.IsFinish     = false ;
      $scope.IsError      = false ;
      $scope.TimeLeft     = '' ;
      $scope.BarColor     = 0 ;
      //任务表格
      $scope.TaskTable = {
         'title': {},
         'options': {
            'max': 50
         }
      } ;
      $scope.TaskInfo = [] ;

      //输出到表格task的数据
      $scope.NewTaskInfo = [] ;

      var installTask = $rootScope.tempData( 'Deploy', 'ModuleTaskID' ) ;
      var shrink = $rootScope.tempData( 'Deploy', 'Shrink' ) ;
      $scope.DeployType  = $rootScope.tempData( 'Deploy', 'Model' ) ;
      $scope.ModuleType  = $rootScope.tempData( 'Deploy', 'Module' ) ;
      if( $scope.DeployType == null || $scope.ModuleType == null || installTask == null )
      {
         $location.path( '/Deploy/Index' ).search( { 'r': new Date().getTime() } ) ;
         return ;
      }

      function setTaskTable( moduleType )
      {
         //任务表格
         if( moduleType == 'sequoiadb' )
         {
            $scope.TaskTable['title'] = {
               'Status':         '',
               'HostName':       $scope.autoLanguage( '主机名' ),
               'svcname':        $scope.autoLanguage( '服务名' ),
               'role':           $scope.autoLanguage( '角色' ),
               'datagroupname':  $scope.autoLanguage( '分区组' ),
               'StatusDesc':     $scope.autoLanguage( '状态' ),
               'Flow':           $scope.autoLanguage( '描述' )
            } ;
            $scope.TaskTable['options']['width'] = {
               'Status':         '24px',
               'HostName':       '25%',
               'svcname':        '15%',
               'role':           '100px',
               'datagroupname':  '15%',
               'StatusDesc':     '10%',
               'Flow':           '35%'
            } ;
         }
         else if( moduleType == 'sequoiasql-postgresql' || moduleType == 'sequoiasql-mysql' )
         {
            $scope.TaskTable['title'] = {
               'Status':         '',
               'HostName':       $scope.autoLanguage( '主机名' ),
               'port':           $scope.autoLanguage( '端口' ),
               'StatusDesc':     $scope.autoLanguage( '状态' ),
               'Flow':           $scope.autoLanguage( '描述' )
            } ;
            $scope.TaskTable['options']['width'] = {
               'Status':      '24px',
               'HostName':    '30%',
               'port':        '15%',
               'StatusDesc':  '25%',
               'Flow':        '30%'
            } ;
         }
      }

      setTaskTable( $scope.ModuleType ) ;

      //上一步
      $scope.GotoMod = function(){
         if( $scope.ModuleType == 'sequoiadb' )
         {
            $location.path( '/Deploy/SDB-Mod' ).search( { 'r': new Date().getTime() } ) ;
         }
         else if( $scope.ModuleType == 'sequoiasql' )
         {
            $location.path( '/Deploy/SSQL-Mod' ).search( { 'r': new Date().getTime() } ) ;
         }
         else if( $scope.ModuleType == 'zookeeper' )
         {
            $location.path( '/Deploy/ZKP-Mod' ).search( { 'r': new Date().getTime() } ) ;
         }
         else if( $scope.ModuleType == 'sequoiasql-postgresql' )
         {
            $location.path( '/Deploy/PostgreSQL-Mod' ).search( { 'r': new Date().getTime() } ) ;
         }
         else if( $scope.ModuleType == 'sequoiasql-mysql' )
         {
            $location.path( '/Deploy/MySQL-Mod' ).search( { 'r': new Date().getTime() } ) ;
         }
      }

      //返回
      $scope.GotoDeploy2 = function(){
          $location.path( '/Deploy/Index' ).search( { 'r': new Date().getTime() } ) ;
      }

      //下一步
      $scope.GotoDeploy = function(){
         if( $scope.IsFinish == true )
         {
            if( $scope.DeployType == 'Update Config' )
            {
               if ( 'sequoiadb' == $scope.ModuleType )
               {
                  $location.path( '/Config/SDB/Index' ).search( { 'r': new Date().getTime() } ) ;
                  return ;
               }
               else if ( 'sequoiasql-postgresql' == $scope.ModuleType )
               {
                  $location.path( '/Config/SequoiaSQL/PostgreSQL/Index' ).search( { 'r': new Date().getTime() } ) ;
                  return ;
               }
               else if ( 'sequoiasql-mysql' == $scope.ModuleType )
               {
                  $location.path( '/Config/SequoiaSQL/MySQL/Index' ).search( { 'r': new Date().getTime() } ) ;
                  return ;
               }
            }

            $location.path( '/Deploy/Index' ).search( { 'r': new Date().getTime() } ) ;
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

      var firstTime = true ;

      //循环查询任务信息
      var queryTask = function( taskID ){

         var data = { 'cmd': 'query task', 'filter': JSON.stringify( { 'TaskID': taskID } ) } ;
         SdbRest.OmOperation( data, {
            'success': function( taskInfo ){
               $scope.TimeLeft = '' ;
               if( taskInfo.length > 0 )
               {
                  $scope.TaskInfo = taskInfo[0] ;

                  //因为从任务管理器跳转进来的，是不知道任务是什么业务的，所以通过字段来判断
                  if( $scope.ModuleType == 'None' )
                  {
                     $scope.ModuleType = taskInfo[0]['Info']['BusinessType'] ;
                     setTaskTable( $scope.ModuleType ) ;
                  }

                  $scope.BarColor = 0 ;
                  $scope.TimeLeft = 0 ;
                  var errorNum = 0 ;
                  if( $scope.ModuleType == 'sequoiadb' )
                  {
                     $.each( $scope.TaskInfo['ResultInfo'], function( index, nodeInfo ){
                        if( nodeInfo['errno'] != 0 )
                        {
                           ++errorNum ;
                           $scope.BarColor = 1 ;
                        }
                        if( nodeInfo['Status'] == 4 )
                        {
                           return true ;
                        }
                        if( nodeInfo['role'] == 'coord' )
                        {
                           $scope.TimeLeft += 0.3 ;
                        }
                        else
                        {
                           $scope.TimeLeft += 0.5 ;
                        }
                     } ) ;
                     if( $scope.TaskInfo['ResultInfo'].length == errorNum )
                     {
                        $scope.BarColor = 2 ;
                     }
                  }
                  else if( $scope.ModuleType == 'sequoiasql-postgresql' || $scope.ModuleType == 'sequoiasql-mysql' )
                  {
                     $.each( $scope.TaskInfo['ResultInfo'], function( index, nodeInfo ){
                        if( nodeInfo['errno'] != 0 )
                        {
                           ++errorNum ;
                           $scope.BarColor = 1 ;
                        }
                        if( nodeInfo['Status'] == 4 )
                        {
                           return true ;
                        }
                        $scope.TimeLeft += 1 ;
                     } ) ;
                     if( $scope.TaskInfo['ResultInfo'].length == errorNum )
                     {
                        $scope.BarColor = 2 ;
                     }
                  }
                  else
                  {
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
                        $scope.IsError = true ;
                        $scope.TaskInfo['Progress'] = 100 ;
                        $scope.BarColor = 2 ;
                        $.each( $scope.TaskInfo['ResultInfo'], function( index, hostInfo ){
                           if( $scope.TaskInfo['ResultInfo'][index]['errno'] == 0 )
                           {
                              $scope.TaskInfo['ResultInfo'][index]['errno'] = 1 ;
                           }
                           $scope.TaskInfo['ResultInfo'][index]['Status'] = 4 ;
                           $scope.TaskInfo['ResultInfo'][index]['StatusDesc'] = 'FINISH' ;
                        } ) ;
                     }
                  }
                  else
                  {
                     if( $scope.TaskInfo['Status'] == 2 )
                     {
                        $scope.BarColor = 1 ;
                     }
                     $scope.TimeLeft = sprintf( $scope.autoLanguage( '?分钟' ), parseInt( $scope.TimeLeft ) ) ;
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
                     if( hostInfo['Status'] == $scope.TaskInfo['ResultInfo'][index]['Status'] &&
                         hostInfo['errno'] == $scope.TaskInfo['ResultInfo'][index]['errno'] )
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
                  $rootScope.bindResize() ;
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