//@ sourceURL=Index.js
(function(){
   var sacApp = window.SdbSacManagerModule ;
   //控制器
   sacApp.controllerProvider.register( 'Strategy.SDB.Strategy.Ctrl', function( $scope, $location, $compile, SdbFunction, SdbRest ){
      var clusterName = SdbFunction.LocalData( 'SdbClusterName' ) ;
      var moduleType = SdbFunction.LocalData( 'SdbModuleType' ) ;
      var moduleName = SdbFunction.LocalData( 'SdbModuleName' ) ;
      if( clusterName == null || moduleType != 'sequoiadb' || moduleName == null )
      {
         $location.path( '/Transfer' ).search( { 'r': new Date().getTime() } ) ;
         return;
      }

      //'RuleID'   : $scope.autoLanguage( '规则ID' ),
      //'TaskID'   : $scope.autoLanguage( '任务ID' ),
      //'SortID'   : $scope.autoLanguage( '任务ID' ),
      //'Nice'     : $scope.autoLanguage( '任务名' ),
      //'Status'   : $scope.autoLanguage( '状态' ),
      //'UserName' : $scope.autoLanguage( '创建用户' ),
      //'IPs'      : $scope.autoLanguage( '创建时间' ),
      
      var taskList = [] ;

      //构造表格
      $scope.StrategyTable = {
         'title': {
            'RuleID'    : 'RuleID',
            'Operation' : '',
            'TaskID'    : 'TaskID',
            'TaskName'  : 'TaskName',
            'SortID'    : 'SortID',
            'Nice'      : 'Nice',
            'Status'    : 'Status',
            'UserName'  : 'UserName',
            'IPs'       : 'IPs'
         },
         'body': [],
         'options': {
            'width': {
               'RuleID'    : '10%',
               'Operation' : '50px',
               'TaskID'    : '10%',
               'TaskName'  : '10%',
               'SortID'    : '10%',
               'Nice'      : '10%',
               'Status'    : '10%',
               'UserName'  : '20%',
               'IPs'       : '20%'
            },
            'sort': {
               'RuleID'    : true,
               'Operation' : false,
               'TaskID'    : true,
               'TaskName'  : true,
               'SortID'    : true, 
               'Nice'      : true,
               'Status'    : true,
               'UserName'  : true,
               'IPs'       : true
            },
            'max': 50,
            'filter': {
               'RuleID'    : 'number',
               'TaskID'    : 'number',
               'TaskName'  : 'indexof',
               'SortID'    : 'number'
            }
         }
      } ;

      //查询task列表
      var queryTasksList = function(){
         var data = {
            'cmd': 'list svc task',
            'ClusterName': clusterName,
            'BusinessName': moduleName
         } ;
         SdbRest.OmOperation( data, {
            'success': function( tasks){
               $.each( tasks, function( index, taskInfo ){
                  taskList.push( { 'key': taskInfo['TaskName'], 'value': taskInfo['TaskName'] } ) ;
               } ) ;
            },
            'failed': function( errorInfo ){
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  queryTasksList() ;
                  return true ;
               } ) ;
            }
         } ) ;
      }

      //查询strategy列表
      $scope.QueryStrategyList = function(){
         var data = {
            'cmd': 'list svc task strategy',
            'ClusterName': clusterName,
            'BusinessName': moduleName
         } ;
         SdbRest.OmOperation( data, {
            'success': function( info ){
               $.each( info, function( index, strategyInfo ){
                  strategyInfo['IPs'] = strategyInfo['IPs'].toString() ;
               } ) ;
               $scope.StrategyTable['body'] = info ;
            },
            'failed': function( errorInfo ){
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  $scope.QueryStrategyList() ;
                  return true ;
               } ) ;
            }
         } ) ;
      }

      //添加策略
      var addStrategy = function( data ){
         SdbRest.OmOperation( data, {
            'success': function( info ){
               $scope.Components.Confirm.type = 4 ;
               $scope.Components.Confirm.context = $scope.autoLanguage( '添加策略配置成功！' ) ;
               $scope.Components.Confirm.isShow = true ;
               $scope.Components.Confirm.okText = $scope.autoLanguage( '确定' ) ;
               $scope.Components.Confirm.ok = function(){
                  $scope.Components.Confirm.isShow = false ;
               }
               $scope.QueryStrategyList() ;
            },
            'failed': function( errorInfo ){
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  addStrategy( data ) ;
                  return true ;
               } ) ;
            }
         } ) ;
      }

      //删除Strategy
      var deleteStrategy = function( ruleID ){
         var data = {
            'cmd': 'del svc task strategy',
            'ClusterName': clusterName,
            'BusinessName': moduleName,
            'RuleID': ruleID
         } ;
         SdbRest.OmOperation( data, {
            'success': function( info ){
               $scope.QueryStrategyList() ;
            },
            'failed': function( errorInfo ){
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  deleteStrategy( ruleID ) ;
                  return true ;
               } ) ;
            }
         } ) ;
      }

      //立即生效策略配置
      var flushStrategy = function(){
         var data = {
            'cmd': 'flush svc task strategy',
            'ClusterName': clusterName,
            'BusinessName': moduleName
         } ;
         SdbRest.OmOperation( data, {
            'success': function( info ){
               $scope.Components.Confirm.type = 4 ;
               $scope.Components.Confirm.context = $scope.autoLanguage( '立即生效策略配置成功！' ) ;
               $scope.Components.Confirm.isShow = true ;
               $scope.Components.Confirm.okText = $scope.autoLanguage( '确定' ) ;
               $scope.Components.Confirm.ok = function(){
                  $scope.Components.Confirm.isShow = false ;
               }
               $scope.QueryStrategyList() ;
            },
            'failed': function( errorInfo ){
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  flushStrategy() ;
                  return true ;
               } ) ;
            }
         } ) ;
      }

      //修改Nice值
      var updateNice = function( ruleID, nice ){
         var data = {
            'cmd': 'update svc task strategy nice',
            'ClusterName': clusterName,
            'BusinessName': moduleName,
            'RuleID': ruleID,
            'Nice': nice
         } ;
         SdbRest.OmOperation( data, {
            'success': function( info ){
               $scope.Components.Confirm.type = 4 ;
               $scope.Components.Confirm.context = $scope.autoLanguage( '修改策略配置成功！' ) ;
               $scope.Components.Confirm.isShow = true ;
               $scope.Components.Confirm.okText = $scope.autoLanguage( '确定' ) ;
               $scope.Components.Confirm.ok = function(){
                  $scope.Components.Confirm.isShow = false ;
               }
               $scope.QueryStrategyList() ;
            },
            'failed': function( errorInfo ){
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  updateNice( ruleID, nice ) ;
                  return true ;
               } ) ;
            }
         } ) ;
      }

      //修改生效顺序
      var updateSort = function( ruleID, sortID ){
         var data = {
            'cmd': 'update svc task strategy sort',
            'ClusterName': clusterName,
            'BusinessName': moduleName,
            'RuleID': ruleID,
            'SortID': sortID
         } ;
         SdbRest.OmOperation( data, {
            'success': function( info ){
               $scope.Components.Confirm.type = 4 ;
               $scope.Components.Confirm.context = $scope.autoLanguage( '修改策略配置成功！' ) ;
               $scope.Components.Confirm.isShow = true ;
               $scope.Components.Confirm.okText = $scope.autoLanguage( '确定' ) ;
               $scope.Components.Confirm.ok = function(){
                  $scope.Components.Confirm.isShow = false ;
               }
               $scope.QueryStrategyList() ;
            },
            'failed': function( errorInfo ){
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  updateSort( ruleID, sortID ) ;
                  return true ;
               } ) ;
            }
         } ) ;
      }
 
      //修改策略配置状态
      var updateStatus = function( ruleID, status ){
         var data = {
            'cmd': 'update svc task strategy status',
            'ClusterName': clusterName,
            'BusinessName': moduleName,
            'RuleID': ruleID,
            'status': status
         } ;
         SdbRest.OmOperation( data, {
            'success': function( info ){
               $scope.Components.Confirm.type = 4 ;
               $scope.Components.Confirm.context = $scope.autoLanguage( '修改策略配置成功！' ) ;
               $scope.Components.Confirm.isShow = true ;
               $scope.Components.Confirm.okText = $scope.autoLanguage( '确定' ) ;
               $scope.Components.Confirm.ok = function(){
                  $scope.Components.Confirm.isShow = false ;
               }
               $scope.QueryStrategyList() ;
            },
            'failed': function( errorInfo ){
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  updateStatus( ruleID, status ) ;
                  return true ;
               } ) ;
            }
         } ) ;
      }

      //删除策略IPs
      var deleteIPs = function( ruleID, IPs, newIPs ){
         var data = {
            'cmd': 'del svc task strategy IPs',
            'ClusterName': clusterName,
            'BusinessName': moduleName,
            'RuleID': ruleID,
            'IPs': IPs
         } ;
         SdbRest.OmOperation( data, {
            'success': function( info ){
               addIPs( ruleID, newIPs ) ;
            },
            'failed': function( errorInfo ){
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  deleteIPs( ruleID, IPs, newIPs ) ;
                  return true ;
               } ) ;
            }
         } ) ;
      }
      
      //新增IPs
      var addIPs = function( ruleID, IPs ){
         var data = {
            'cmd': 'add svc task strategy IPs',
            'ClusterName': clusterName,
            'BusinessName': moduleName,
            'RuleID': ruleID,
            'IPs': IPs
         } ;
         SdbRest.OmOperation( data, {
            'success': function( info ){
               $scope.Components.Confirm.type = 4 ;
               $scope.Components.Confirm.context = $scope.autoLanguage( '修改策略配置成功！' ) ;
               $scope.Components.Confirm.isShow = true ;
               $scope.Components.Confirm.okText = $scope.autoLanguage( '确定' ) ;
               $scope.Components.Confirm.ok = function(){
                  $scope.Components.Confirm.isShow = false ;
               }
               $scope.QueryStrategyList() ;
            },
            'failed': function( errorInfo ){
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  addIPs( ruleID, IPs ) ;
                  return true ;
               } ) ;
            }
         } ) ;
      }

      //查询Strategy列表
      $scope.QueryStrategyList() ;

      //查询task
      queryTasksList() ;

      //添加Strategy 弹窗
      $scope.CreateStrategyWindow = {
         'config': {
            inputList: []
         },
         'callback': {}
      } ;

      //打开 添加Strategy 弹窗
      $scope.ShowCreateStrategy = function(){
         $scope.CreateStrategyWindow['config']['inputList'] = [
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
               "name": 'businessName',
               "webName": $scope.autoLanguage( '业务名' ),
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
               "webName": 'TaskName',
               "type": "select",
               "required": true,
               "desc": $scope.autoLanguage( '对应的任务名' ),
               "value": taskList[0]['value'],
               "valid": taskList
            },
            {
               "name": 'nice',
               "webName": 'Nice',
               "type": "int",
               "required": true,
               "desc": $scope.autoLanguage( '任务优先级，取值范围：-20~19，值越大优先级越低' ),
               "value": 0,
               "valid": {
                  "min": -20,
                  "max": 19
               }
            },
            {
               "name": 'sortID',
               "webName": 'SortID',
               "type": "string",
               "desc": $scope.autoLanguage( '控制策略配置的生效顺序' ),
               "value": ''
            },
            {
               "name": 'userName',
               "webName": 'UserName',
               "type": "string",
               "value": ''
            },
            {
               "name": 'IPs',
               "webName": 'IPs',
               "type": "string",
               "desc": $scope.autoLanguage( '直连数据库应用程序所在客户端的IP地址列表，多个地址使用","隔开' ),
               "value": ''
            }
         ] ;
         $scope.CreateStrategyWindow['callback']['SetOkButton']( $scope.autoLanguage( '确定' ), function(){
            var isClear = $scope.CreateStrategyWindow['config'].check() ;
            if( isClear )
            {
               var formValue = $scope.CreateStrategyWindow['config'].getValue() ;
               var data = { 'cmd': 'add svc task strategy' } ;
               data['ClusterName'] = formValue['clusterName'] ;
               data['BusinessName'] = formValue['businessName'] ;
               data['TaskName'] = formValue['taskName'] ;
               data['Nice'] = formValue['nice'] ;
               data['SortID'] = formValue['sortID'] ;
               data['UserName'] = formValue['userName'] ;
               data['IPs'] = formValue['IPs'] ;

               addStrategy( data ) ;
               $scope.CreateStrategyWindow['callback']['Close']() ;

            }
         } ) ;
         $scope.CreateStrategyWindow['callback']['SetTitle']( $scope.autoLanguage( '添加策略' ) ) ;
         $scope.CreateStrategyWindow['callback']['SetIcon']( '' ) ;
         $scope.CreateStrategyWindow['callback']['Open']() ;
      }

      //立即生效策略
      $scope.ShowFlushStrategy = function(){
         $scope.Components.Confirm.type = 1 ;
         $scope.Components.Confirm.context = $scope.autoLanguage( '是否确定立即生效策略配置？' ) ;
         $scope.Components.Confirm.isShow = true ;
         $scope.Components.Confirm.okText = $scope.autoLanguage( '确定' ) ;
         $scope.Components.Confirm.ok = function(){
            flushStrategy() ;
            $scope.Components.Confirm.isShow = false ;
         }
      }

      //修改status 弹窗
      $scope.UpdateSatusWindow = {
         'config': {},
         'callback': {}
      } ;

      //打开 修改status 弹窗
      var showUpdateStatus = function( strategyInfo ){
         $scope.UpdateSatusWindow['config']['inputList'] = [
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
               "name": 'businessName',
               "webName": $scope.autoLanguage( '业务名' ),
               "type": "string",
               "required": true,
               "value": moduleName,
               "disabled": true,
               "valid": {
                  "min": 1
               }
            },
            {
               "name": 'ruleID',
               "webName": 'RuleID',
               "type": "string",
               "required": true,
               "value": strategyInfo['RuleID'],
               "disabled": true
            },
            {
               "name": 'status',
               "webName": $scope.autoLanguage( '状态' ),
               "type": "select",
               "required": true,
               "value": strategyInfo['Status'],
               "valid": [
                  { 'value': 1, 'key': 1 },
                  { 'value': 0, 'key': 0 }
               ]
            }
         ] ;
         
         $scope.UpdateSatusWindow['callback']['SetOkButton']( $scope.autoLanguage( '确定' ), function(){
            var isClear = $scope.UpdateSatusWindow['config'].check() ;
            if( isClear )
            {
               var formValue = $scope.UpdateSatusWindow['config'].getValue() ;
               updateStatus( strategyInfo['RuleID'], formValue['status'] ) ;
               $scope.UpdateSatusWindow['callback']['Close']() ;
            }
         } ) ;
         $scope.UpdateSatusWindow['callback']['SetTitle']( $scope.autoLanguage( '修改策略状态' ) ) ;
         $scope.UpdateSatusWindow['callback']['SetIcon']( '' ) ;
         $scope.UpdateSatusWindow['callback']['Open']() ;
      }

      //修改策略 弹窗
      $scope.UpdateStrategyWindow = {
         'config': {},
         'callback': {}
      } ;

      //打开 修改策略 弹窗
      $scope.ShowUpdateStrategy = function( strategyInfo ){
         $scope.UpdateStrategyWindow['config']['inputList'] = [
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
               "name": 'businessName',
               "webName": $scope.autoLanguage( '业务名' ),
               "type": "string",
               "required": true,
               "value": moduleName,
               "disabled": true,
               "valid": {
                  "min": 1
               }
            },
            {
               "name": 'ruleID',
               "webName": 'RuleID',
               "type": "string",
               "required": true,
               "value": strategyInfo['RuleID'],
               "disabled": true
            },
            {
               "name": 'nice',
               "webName": 'Nice',
               "type": "int",
               "required": true,
               "desc": $scope.autoLanguage( '任务优先级，取值范围：-20~19，值越大优先级越低' ),
               "value": strategyInfo['Nice'],
               "valid": {
                  "min": -20,
                  "max": 19
               }
            },
            {
               "name": 'sortID',
               "webName": 'SortID',
               "type": "string",
               "desc": $scope.autoLanguage( '控制策略配置的生效顺序' ),
               "value": strategyInfo['SortID']
            },
            {
               "name": 'userName',
               "webName": 'UserName',
               "type": "string",
               "value": strategyInfo['UserName'],
               "disabled": true
            },
            {
               "name": 'IPs',
               "webName": 'IPs',
               "type": "string",
               "desc": $scope.autoLanguage( '直连数据库应用程序所在客户端的IP地址列表，多个地址使用","隔开' ),
               "value": strategyInfo['IPs'].toString()
            },
            {
               "name": 'status',
               "webName": 'Status',
               "type": "switch",
               "value": strategyInfo['Status'] == 1 ? true : false,
               "desc": $scope.autoLanguage( '策略配置状态' ),
               "onChange": function( name, key ){
                  $scope.UpdateStrategyWindow['config']['inputList'][7]['value'] = !key ;
               }
            }
         ] ;
         
         $scope.UpdateStrategyWindow['callback']['SetOkButton']( $scope.autoLanguage( '确定' ), function(){
            var isClear = $scope.UpdateStrategyWindow['config'].check() ;
            if( isClear )
            {
               var formValue = $scope.UpdateStrategyWindow['config'].getValue() ;
               if( strategyInfo['Nice'] != formValue['nice'] )
               {
                  updateNice( strategyInfo['RuleID'], formValue['nice'] ) ;
               }
               if( strategyInfo['SortID'] != formValue['sortID'] )
               {
                  updateSort( strategyInfo['RuleID'], formValue['sortID'] ) ;
               }
               if( strategyInfo['IPs'] != formValue['IPs'] )
               {
                  deleteIPs( strategyInfo['RuleID'], strategyInfo['IPs'], formValue['IPs'] ) ;
               }
               if( strategyInfo['Status'] != formValue['status'] )
               {
                  updateStatus( strategyInfo['RuleID'], ( formValue['status'] == true ? 1 : 0 ) ) ;
               }

               $scope.UpdateStrategyWindow['callback']['Close']() ;
            }
         } ) ;
         $scope.UpdateStrategyWindow['callback']['SetTitle']( $scope.autoLanguage( '修改策略配置' ) ) ;
         $scope.UpdateStrategyWindow['callback']['SetIcon']( '' ) ;
         $scope.UpdateStrategyWindow['callback']['Open']() ;
      }

      //打开 删除Strategy 弹窗
      $scope.ShowDeleteStrategy = function( ruleID ){
         $scope.Components.Confirm.type = 3 ;
         $scope.Components.Confirm.context = sprintf( $scope.autoLanguage( '是否确定删除策略：RuleID: ?？' ), ruleID ) ;
         $scope.Components.Confirm.isShow = true ;
         $scope.Components.Confirm.okText = $scope.autoLanguage( '确定' ) ;
         $scope.Components.Confirm.ok = function(){
            deleteStrategy( ruleID ) ;
            $scope.Components.Confirm.isShow = false ;
         }
      }

      $scope.GotoTask = function(){
         $location.path( '/Strategy/SDB/Index' ).search( { 'r': new Date().getTime() } ) ;
      }
   } ) ;
}());