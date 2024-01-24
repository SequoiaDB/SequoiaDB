//@ sourceURL=Module.js
//"use strict" ;
(function(){
   var sacApp = window.SdbSacManagerModule ;
   //控制器
   sacApp.controllerProvider.register( 'Deploy.Task.Install.Ctrl', function( $scope, $compile, $location, $rootScope, $timeout, SdbRest, SdbFunction ){

      //初始化
      $scope.ContainerBox = [ { offsetY: -70 }, { offsetY: -4 } ] ;
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
      //添加服务 弹窗
      $scope.InstallModule = {
         'config': {},
         'callback': {}
      } ;
      $scope.TaskInfo = [] ;

      //输出到表格task的数据
      $scope.NewTaskInfo = [] ;

      //复合任务
      var isSecondTask   = false ;
      var isCompoundTask = false ;

      var installTask = $rootScope.tempData( 'Deploy', 'ModuleTaskID' ) ;
      var shrink = $rootScope.tempData( 'Deploy', 'Shrink' ) ;
      $scope.DeployType  = $rootScope.tempData( 'Deploy', 'Model' ) ;
      $scope.ModuleType  = $rootScope.tempData( 'Deploy', 'Module' ) ;
      var secondTask = $rootScope.tempData( 'Deploy', 'SecondTask' ) ;
      if( $scope.DeployType == null || $scope.ModuleType == null || installTask == null )
      {
         $location.path( '/Deploy/Index' ).search( { 'r': new Date().getTime() } ) ;
         return ;
      }

      if ( secondTask )
      {
         $rootScope.tempData( 'Deploy', 'SecondTask', null ) ;
         isCompoundTask = true ;
      }

      if( $scope.DeployType != 'Task' )
      {
         if( shrink == true )
         {
            $scope.stepList = _Deploy.BuildSdbShrinkStep( $scope, $location, $scope['Url']['Method'], 'sequoiadb' ) ;
         }
         else if( $scope.ModuleType == 'sequoiasql-postgresql' )
         {
            $scope.stepList = _Deploy.BuildSdbPgsqlStep( $scope, $location, $scope['Url']['Method'] ) ;
         }
         else if( $scope.ModuleType == 'sequoiasql-mysql' )
         {
            $scope.stepList = _Deploy.BuildSdbMysqlStep( $scope, $location, $scope['Url']['Method'] ) ;
         }
         else if( $scope.ModuleType == 'sequoiasql-mariadb' )
         {
            $scope.stepList = _Deploy.BuildSdbMysqlStep( $scope, $location, $scope['Url']['Method'] ) ;
         }
         else
         {
            $scope.stepList = _Deploy.BuildSdbStep( $scope, $location, $scope.DeployType, $scope['Url']['Method'], $scope.ModuleType ) ;
         }

         if( $scope.DeployType != 'Task' && $scope.stepList['info'].length == 0 )
         {
            $location.path( '/Deploy/Index' ).search( { 'r': new Date().getTime() } ) ;
            return ;
         }
      }

      var setTaskTable = function( moduleType ){
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
         else if( moduleType == 'sequoiasql' )
         {
            $scope.TaskTable['title'] = {
               'Status':         '',
               'HostName':       $scope.autoLanguage( '主机名' ),
               'role':           $scope.autoLanguage( '角色' ),
               'StatusDesc':     $scope.autoLanguage( '状态' ),
               'Flow':           $scope.autoLanguage( '描述' )
            } ;
            $scope.TaskTable['options']['width'] = {
               'Status':      '24px',
               'HostName':    '30%',
               'role':        '20%',
               'StatusDesc':  '20%',
               'Flow':        '30%'
            } ;
         }
         else if( moduleType == 'zookeeper' )
         {
            $scope.TaskTable['title'] = {
               'Status':         '',
               'zooid':          $scope.autoLanguage( '节点Id' ),
               'HostName':       $scope.autoLanguage( '主机名' ),
               'StatusDesc':     $scope.autoLanguage( '状态' ),
               'Flow':           $scope.autoLanguage( '描述' )
            } ;
            $scope.TaskTable['options']['width'] = {
               'Status':      '24px',
               'zooid':       '30%',
               'HostName':    '15%',
               'StatusDesc':  '15%',
               'Flow':        '40%'
            } ;
         }
         else if( moduleType == 'sequoiasql-postgresql' || moduleType == 'sequoiasql-mysql' || moduleType == 'sequoiasql-mariadb' )
         {
            $scope.TaskTable['title'] = {
               'Status':         '',
               'HostName':       $scope.autoLanguage( '主机名' ),
               'StatusDesc':     $scope.autoLanguage( '状态' ),
               'Flow':           $scope.autoLanguage( '描述' )
            } ;
            $scope.TaskTable['options']['width'] = {
               'Status':      '24px',
               'HostName':    '35%',
               'StatusDesc':  '30%',
               'Flow':        '35%'
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
         else if( $scope.ModuleType == 'sequoiasql-mariadb' )
         {
            $location.path( '/Deploy/Mariadb-Mod' ).search( { 'r': new Date().getTime() } ) ;
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

      function execSecondTask( installConfig )
      {
         var data = { 'cmd': 'add business', 'Force': true, 'ConfigInfo': JSON.stringify( installConfig ) } ;

         if( installConfig['BusinessType'] == 'sequoiasql-mysql' )
         {
            data['Force'] = true ;
         }

         SdbRest.OmOperation( data, {
            'success': function( taskInfo ){
               $scope.IsFinish = false ;
               isSecondTask = true ;
               installTask = taskInfo[0]['TaskID'] ;
               queryTask( taskInfo[0]['TaskID'] ) ;
            },
            'failed': function( errorInfo ){
               $scope.BarColor = 2 ;
               $scope.TimeLeft = '' ;
               $scope.IsFinish = true ;
               $scope.IsError = true ;
               $scope.TaskInfo['Progress'] = 100 ;
               $scope.TaskInfo['TaskName'] = 'ADD_BUSINESS' ;
               $scope.TaskInfo['Info']['BusinessName'] = installConfig['BusinessName'] ;

               $.each( $scope.NewTaskInfo['ResultInfo'], function( index, hostInfo ){
                  $scope.NewTaskInfo['ResultInfo'][index]['errno'] = 1 ;
                  $scope.NewTaskInfo['ResultInfo'][index]['Status'] = 4 ;
                  $scope.NewTaskInfo['ResultInfo'][index]['StatusDesc'] = 'FINISH' ;
                  $scope.NewTaskInfo['ResultInfo'][index]['Flow'].push( 'Failed to add service, errno: ' + errorInfo['errno'] ) ;
               } ) ;
            }
         }, { 'showLoading': false } ) ;
      }

      //打开 添加服务 弹窗
      function ShowInstallModule()
      {
         $scope.InstallModule['config'] = {
            inputList: [
               {
                  "name": 'moduleName',
                  "webName": $scope.autoLanguage( '实例名' ),
                  "type": "string",
                  "required": true,
                  "value": 'MySQLInstance',
                  "valid": {
                     "min": 1,
                     "max": 127,
                     'regex': '^[0-9a-zA-Z_-]+$'
                  }
               },
               {
                  "name": 'moduleType',
                  "webName": $scope.autoLanguage( '实例类型' ),
                  "type": "select",
                  "value": 'sequoiasql-mysql',
                  "valid": [
                     { 'key': 'PostgreSQL', 'value': 'sequoiasql-postgresql' },
                     { 'key': 'MySQL', 'value': 'sequoiasql-mysql' },
                     { 'key': 'MariaDB', 'value': 'sequoiasql-mariadb' },
                  ],
                  "onChange": function( name, key, value ){
                     $scope.InstallModule['config']['inputList'][0]['value'] = key + 'Instance' ;
                  }
               }
            ]
         } ;
            
         $scope.InstallModule['callback']['SetOkButton']( $scope.autoLanguage( '确定' ), function(){
            var isAllClear = $scope.InstallModule['config'].check() ;
            if( isAllClear )
            {
               var formVal = $scope.InstallModule['config'].getValue() ;
               $rootScope.tempData( 'Deploy', 'Model', 'Module' ) ;
               $rootScope.tempData( 'Deploy', 'Module', formVal['moduleType'] ) ;
               $rootScope.tempData( 'Deploy', 'ModuleName', formVal['moduleName'] ) ;
               $rootScope.tempData( 'Deploy', 'ClusterName', $scope.TaskInfo['Info']['ClusterName'] ) ;

               //当服务类型是postgresql时
               if( formVal['moduleType'] == 'sequoiasql-postgresql' )
               {
                  var businessConf = {} ;
                  businessConf['ClusterName']  = $scope.TaskInfo['Info']['ClusterName'] ;
                  businessConf['BusinessName'] = formVal['moduleName'] ;
                  businessConf['BusinessType'] = formVal['moduleType'] ;
                  businessConf['DeployMod']    = '' ;
                  businessConf['Property']     = [] ;
                  $rootScope.tempData( 'Deploy', 'ModuleConfig', businessConf ) ;
                  $location.path( '/Deploy/PostgreSQL-Mod' ).search( { 'r': new Date().getTime() } ) ;
               }
               else if( formVal['moduleType'] == 'sequoiasql-mysql' )
               {
                  var businessConf = {} ;
                  businessConf['ClusterName']  = $scope.TaskInfo['Info']['ClusterName'] ;
                  businessConf['BusinessName'] = formVal['moduleName'] ;
                  businessConf['BusinessType'] = formVal['moduleType'] ;
                  businessConf['DeployMod']    = '' ;
                  businessConf['Property']     = [] ;
                  $rootScope.tempData( 'Deploy', 'ModuleConfig', businessConf ) ;
                  $location.path( '/Deploy/MySQL-Mod' ).search( { 'r': new Date().getTime() } ) ;
               }
               else if( formVal['moduleType'] == 'sequoiasql-mariadb' )
               {
                  var businessConf = {} ;
                  businessConf['ClusterName']  = $scope.TaskInfo['Info']['ClusterName'] ;
                  businessConf['BusinessName'] = formVal['moduleName'] ;
                  businessConf['BusinessType'] = formVal['moduleType'] ;
                  businessConf['DeployMod']    = '' ;
                  businessConf['Property']     = [] ;
                  $rootScope.tempData( 'Deploy', 'ModuleConfig', businessConf ) ;
                  $location.path( '/Deploy/MariaDB-Mod' ).search( { 'r': new Date().getTime() } ) ;
               }
            }
            return isAllClear ;
         } ) ;
         $scope.InstallModule['callback']['SetTitle']( $scope.autoLanguage( '创建实例' ) ) ;
         $scope.InstallModule['callback']['SetIcon']( '' ) ;
         $scope.InstallModule['callback']['Open']() ;
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
                     $scope.ModuleDesc = 'SequoiaDB' ;

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
                           $scope.TimeLeft += 0.5 ;
                        }
                        else
                        {
                           $scope.TimeLeft += 1 ;
                        }
                     } ) ;
                     if( $scope.TaskInfo['ResultInfo'].length == errorNum )
                     {
                        $scope.BarColor = 2 ;
                     }
                  }
                  else if( $scope.ModuleType == 'zookeeper' || $scope.ModuleType == 'sequoiasql' ||
                           $scope.ModuleType == 'sequoiasql-postgresql' ||
                           $scope.ModuleType == 'sequoiasql-mysql' ||
                           $scope.ModuleType == 'sequoiasql-mariadb' )
                  {
                     if ( $scope.ModuleType == 'sequoiasql-mysql' )
                     {
                        $scope.ModuleDesc = 'MySQL' ;
                     }
                     else if ( $scope.ModuleType == 'sequoiasql-mysql' )
                     {
                        $scope.ModuleDesc = 'MariaDB' ;
                     }
                     else if ( $scope.ModuleType == 'sequoiasql-postgresql' )
                     {
                        $scope.ModuleDesc = 'PostgreSQL' ;
                     }

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

                  if ( isCompoundTask && isSecondTask == false )
                  {
                     $scope.TimeLeft += 1 ;
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
                  $rootScope.bindResize() ;
                  $scope.$apply() ;

                  if ( isCompoundTask )
                  {
                     if ( isSecondTask == false )
                     {
                        $scope.TaskInfo['Progress'] = parseInt( $scope.TaskInfo['Progress'] / 2 ) ;
                     }
                     else
                     {
                        $scope.TaskInfo['Progress'] = parseInt( $scope.TaskInfo['Progress'] / 2 ) + 50 ;
                     }
                  }

                  if( $scope.IsFinish == false )
                  {
                     SdbFunction.Timeout( function(){
                        queryTask( taskID ) ;
                     }, 1500 ) ;
                  }
                  else
                  {
                     if ( ( isCompoundTask == false || isSecondTask == true ) && errorNum == 0 && $scope.TaskInfo['TaskName'] == 'ADD_BUSINESS' )
                     {
                        SdbFunction.LocalData( 'ShowRelationTip', null ) ;
                     }

                     if ( isSecondTask == false && isCompoundTask && errorNum == 0 )
                     {
                        $scope.BarColor = 0 ;
                        execSecondTask( secondTask ) ;
                     }
                     else if( isCompoundTask && errorNum > 0 )
                     {
                        $scope.TaskInfo['Progress'] = 100 ;
                        $scope.BarColor = 2 ;
                     }
                     else if( ( $scope.DeployType == 'Deploy' || $scope.DeployType == 'Module' ) &&
                              $scope.ModuleType == 'sequoiadb' &&
                              $scope.TaskInfo['TaskName'] == 'ADD_BUSINESS' &&
                              $scope.TaskInfo['ResultInfo'].length > 1 &&
                              isCompoundTask == false &&
                              errorNum == 0 )
                     {
                        $timeout( function(){
                           $scope.Components.Confirm.type = 2 ;
                           $scope.Components.Confirm.context = $scope.autoLanguage( '是否创建 SQL 实例？' ) ;
                           $scope.Components.Confirm.isShow = true ;
                           $scope.Components.Confirm.okText = $scope.autoLanguage( '是' ) ;
                           $scope.Components.Confirm.ok = function(){
                              $scope.Components.Confirm.isShow = false ;
                              ShowInstallModule() ;
                           }
                        }, 10 ) ;
                     }
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