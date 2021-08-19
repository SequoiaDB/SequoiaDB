//@ sourceURL=Mod.js
(function(){
   var sacApp = window.SdbSacManagerModule ;
   //控制器
   sacApp.controllerProvider.register( 'Deploy.Ssql.Mod.Ctrl', function( $scope, $compile, $location, $rootScope, SdbRest ){

      //初始化
      $scope.NodeListOptions = {
         'titleWidth': [ '26px', 23, 18, 31, 10, 18 ]
      } ;
      $scope.NodeList = [] ;
      $scope.Template = {
         'keyWidth': '160px',
         'inputList': []
      } ;
      $scope.installConfig = {} ;
      $scope.CurrentNode   = 0 ;
      $scope.IsAllClear    = true ;

      $scope.Configure   = $rootScope.tempData( 'Deploy', 'ModuleConfig' ) ;
      var deployType     = $rootScope.tempData( 'Deploy', 'Model' ) ;
      
      /*
      deployType = "Module" ;
      $scope.Configure = {
         "ClusterName": "myCluster2",
         "BusinessName": "myModule4",
         "DeployMod": "olap",
         "BusinessType": "sequoiasql",
         "Property": [
            { "Name": "deploy_standby", "Value": "false" },
            { "Name": "segment_num", "Value": "1" }
         ],
         "HostInfo": [
            { "HostName": "ubuntu-test-05" }
         ]
      } ;
      */
      
      if( deployType == null || $scope.Configure == null )
      {
         $location.path( '/Deploy/Index' ).search( { 'r': new Date().getTime() } ) ;
         return ;
      }

      $scope.stepList = _Deploy.BuildSdbStep( $scope, $location, deployType, $scope['Url']['Action'], 'sequoiasql' ) ;
      if( $scope.stepList['info'].length == 0 )
      {
         $location.path( '/Deploy/Index' ).search( { 'r': new Date().getTime() } ) ;
         return ;
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
               var masterHost = '' ;
               var standbyIndex = 0 ;
               $.each( $scope.Template['inputList'], function( index ){
                  var name = $scope.Template['inputList'][index]['name'] ;
                  $scope.Template['inputList'][index]['value'] = $scope.NodeList[0][name] ;
                  if( name == 'master_host' )
                  {
                     masterHost = $scope.Template['inputList'][index]['value'] ;
                     $scope.Template['inputList'][index]['type'] = 'select' ;
                     $scope.Template['inputList'][index]['valid'] = [] ;
                     $.each( $scope.Configure['HostInfo'], function( index2, hostInfo ){
                        $scope.Template['inputList'][index]['valid'].push( { 'key': hostInfo['HostName'], 'value': hostInfo['HostName'] } ) ;
                     } ) ;
                     $scope.Template['inputList'][index]['onChange'] = function( name, key, value ){
                        masterHost = value ;
                        $scope.NodeList[0]['master_host'] = masterHost ;
                        if( $scope.Template['inputList'][standbyIndex]['value'] == masterHost )
                        {
                           $scope.Template['inputList'][standbyIndex]['value'] = '' ;
                        }
                        $scope.Template['inputList'][standbyIndex]['valid'] = [] ;
                        $scope.Template['inputList'][standbyIndex]['valid'].push( { 'key': $scope.autoLanguage( '无' ), 'value': '' } ) ;
                        $.each( $scope.Configure['HostInfo'], function( index2, hostInfo ){
                           if( hostInfo['HostName'] != masterHost )
                           {
                              $scope.Template['inputList'][standbyIndex]['valid'].push( { 'key': hostInfo['HostName'], 'value': hostInfo['HostName'] } ) ;
                           }
                        } ) ;
                     }
                  }
                  else if( name == 'standby_host' )
                  {
                     $scope.Template['inputList'][index]['type'] = 'select' ;
                     $scope.Template['inputList'][index]['valid'] = [] ;
                     $scope.Template['inputList'][index]['valid'].push( { 'key': $scope.autoLanguage( '无' ), 'value': '' } ) ;
                     $.each( $scope.Configure['HostInfo'], function( index2, hostInfo ){
                        if( hostInfo['HostName'] != masterHost )
                        {
                           $scope.Template['inputList'][index]['valid'].push( { 'key': hostInfo['HostName'], 'value': hostInfo['HostName'] } ) ;
                        }
                        else
                        {
                           standbyIndex = index ;
                        }
                     } ) ;
                     $scope.Template['inputList'][index]['onChange'] = function( name, key, value ){
                        $scope.NodeList[0]['standby_host'] = value ;
                     }
                  }
                  else if( name == 'hdfs_url' )
                  {
                     $scope.Template['inputList'][index]['valid'] = {
                        'min': 1
                     } ;
                     $scope.Template['inputList'][index]['onChange'] = function(){
                        $scope.IsAllClear = $scope.Template.check() ;
                     }
                  }
                  else if( name == 'segment_hosts' )
                  {
                     $scope.Template['inputList'][index]['type'] = 'multiple' ;
                     $scope.Template['inputList'][index]['valid'] = {
                        'min': 1,
                        'list': []
                     } ;
                     $.each( $scope.Configure['HostInfo'], function( index2, hostInfo ){
                        $scope.Template['inputList'][index]['valid']['list'].push( {
                           'key': hostInfo['HostName'],
                           'value': hostInfo['HostName'],
                           'checked': $scope.Template['inputList'][index]['value'].indexOf( hostInfo['HostName'] ) >= 0
                        } ) ;
                     } ) ;
                     $scope.Template['inputList'][index]['onChange'] = function(){
                        $scope.IsAllClear = $scope.Template.check() ;
                     }
                  }
                  else
                  {
                      $scope.Template['inputList'][index]['onChange'] = function(){
                        $scope.IsAllClear = $scope.Template.check() ;
                      }
                  }
               } ) ;           
               $scope.$apply() ;
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
         if( $scope.Template.check() )
         {
            var formVal = $scope.Template.getValue() ;
            var configure = {} ;
            configure['ClusterName']  = $scope.installConfig['ClusterName'] ;
            configure['BusinessType'] = $scope.installConfig['BusinessType'] ;
            configure['BusinessName'] = $scope.installConfig['BusinessName'] ;
            configure['DeployMod']    = $scope.installConfig['DeployMod'] ;
            configure['Config']       = [ formVal ] ;
            $.each( configure['Config'], function( index ){
               configure['Config'][index] = convertJsonValueString( configure['Config'][index] ) ;
            } ) ;
            installSdb( configure ) ;
         }
         else
         {
            $scope.IsAllClear = false ;
         }
      }

      //上一步
      $scope.GotoConf = function(){
         $location.path( '/Deploy/SSQL-Conf' ).search( { 'r': new Date().getTime() } ) ;
      }

   } ) ;
}());