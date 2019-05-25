(function(){
   var sacApp = window.SdbSacManagerModule ;
   //控制器
   sacApp.controllerProvider.register( 'Monitor.SdbNode.Context.Ctrl', function( $scope, $compile, $location, SdbRest, SdbFunction ){
      
      //初始化
      $scope.ClusterName = SdbFunction.LocalData( 'SdbClusterName' ) ;
      $scope.ModuleType = SdbFunction.LocalData( 'SdbModuleType' ) ;
      $scope.ModuleName = SdbFunction.LocalData( 'SdbModuleName' ) ;
      var HostName = SdbFunction.LocalData( 'SdbHostName' ) ;
      var svcname = SdbFunction.LocalData( 'SdbServiceName' ) ;
      var isOpenSelectMenu = false ;
      
      $scope.SelectMenu = [] ;
      $scope.ContextList = [] ;
      $scope.ContextGridOptions = { 'titleWidth': [] } ;
      $scope.ShowKeyList = [ 'SessionID', 'ContextID', 'Description', 'DataRead' ] ;
      $scope.ShowKey = [] ;
      $scope.SelectMenu = [] ;
      $scope.OrderByField = [] ;
      $scope.ContextType = 'all' ;
      //节点信息，后期根据 跳转函数 获取节点名（主机名+端口号）
      

      var sql = 'SELECT * FROM $SNAPSHOT_CONTEXT WHERE HostName="' + HostName +'" AND svcname="'+ svcname + '"' ;

      var getContextList = function(){
         SdbRest.Exec( sql, {
            'success': function( ContextList ){
               $scope.ContextList = [] ;
               var keyList = [] ;
               var NewContextList = {} ;
               $.each( ContextList, function( index, value ){
                  if( typeof( value['Contexts'] ) != 'undefined' )
                  {
                     //解决当一个会话有多个上下文时，信息的错误
                     $.each( value['Contexts'], function( ContextsIndex, value2 ){
                        NewContextList['SessionID'] = value['SessionID'] ;
                        NewContextList['Status'] = value['Status'] ;
                        value2['SessionID'] = value['SessionID'] ;
                        value2['Status'] = value['Status'] ;
                        $.each( value2, function( key, value3 ){
                           NewContextList[key] = value3 ; 
                        } ) ;
                        $scope.ContextList.push( value2 ) ;
                        keyList = SdbFunction.getJsonKeys( NewContextList, 0, keyList ) ;
                     } ) ;
                  }
               } ) ;
               $scope.ShowKey = [] ;
               $scope.SelectMenu = [] ;
               $.each( keyList, function( index, key ){
                  $scope.ShowKey.push( { 'key': key, 'show': $scope.ShowKeyList.indexOf( key ) >= 0 } ) ;
                  $scope.SelectMenu.push( { 
                     'html': $compile( '<label><div class="Ellipsis" style="padding:5px 10px"><input type="checkbox" ng-model="ShowKey[\'' + index + '\'][\'show\']"/>&nbsp;' + key + '</div></label>' )( $scope ),
                     'onClick': function(){}
                  } ) ;
               } ) ;
               $scope.SelectMenu.push( { 
                  'html': $compile( '<button class="btn btn-primary" ng-click="SaveShowKeyList()" style="width:100%;">确定</button>' )( $scope )
               } ) ;
               gridShowColumn() ;
               $scope.Timer.complete = true ;
            },
            'failed': function( errorInfo ){
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  getContextList() ;
                  return true ;
               } ) ;
            }
         } ) ;
      } ;

      //创建 设置实时刷新 的弹窗
      $scope.CreatePlayIntervalModel = function(){
         $scope.Components.Modal.icon = '' ;
         $scope.Components.Modal.title = $scope.autoLanguage( '实时刷新设置' ) ;
         $scope.Components.Modal.isShow = true ;
         $scope.Components.Modal.form = {
            inputList: [
               {
                  "name": "play",
                  "webName": $scope.autoLanguage( '自动刷新' ),
                  "type": "select",
                  "value": $scope.Timer.status == 'start',
                  "valid": [
                     { 'key': '开启', 'value': true },
                     { 'key': '停止', 'value': false }
                  ]
               },
               {
                  "name": "interval",
                  "webName": $scope.autoLanguage( '刷新间距(秒)' ),
                  "type": "int",
                  "value": $scope.Timer.interval,
                  "valid": {
                     'min': 1
                  }
               }
            ]
         } ;
         $scope.Components.Modal.Context = '<div form-create para="data.form"></div>' ;
         $scope.Components.Modal.ok = function(){
            var isAllClear = $scope.Components.Modal.form.check() ;
            if( isAllClear )
            {
               var formVal = $scope.Components.Modal.form.getValue() ;
               $scope.Timer.interval = formVal['interval'] ;
               if( formVal['play'] == true )
               {
                  $scope.Timer.status = 'start' ;
               }
               else
               {
                  $scope.Timer.status = 'stop' ;
               }
            }
            return isAllClear ;
         }
      }
      
      //设置排序字段
      $scope.SetOrderField = function( fieldName ){
         var normal  = $scope.OrderByField.indexOf( fieldName ) ;
         var reverse = $scope.OrderByField.indexOf( '-' + fieldName ) ;
         if( normal == -1 && reverse == -1 )
         {
            $scope.OrderByField.push( fieldName ) ;
         }
         else if( normal >= 0 )
         {
            $scope.OrderByField[normal] = '-' + fieldName ;
         }
         else if( reverse >= 0 )
         {
            $scope.OrderByField.splice( reverse, 1 ) ;
         }
      }

      //创建 设置实时刷新 的弹窗
      $scope.CreatePlayIntervalModel = function(){
         $scope.Components.Modal.icon = '' ;
         $scope.Components.Modal.title = $scope.autoLanguage( '实时刷新设置' ) ;
         $scope.Components.Modal.isShow = true ;
         $scope.Components.Modal.form = {
            inputList: [
               {
                  "name": "play",
                  "webName": $scope.autoLanguage( '自动刷新' ),
                  "type": "select",
                  "value": $scope.Timer.status == 'start',
                  "valid": [
                     { 'key': '开启', 'value': true },
                     { 'key': '停止', 'value': false }
                  ]
               },
               {
                  "name": "interval",
                  "webName": $scope.autoLanguage( '刷新间距(秒)' ),
                  "type": "int",
                  "value": $scope.Timer.interval,
                  "valid": {
                     'min': 1
                  }
               }
            ]
         } ;
         $scope.Components.Modal.Context = '<div form-create para="data.form"></div>' ;
         $scope.Components.Modal.ok = function(){
            var isAllClear = $scope.Components.Modal.form.check() ;
            if( isAllClear )
            {
               var formVal = $scope.Components.Modal.form.getValue() ;
               $scope.Timer.interval = formVal['interval'] ;
               if( formVal['play'] == true )
               {
                  $scope.Timer.status = 'start' ;
               }
               else
               {
                  $scope.Timer.status = 'stop' ;
               }
            }
            return isAllClear ;
         }
      }
      
      //打开 网格显示列 的下拉菜单
      $scope.OpenSelecMenu = function(){
         if( $scope.Timer.status == 'start' )
         {
            isOpenSelectMenu = true ;
            $scope.Timer.status = 'stop' ;
         }
      }

      //渲染网格显示的列
      var gridShowColumn = function(){
         $scope.ContextGridOptions['titleWidth'] = [] ;
         $scope.ContextGridOptions['titleWidth'].push( '50px' ) ;
         var widthPercent = 100 / $scope.ShowKeyList.length ;
         $.each( $scope.ShowKeyList, function( index, keyName ){
            $scope.ContextGridOptions['titleWidth'].push( widthPercent ) ;
         } ) ;
         $scope.ContextGridOptions.onResize() ;
      }

      //保存显示列
      $scope.SaveShowKeyList = function(){
         if( isOpenSelectMenu == true && $scope.Timer.status == 'stop' )
         {
            isOpenSelectMenu = false ;
            $scope.Timer.status = 'start' ;
         }
         $scope.ShowKeyList = [] ;
         $.each( $scope.ShowKey, function( index, keyInfo ){
            if( keyInfo['show'] == true )
            {
               $scope.ShowKeyList.push( keyInfo['key'] ) ;
            }
         } ) ;
         gridShowColumn() ;
      }

      getContextList() ;

      $scope.showContext = function(){
         $scope.Components.Modal.contextInfo = {
            '会话ID' : 'Host-test-02:11810:10' ,
            '上下文ID': 6451 ,
            '对应系统线程ID': 854 ,
            '会话状态' : 'Running' ,
            'EDU类型' : 'Agent' ,
            '等待请求的队列长度' : 0 ,
            '已经处理请求的数量' : 150 ,
            '上下文ID数组' : 199 ,
            '数据记录读' : 0 ,
            '索引读' : 0 ,
            '数据记录写' : 0 ,
            '索引写' : 0 ,
            '总更新记录数量' : 0 ,
            '总删除记录数量' : 0 ,
            '总插入记录数量' : 0 ,
            '总读取记录数量' : 0 ,
            '总数据读' : 0 ,
            '总数据读时间' : 0 ,
            '总数据写时间' : 0 ,
            '读取记录的时间' : 0 ,
            '写入记录的时间' : 0 ,
            '连接发起时间' : "2016-04-07-19.19.42.932665",
            '最后一次操作类型' : 'COMMAND',
            'LastOpInfo' : 'Command:$SNAPSHOT_SESSION_CUR, Collection:, Match:{}, Selector:{}, OrderBy:{ \"SessionID\": 1 }, Hint:{}, Skip:0, Limit:-1, Flag:0x00000000(0)',
            "UserCPU" : 0.03 ,
            "SysCPU" : 0.02
         } ;
         $scope.Components.Modal.icon = '' ;
         $scope.Components.Modal.title = '上下文详细' ;
         $scope.Components.Modal.noOK = true ;
         $scope.Components.Modal.isShow = true ;
         $scope.Components.Modal.indexList = '' ;
         $scope.Components.Modal.Context = '\
<table class="table loosen border">\
<tr>\
<td style="width:40%;background-color:#F1F4F5;"><b>Key</b></td>\
<td style="width:60%;background-color:#F1F4F5;"><b>Value</b></td>\
</tr>\
<tr>\
<td>会话ID</td>\
<td>{{data.contextInfo["会话ID"]}}</td>\
</tr>\
<tr ng-repeat="(key, value) in data.contextInfo" ng-if="key != \'会话ID\'">\
<td>{{key}}</td>\
<td>{{value}}</td>\
</tr>\
</table>';
      }

      $scope.stopContext = function( ){
         $scope.Components.Confirm.isShow = true ;
         $scope.Components.Confirm.type = 1 ;
         $scope.Components.Confirm.okText = $scope.autoLanguage( '确定' ) ;
         $scope.Components.Confirm.closeText = $scope.autoLanguage( '取消' ) ;
         $scope.Components.Confirm.title = $scope.autoLanguage( '要中断该上下文吗？' ) ;
         $scope.Components.Confirm.context = '会话id : Host-test-02:11810:10' ;
         $scope.Components.Confirm.ok = function(){
         }
      }
      
      //选择当前上下文或者所有上下文
      $scope.ScreenMenu = [
         { 
            'html': $compile( '<label><div style="padding:5px 10px"><input type="radio" name="a" value="all" ng-model="screenResult[\'Role\']" />所有上下文</div></label>' )( $scope ),
            'onClick': function(){}
         },
         { 
            'html': $compile( '<label><div style="padding:5px 10px"><input type="radio" name="a" value="current" ng-model="screenResult[\'Role\']"/>当前上下文</div></label>' )( $scope ),
            'onClick': function(){}
         },
         { 
            'html': $compile( '<button class="btn btn-primary" ng-click="changeScreen()" style="width:100%;">确定</button>' )( $scope )
         }
      ] ;

      $scope.screenResult = {
         'Role':'all',
      } ;

      $scope.changeScreen = function(){
         if( isOpenSelectMenu == true && $scope.Timer.status == 'stop' )
         {
            isOpenSelectMenu = false ;
            $scope.Timer.status = 'start' ;
         }
         $scope.ContextType = $scope.screenResult['Role'] ;
         if( $scope.screenResult['Role'] == 'current' )
         {
            sql = 'SELECT * FROM $SNAPSHOT_CONTEXT_CUR WHERE HostName="' + HostName +'" AND svcname="'+ svcname + '"' ;
         }
         else
         {
            sql = 'SELECT * FROM $SNAPSHOT_CONTEXT WHERE HostName="' + HostName +'" AND svcname="'+ svcname + '"' ;
         }
         getContextList() ;
      } ;


      $scope.Timer = {
         status: 'stop',
         interval: 5,
         currentTimer: 0,
         complete: false,
         fn: getContextList
      } ;

      //跳转至部署
      $scope.GotoDeploy = function(){
         $location.path( '/Deploy/Index' ) ;
      } ;

      //跳转至监控主页
      $scope.GotoModule = function(){
         $location.path( '/Monitor/Index' ) ;
      } ;

      //跳转至分区组列表
      $scope.GotoGroups = function(){
         $location.path( '/Monitor/SDB-Nodes/Groups' ) ;
      } ;

      //跳转至资源
      $scope.GotoResource = function(){
         $location.path( '/Monitor/SDB-Resources/Domain' ) ;
      } ;

      //跳转至主机列表
      $scope.GotoHosts = function(){
         $location.path( '/Monitor/Host-List/Index' ) ;
      } ;
      
      
      //跳转至节点列表
      $scope.GotoNodes = function(){
         $location.path( '/Monitor/SDB-Nodes/Nodes' ) ;
      } ;
   } ) ;
}());