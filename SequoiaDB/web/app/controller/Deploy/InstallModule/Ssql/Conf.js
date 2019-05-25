//@ sourceURL=Conf.js
(function(){
   var sacApp = window.SdbSacManagerModule ;
   //控制器
   sacApp.controllerProvider.register( 'Deploy.Ssql.Conf.Ctrl', function( $scope, $compile, $location, $rootScope, SdbRest ){

      $scope.DeployType  = $rootScope.tempData( 'Deploy', 'Model' ) ;
      var clusterName    = $rootScope.tempData( 'Deploy', 'ClusterName' ) ;
      $scope.ModuleName  = $rootScope.tempData( 'Deploy', 'ModuleName' ) ;

      /*
      $scope.DeployType = 'Module' ;
      clusterName = 'myCluster1' ;
      $scope.ModuleName = 'myModule2' ;
      */
      
      if( $scope.DeployType == null || clusterName == null || $scope.ModuleName == null )
      {
         $location.path( '/Deploy/Index' ).search( { 'r': new Date().getTime() } ) ;
         return ;
      }

      $scope.stepList = _Deploy.BuildSdbStep( $scope, $location, $scope.DeployType, $scope['Url']['Action'], 'sequoiasql' ) ;
      if( $scope.stepList['info'].length == 0 )
      {
         $location.path( '/Deploy/Index' ).search( { 'r': new Date().getTime() } ) ;
         return ;
      }

      //初始化
      $scope.IsAllClear = true ;
      $scope.Conf1 = {} ;
      $scope.Conf2 = {} ;
      $scope.HostList = [] ;
      $scope.templateList = [] ;
      $scope.ConfForm1 = { 'keyWidth': '160px', 'inputList': [] } ;
      $scope.ConfForm2 = { 'keyWidth': '160px', 'inputList': [] } ;
      //选择安装业务的主机 弹窗
      $scope.SwitchHostWindow = {
         'config': {},
         'callback': {}
      } ;

      //获取业务模板
      var getBusinessTemplate = function(){
         var data = { 'cmd': 'get business template', 'BusinessType': 'sequoiasql' } ;
         SdbRest.OmOperation( data, {
            'success': function( templateList ){
               $.each( templateList, function( index ){
                  templateList[index]['Property'] = _Deploy.ConvertTemplate( templateList[index]['Property'] ) ;
               } ) ;
               var confForm = {
                  'keyWidth': '160px',
                  'inputList': [
                     {
                        "name": "type",
                        "webName": $scope.autoLanguage( '部署模式' ),
                        "type": "select",
                        "value": "",
                        "valid": []
                     }
                  ]
               } ;
               $.each( templateList, function( index, template ){
                  if( index == 0 )
                  {
                     confForm['inputList'][0]['value'] = template['DeployMod'] ;
                     $scope.ConfForm2 = { 'keyWidth': '160px', 'inputList': template['Property'] } ;
                  }
                  confForm['inputList'][0]['valid'].push( { 'key': template['WebName'], 'value': template['DeployMod'] } ) ;
               } ) ;
               $scope.templateList = templateList ;
               $scope.ConfForm1 = confForm ;
            },
            'failed': function( errorInfo ){
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  getBusinessTemplate() ;
                  return true ;
               } ) ;
            }
         } ) ;
      }

      //获取主机列表
      var getHostList = function(){
         var filter = { "ClusterName": clusterName } ;
         var data = { 'cmd': 'query host', 'filter': JSON.stringify( filter ) } ;
         SdbRest.OmOperation( data, {
            'success': function( hostList ){
               $scope.HostList = hostList ;
               $scope.Conf1['DiskNum'] = 0 ;
               $.each( $scope.HostList, function( index, hostInfo ){
                  $scope.HostList[index]['checked'] = true ;
                  $.each( hostInfo['Disk'], function( index2, diskInfo ){
                     if( diskInfo['IsLocal'] == true )
                     {
                        ++$scope.Conf1['DiskNum'] ;
                     }
                  } ) ;
               } )  ;
               getBusinessTemplate() ;
            },
            'failed': function( errorInfo ){
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  getHostList() ;
                  return true ;
               } ) ;
            }
         } ) ;
      }

      //返回
      $scope.GotoDeploy = function(){
         $location.path( '/Deploy/Index' ).search( { 'r': new Date().getTime() } ) ;
      }

      //上一步
      $scope.GotoScanHost = function(){
         $location.path( '/Deploy/Install' ) ;
      }

      //获取配置
      var getModuleConfig = function( Configure ){
         var data = { 'cmd': 'get business config', 'TemplateInfo': JSON.stringify( Configure ) } ;
         SdbRest.OmOperation( data, {
            'success': function(){
               $rootScope.tempData( 'Deploy', 'ModuleConfig', Configure ) ;
               $location.path( '/Deploy/SSQL-Mod' ).search( { 'r': new Date().getTime() } ) ;
            },
            'failed': function( errorInfo ){
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  getModuleConfig() ;
                  return true ;
               } ) ;
            }
         } ) ;
      }

      //下一步
      $scope.GotoModSsql = function(){
         var isAllClear1 = $scope.ConfForm1.check() ;
         var isAllClear2 = $scope.ConfForm2.check() ;
         $scope.IsAllClear = isAllClear1 && isAllClear2 ;
         if( $scope.IsAllClear )
         {
            var formVal1 = $scope.ConfForm1.getValue() ;
            var formVal2 = $scope.ConfForm2.getValue() ;
            var tempHostInfo = [] ;
			   $.each( $scope.HostList, function( index, value ){
               if( value['checked'] == true )
               {
				      tempHostInfo.push( { 'HostName': value['HostName'] } ) ;
               }
			   } ) ;
            if( tempHostInfo.length == 0 )
            {
               $.each( $scope.HostList, function( index, value ){
				      tempHostInfo.push( { 'HostName': value['HostName'] } ) ;
			      } ) ;
            }
            var businessConf = {} ;
            businessConf['ClusterName']  = clusterName ;
            businessConf['BusinessName'] = $scope.ModuleName ;
            businessConf['BusinessType'] = 'sequoiasql' ;
            businessConf['DeployMod'] = formVal1['type'] ;
            businessConf['Property'] = [
               { "Name": "deploy_standby", "Value": formVal2['deploy_standby'] },
               { "Name": "segment_num", "Value": formVal2['segment_num'] + '' }
            ] ;
            businessConf['HostInfo'] = tempHostInfo ;
            getModuleConfig( businessConf ) ;
         }
      }

      $scope.SelectAll = function(){
         $.each( $scope.HostList, function( index ){
            $scope.HostList[index]['checked'] = true ;
         } ) ;
      }

      $scope.Unselect = function(){
         $.each( $scope.HostList, function( index ){
            $scope.HostList[index]['checked'] = !$scope.HostList[index]['checked'] ;
         } ) ;
      }
 
      //打开 选择安装业务的主机 弹窗
      $scope.OpenSwitchHost = function(){
         var tempHostList = $.extend( true, [], $scope.HostList ) ;
         $scope.SwitchHostWindow['callback']['SetOkButton']( $scope.autoLanguage( '确定' ), function(){
            return true ;
         } ) ;
         $scope.SwitchHostWindow['callback']['SetCloseButton']( $scope.autoLanguage( '取消' ), function(){
            $.each( tempHostList, function( index ){
               $scope.HostList[index]['check'] = tempHostList[index]['check'] ;
            } ) ;
            return true ;
         } ) ;
         $scope.SwitchHostWindow['callback']['SetTitle']( $scope.autoLanguage( '主机列表' ) ) ;
         $scope.SwitchHostWindow['callback']['Open']() ;
      }

      getHostList() ;

   } ) ;
}());