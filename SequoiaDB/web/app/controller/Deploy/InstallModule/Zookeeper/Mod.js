//@ sourceURL=Mod.js
(function(){
   var sacApp = window.SdbSacManagerModule ;
   //控制器
   sacApp.controllerProvider.register( 'Deploy.Zkp.Mod.Ctrl', function( $scope, $compile, $location, $rootScope, SdbRest ){

      //初始化
      $scope.NodeListOptions = {
         'titleWidth': [ '26px', 23, 18, 31, 10, 18 ]
      } ;
      $scope.NodeList       = [] ;
      $scope.Template = {
         'keyWidth': '160px',
         'inputList': []
      } ;
      $scope.installConfig  = {} ;
      $scope.CurrentNode    = 0 ;

      $scope.Configure   = $rootScope.tempData( 'Deploy', 'ModuleConfig' ) ;
      var deployType     = $rootScope.tempData( 'Deploy', 'Model' ) ;
      var clusterName    = $rootScope.tempData( 'Deploy', 'ClusterName' ) ;
      $scope.ModuleName  = $rootScope.tempData( 'Deploy', 'ModuleName' ) ;
      /*
      deployType = "Install_Module" ;
      clusterName = "myCluster1" ;
      $scope.ModuleName = 'myModule1' ;
      $scope.Configure = {
         "ClusterName": "myCluster1",
         "BusinessName": "myModule1",
         "DeployMod": "distribution",
         "BusinessType": "zookeeper",
         "Property": [
            { "Name": "zoonodenum", "Value":"3" }
         ],
         "HostInfo": [
            { "HostName": "ubuntu-test-03" },
            { "HostName": "ubuntu-test-05" }
         ]
      } ; 
      */
      if( deployType == null || clusterName == null || $scope.ModuleName == null || $scope.Configure == null )
      {
         $location.path( '/Deploy/Index' ).search( { 'r': new Date().getTime() } ) ;
         return ;
      }

      $scope.stepList = _Deploy.BuildSdbStep( $scope, $location, deployType, $scope['Url']['Action'], 'zookeeper' ) ;
      if( $scope.stepList['info'].length == 0 )
      {
         $location.path( '/Deploy/Index' ).search( { 'r': new Date().getTime() } ) ;
         return ;
      }

      //删除节点
      $scope.RemoveNode = function( nodeIndex ){
         $scope.NodeList.splice( nodeIndex, 1 ) ;
      }

      //创建 添加节点 弹窗
      $scope.CreateAddNodeModel = function(){
         $scope.Components.Modal.icon = '' ;
         $scope.Components.Modal.title = $scope.autoLanguage( '添加节点' ) ;
         $scope.Components.Modal.isShow = true ;
         $scope.Components.Modal.form1 = {
            inputList: [
               {
                  "name": "createModel",
                  "webName": $scope.autoLanguage( '创建节点模式' ),
                  "type": "select",
                  "value": 0,
                  "valid": [
                     { 'key': $scope.autoLanguage( '默认配置' ), 'value': 0 },
                     { 'key': $scope.autoLanguage( '复制节点配置' ), 'value': 1 }
                  ],
                  'onChange': function( name, key, value ){
                     if( value == 1 )
                     {
                        $scope.Components.Modal.ShowType = 2 ;
                        $scope.Components.Modal.form2['inputList'][0]['value'] = $scope.Components.Modal.form1['inputList'][0]['value'] ;
                        $scope.Components.Modal.form2['inputList'][1]['value'] = $scope.Components.Modal.form1['inputList'][1]['value'] ;
                     }
                  }
               },
               {
                  "name": "nodeid",
                  "webName": $scope.autoLanguage( '节点Id' ),
                  "required": true,
                  "type": "int",
                  "value": '',
                  "valid": {
                     'min': 1,
                     'max': 10000
                  }
               }
            ]
         } ;
         $scope.Components.Modal.form2 = {
            inputList: [
               {
                  "name": "createModel",
                  "webName": $scope.autoLanguage( '创建节点模式' ),
                  "type": "select",
                  "value": 0,
                  "valid": [
                     { 'key': $scope.autoLanguage( '默认配置' ), 'value': 0 },
                     { 'key': $scope.autoLanguage( '复制节点配置' ), 'value': 1 }
                  ],
                  'onChange': function( name, key, value ){
                     if( value != 1 )
                     {
                        $scope.Components.Modal.ShowType = 1 ;
                        $scope.Components.Modal.form1['inputList'][0]['value'] = $scope.Components.Modal.form2['inputList'][0]['value'] ;
                        $scope.Components.Modal.form1['inputList'][1]['value'] = $scope.Components.Modal.form2['inputList'][1]['value'] ;
                     }
                  }
               },
               {
                  "name": "nodeid",
                  "webName": $scope.autoLanguage( '节点Id' ),
                  "required": true,
                  "type": "int",
                  "value": '',
                  "valid": {
                     'min': 1,
                     'max': 10000
                  }
               },
               {
                  "name": "copyNode",
                  "webName": $scope.autoLanguage( '复制的节点' ),
                  "type": "select",
                  "value": 0,
                  "valid": []
               }
            ]
         } ;
         $.each( $scope.NodeList, function( index2, nodeInfo ){
            $scope.Components.Modal.form2['inputList'][2]['valid'].push( { 'key': nodeInfo['zooid'], 'value': index2 } ) ;
         } ) ;
         $scope.Components.Modal.ShowType = 1 ;
         $scope.Components.Modal.Context = '<div ng-show="data.ShowType == 1" form-create para="data.form1"></div><div ng-show="data.ShowType == 2" form-create para="data.form2"></div>' ;
         $scope.Components.Modal.ok = function(){
            var form ;
            if( $scope.Components.Modal.ShowType == 1 )
            {
               form = $scope.Components.Modal.form1 ;
            }
            else
            {
               form = $scope.Components.Modal.form2 ;
            }
            var isAllClear = form.check( function( formVal ){
               var error = [] ;
               $.each( $scope.NodeList, function( index, nodeInfo ){
                  if( nodeInfo['zooid'] == formVal['nodeid'] )
                  {
                     error.push( { 'name': 'nodeid', 'error': $scope.autoLanguage( '节点Id已经存在' ) } ) ;
                     return false ;
                  }
               } ) ;
               return error ;
            } ) ;
            if( isAllClear )
            {
               var formVal = form.getValue() ;
               if( formVal['createModel'] == 0 )
               {
                  var newNode = {} ;
                  $.each( $scope.installConfig['Property'], function( index, paraInfo ){
                     newNode[ paraInfo['name'] ] = paraInfo['value'] ;
                  } ) ;
                  newNode['zooid'] = formVal['nodeid'] ;
                  newNode['HostName'] = $scope.Configure['HostInfo'][0]['HostName'] ;
                  $scope.NodeList.push( newNode ) ;
               }
               else
               {
                  var newNode = $.extend( true, {}, $scope.NodeList[ formVal['copyNode'] ] ) ;
                  newNode['zooid'] = formVal['nodeid'] ;
                  $scope.NodeList.push( newNode ) ;
               }
               $scope.SwitchNode( $scope.NodeList.length - 1 ) ;
            }
            return isAllClear ;
         }
      }

      //切换节点
      $scope.SwitchNode = function( nodeIndex ){
         $.each( $scope.Template['inputList'], function( index ){
            var name = $scope.Template['inputList'][index]['name'] ;
            $scope.Template['inputList'][index]['value'] = $scope.NodeList[nodeIndex][name] ;
         } ) ;
         $scope.Template['inputList'][1]['value'] = $scope.NodeList[nodeIndex]['HostName'] ;
         $scope.Template['inputList'][1]['valid'] = [] ;
         $.each( $scope.Configure['HostInfo'], function( index, hostInfo ){
            $scope.Template['inputList'][1]['valid'].push( { 'key': hostInfo['HostName'], 'value': hostInfo['HostName'] } ) ;
         } ) ;
         $scope.CurrentNode = nodeIndex ;
      }

      //获取配置
      var getModuleConfig = function(){
         var data = { 'cmd': 'get business config', 'TemplateInfo': JSON.stringify( $scope.Configure ) } ;
         SdbRest.OmOperation( data, {
            'success': function( configure ){
               configure[0]['Property'] = _Deploy.ConvertTemplate( configure[0]['Property'], 0 ) ;
               $scope.installConfig = configure[0] ;
               $scope.NodeList = configure[0]['Config'] ;
               $scope.Template = {
                  'keyWidth': '160px',
                  'inputList': $.extend( true, [], configure[0]['Property'] )
               } ;
               $scope.Template['inputList'].splice( 1, 0, {
                  "name": "HostName",
                  "webName": $scope.autoLanguage( '主机名' ),
                  "type": "select",
                  "value": 0,
                  "valid": []
               } ) ;
               $scope.$apply() ;
               $scope.SwitchNode( 0 ) ;
            },
            'failed': function( errorInfo ){
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  getModuleConfig() ;
                  return true ;
               } ) ;
            }
         } ) ;
      }

      getModuleConfig() ;

      //安装业务
      var installSdb = function( installConfig ){
         var data = { 'cmd': 'add business', 'ConfigInfo': JSON.stringify( installConfig ) } ;
         SdbRest.OmOperation( data, {
            'success': function( taskInfo ){
               $rootScope.tempData( 'Deploy', 'ModuleTaskID', taskInfo[0]['TaskID'] ) ;
               $location.path( '/Deploy/Task/Module' ).search( { 'r': new Date().getTime() } ) ;
            },
            'failed': function( errorInfo ){
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  installSdb( installConfig ) ;
                  return true ;
               } ) ;
            }
         } ) ;
      }

      //跳转到安装业务
      $scope.GotoInstall = function(){
         var configure = {} ;
         configure['ClusterName']  = $scope.installConfig['ClusterName'] ;
         configure['BusinessType'] = $scope.installConfig['BusinessType'] ;
         configure['BusinessName'] = $scope.installConfig['BusinessName'] ;
         configure['DeployMod']    = $scope.installConfig['DeployMod'] ;
         configure['Config']       = $.extend( true, [], $scope.installConfig['Config'] ) ;
         $.each( configure['Config'], function( index ){
            configure['Config'][index] = convertJsonValueString( configure['Config'][index] ) ;
         } ) ;
         installSdb( configure ) ;
      }

      //返回
      $scope.GotoDeploy = function(){
         $location.path( '/Deploy/Index' ).search( { 'r': new Date().getTime() } ) ;
      }

   } ) ;
}());