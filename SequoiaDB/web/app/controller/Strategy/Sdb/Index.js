//@ sourceURL=Index.js
(function(){
   var sacApp = window.SdbSacManagerModule ;
   //控制器
   sacApp.controllerProvider.register( 'Strategy.SDB.Task.Ctrl', function( $scope, $location, $compile, SdbFunction, SdbRest ){
      var clusterName = SdbFunction.LocalData( 'SdbClusterName' ) ;
      var moduleType = SdbFunction.LocalData( 'SdbModuleType' ) ;
      var moduleName = SdbFunction.LocalData( 'SdbModuleName' ) ;
      if( clusterName == null || moduleType != 'sequoiadb' || moduleName == null )
      {
         $location.path( '/Transfer' ).search( { 'r': new Date().getTime() } ) ;
         return;
      }

      //构造表格
      $scope.TaskTable = {
         'title': {
            'TaskID'      : 'TaskID',
            'Operation'   : '',
            'TaskName'    : 'TaskName',
            'Status'      : 'Status',
            'CreateUser'  : 'CreateUser',
            'CreateTime'  : 'CreateTime'
         },
         'body': [],
         'options': {
            'width': {
               'TaskID'     : '8%',
               'Operation'  : '50px',
               'TaskName'   : '23%',
               'Status'     : '23%', 
               'CreateUser' : '23%',
               'CreateTime' : '23%'
            },
            'sort': {
               'TaskID'     : true,
               'Operation'  : false,
               'TaskName'   : true,
               'Status'     : true, 
               'CreateUser' : true,
               'CreateTime' : true
            },
            'autoSort': { 'key': 'TaskID', 'asc': true },
            'max': 50
         }
      } ;

      //时间戳转日期
      var timestampToTime = function( timestamp ) {
         var date = new Date( timestamp * 1000 ) ;
         Y = date.getFullYear() + '-' ;
         M = ( date.getMonth() + 1 < 10 ? '0' + ( date.getMonth() + 1 ) : date.getMonth() + 1 ) + '-' ;
         D = ( date.getDate() < 10 ? '0' + date.getDate() : date.getDate() ) + ' ' ;
         h = ( date.getHours() < 10 ? '0' + date.getHours() : date.getHours() ) + ':' ;
         m = ( date.getMinutes() < 10 ? '0' + date.getMinutes() : date.getMinutes() ) + ':' ;
         s = ( date.getSeconds() < 10 ? '0' + date.getSeconds() : date.getSeconds() ) ;
         return Y+M+D+h+m+s;
      }

      //查询task列表
      $scope.QueryTasksList = function(){
         var data = {
            'cmd': 'list svc task',
            'ClusterName': clusterName,
            'BusinessName': moduleName
         } ;
         SdbRest.OmOperation( data, {
            'success': function( info ){
               $.each( info, function( index, taskInfo ){
                  taskInfo['CreateTime'] = timestampToTime( taskInfo['CreateTime'] ) ;
               } ) ;
               $scope.TaskTable['body'] = info ;
            },
            'failed': function( errorInfo ){
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  $scope.QueryTasksList() ;
                  return true ;
               } ) ;
            }
         } ) ;
      }

      //添加task
      var addTask = function( taskName ){
         var data = {
            'cmd': 'add svc task',
            'ClusterName': clusterName,
            'BusinessName': moduleName,
            'TaskName': taskName
         } ;
         SdbRest.OmOperation( data, {
            'success': function( info ){
               $scope.Components.Confirm.type = 4 ;
               $scope.Components.Confirm.context = sprintf( $scope.autoLanguage( '添加成功！' ) ) ;
               $scope.Components.Confirm.isShow = true ;
               $scope.Components.Confirm.noClose = true ;
               $scope.Components.Confirm.normalOK = true ;
               $scope.Components.Confirm.okText = $scope.autoLanguage( '确定' ) ;
               $scope.Components.Confirm.ok = function(){
                  $scope.Components.Confirm.normalOK = false ;
                  $scope.Components.Confirm.isShow = false ;
                  $scope.Components.Confirm.noClose = false ;
               }
               $scope.QueryTasksList() ;
            },
            'failed': function( errorInfo ){
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  addTask( taskName ) ;
                  return true ;
               } ) ;
            }
         } ) ;
      }

      //修改task
      var updateTask = function( taskName, status ){
         var data = {
            'cmd': 'update svc task status',
            'ClusterName': clusterName,
            'BusinessName': moduleName,
            'Status': status,
            'TaskName': taskName
         } ;
         SdbRest.OmOperation( data, {
            'success': function( info ){
               $scope.Components.Confirm.type = 4 ;
               $scope.Components.Confirm.context = $scope.autoLanguage( '修改任务状态成功！' ) ;
               $scope.Components.Confirm.isShow = true ;
               $scope.Components.Confirm.noClose = true ;
               $scope.Components.Confirm.normalOK = true ;
               $scope.Components.Confirm.okText = $scope.autoLanguage( '确定' ) ;
               $scope.Components.Confirm.ok = function(){
                  $scope.Components.Confirm.normalOK = false ;
                  $scope.Components.Confirm.isShow = false ;
                  $scope.Components.Confirm.noClose = false ;
               }
               $scope.QueryTasksList() ;
            },
            'failed': function( errorInfo ){
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  updateTask( taskName, status ) ;
                  return true ;
               } ) ;
            }
         } ) ;
      }

      //删除task
      var deleteTask = function( taskName ){
         var data = {
            'cmd': 'del svc task',
            'ClusterName': clusterName,
            'BusinessName': moduleName,
            'TaskName': taskName
         } ;
         SdbRest.OmOperation( data, {
            'success': function( info ){
               $scope.Components.Confirm.type = 4 ;
               $scope.Components.Confirm.context = $scope.autoLanguage( '删除成功！' ) ;
               $scope.Components.Confirm.isShow = true ;
               $scope.Components.Confirm.noClose = true ;
               $scope.Components.Confirm.normalOK = true ;
               $scope.Components.Confirm.okText = $scope.autoLanguage( '确定' ) ;
               $scope.Components.Confirm.ok = function(){
                  $scope.Components.Confirm.normalOK = false ;
                  $scope.Components.Confirm.isShow = false ;
                  $scope.Components.Confirm.noClose = false ;
               }
               $scope.QueryTasksList() ;
            },
            'failed': function( errorInfo ){
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  deleteTask( taskName ) ;
                  return true ;
               } ) ;
            }
         } ) ;
      }

      //查询task列表
      $scope.QueryTasksList() ;


      //添加task 弹窗
      $scope.CreateTaskWindow = {
         'config': {
            inputList: []
         },
         'callback': {}
      } ;

      //打开 添加task 弹窗
      $scope.ShowCreateTask = function(){
         $scope.CreateTaskWindow['config']['inputList'] = [
            {
               "name": 'clusterName',
               "webName": $scope.autoLanguage( '集群名' ),
               "type": "string",
               "required": true,
               "value": clusterName,
               "disabled": true,
               "valid": {
                  "min": 1
               }
            },
            {
               "name": 'moduleName',
               "webName": $scope.autoLanguage( '存储集群名' ),
               "type": "string",
               "required": true,
               "value": moduleName,
               "disabled": true,
               "valid": {
                  "min": 1
               }
            },
            {
               "name": 'taskName',
               "webName": $scope.autoLanguage( '任务名' ),
               "type": "string",
               "required": true,
               "value": '',
               "valid": {
                  "min": 1
               }
            }
         ] ;
         $scope.CreateTaskWindow['callback']['SetOkButton']( $scope.autoLanguage( '确定' ), function(){
            var isClear = $scope.CreateTaskWindow['config'].check() ;
            if( isClear )
            {
               var formValue = $scope.CreateTaskWindow['config'].getValue() ;
               var taskName = formValue['taskName'] ;
               addTask( taskName ) ;
               $scope.CreateTaskWindow['callback']['Close']() ;

            }
         } ) ;
         $scope.CreateTaskWindow['callback']['SetTitle']( $scope.autoLanguage( '添加任务' ) ) ;
         $scope.CreateTaskWindow['callback']['SetIcon']( '' ) ;
         $scope.CreateTaskWindow['callback']['Open']() ;
      }

      //修改task 弹窗
      $scope.UpdateTaskWindow = {
         'config': {},
         'callback': {}
      } ;

      //打开 修改task 弹窗
      $scope.ShowUpdateTask = function( taskName, status ){
         $scope.UpdateTaskWindow['config']['inputList'] = [
            {
               "name": 'clusterName',
               "webName": $scope.autoLanguage( '集群名' ),
               "type": "string",
               "required": true,
               "value": clusterName,
               "disabled": true,
               "valid": {
                  "min": 1
               }
            },
            {
               "name": 'moduleName',
               "webName": $scope.autoLanguage( '存储集群名' ),
               "type": "string",
               "required": true,
               "value": moduleName,
               "disabled": true,
               "valid": {
                  "min": 1
               }
            },
            {
               "name": 'taskName',
               "webName": $scope.autoLanguage( '任务名' ),
               "type": "string",
               "required": true,
               "value": taskName,
               "disabled": true,
               "valid": {
                  "min": 1
               }
            },
            {
               "name": 'status',
               "webName": 'Status',
               "type": "switch",
               "value": status == 1 ? true : false,
               "desc": $scope.autoLanguage( '策略配置状态' ),
               "onChange": function( name, key ){
                  $scope.UpdateTaskWindow['config']['inputList'][3]['value'] = !key ;
               }
            }
         ] ;
         $scope.UpdateTaskWindow['callback']['SetOkButton']( $scope.autoLanguage( '确定' ), function(){
            var isClear = $scope.UpdateTaskWindow['config'].check() ;
            if( isClear )
            {
               var formValue = $scope.UpdateTaskWindow['config'].getValue() ;
               var taskName = formValue['taskName'] ;
               var taskStatus = formValue['status'] == true ? 1 : 0 ;
               updateTask( taskName, taskStatus ) ;
               $scope.UpdateTaskWindow['callback']['Close']() ;
            }
         } ) ;

         $scope.UpdateTaskWindow['callback']['SetTitle']( $scope.autoLanguage( '修改任务' ) ) ;
         $scope.UpdateTaskWindow['callback']['SetIcon']( '' ) ;
         $scope.UpdateTaskWindow['callback']['Open']() ;
      }

      //打开 删除Task 弹窗
      $scope.ShowDeleteTask = function( taskName ){
         $scope.Components.Confirm.type = 3 ;
         $scope.Components.Confirm.context = sprintf( $scope.autoLanguage( '是否确定删除任务：?？' ), taskName ) ;
         $scope.Components.Confirm.isShow = true ;
         $scope.Components.Confirm.okText = $scope.autoLanguage( '确定' ) ;
         $scope.Components.Confirm.ok = function(){
            deleteTask( taskName ) ;
            $scope.Components.Confirm.isShow = false ;
         }
      }

      $scope.GotoStrategy = function(){
         $location.path( '/Strategy/SDB/Strategy' ).search( { 'r': new Date().getTime() } ) ;
      }
   } ) ;
}());