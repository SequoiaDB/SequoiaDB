//@ sourceURL=Lobs.js
(function(){
   var sacApp = window.SdbSacManagerModule ;
   var GridId ;
   sacApp.controllerProvider.register( 'Data.Lob.Lobs.Ctrl', function( $scope, $compile, $location, SdbRest, SdbFunction ){
      var clusterName = SdbFunction.LocalData( 'SdbClusterName' ) ;
      var moduleType = SdbFunction.LocalData( 'SdbModuleType' ) ;
      var moduleMode = SdbFunction.LocalData( 'SdbModuleMode' ) ;
      var moduleName = SdbFunction.LocalData( 'SdbModuleName' ) ;
      if( clusterName == null || moduleType != 'sequoiadb' || moduleMode == null || moduleName == null )
      {
         $location.path( '/Transfer' ).search( { 'r': new Date().getTime() } ) ;
         return;
      }

      var csName = SdbFunction.LocalData( 'SdbCsName' ) ;
      var clName = SdbFunction.LocalData( 'SdbClName' ) ;
      var clType = SdbFunction.LocalData( 'SdbClType' ) ;
      if( csName == null || clName == null || clType == null )
      {
         $location.path( 'Data/SDB-Operate/Index' ).search( { 'r': new Date().getTime() } ) ;
         return;
      }

      printfDebug( 'Cluster: ' + clusterName + ', Type: ' + moduleType + ', Module: ' + moduleName + ', Mode: ' + moduleMode ) ;

      //cs.cl
      $scope.fullName = csName + '.' + clName ;
      //lob表格
      $scope.lobTable = {
         'title': {
            'Oid':                     '',
            'Oid.$oid':               'Oid',
            'CreateTime.$timestamp':  'CreateTime',
            'Size':                   'Size',
            'Available' :  '           Available'
         },
         'body': [],
         'options': {
            'width': {
               'Oid': '40px',
               'Oid.$oid': '250px'
            },
            'sort': {
               'Oid':                      false,
               'Oid.$oid':                true,
               'CreateTime.$timestamp':   true,
               'Size':                    true,
               'Available':               true,
            },
            'max': 50,
            'filter': {
               'Oid.$oid':                 'indexof',
               'CreateTime.$timestamp':    'indexof',
               'Size':                     'number',
               'Available':                'indexof'
            }
         },
         'callback': {}
      } ;

      //Lob列表
      var lobList = [] ;

      //查询所有Lob
      $scope.queryAll = function(){
         var data = { 'cmd': 'list lobs', 'name': $scope.fullName } ;
         SdbRest.DataOperation( data, {
            'success': function( lobs ){
               lobList = lobs ;
               $scope.execResult = sprintf( $scope.autoLanguage( '? ? 执行查询成功，总计 ? 条记录' ), timeFormat( new Date(), 'hh:mm:ss' ), $scope.fullName, lobList.length ) ;
               $scope.execRc = true ;
               $scope.lobTable['body'] = lobs ;
            },
            'failed': function( errorInfo ){
               $scope.execResult = sprintf( $scope.autoLanguage( '? ? 执行查询失败，错误码: ?，?. ?' ), timeFormat( new Date(), 'hh:mm:ss' ), $scope.fullName, errorInfo['errno'], errorInfo['description'], errorInfo['detail'] ) ;
               $scope.execRc = false ;
            }
         } ) ;
      }
      
      //显示lob的详细信息
      $scope.showLobInfo = function( oid ){
         $scope.Components.Modal.isShow = true ;
         $scope.Components.Modal.icon = '' ;
         $scope.Components.Modal.title = $scope.autoLanguage( 'Lob信息' ) ;
         $scope.Components.Modal.Grid = {} ;
         $.each( lobList, function( index, lob ){
            if( lob['Oid']['$oid'] == oid )
            {
               $scope.Components.Modal.Grid = lob ;
               return false ;
            }
         } ) ;
         $scope.Components.Modal.Context = '\
<table class="table loosen border">\
<tr>\
<td style="width:40%;background-color:#F1F4F5;"><b>Key</b></td>\
<td style="width:60%;background-color:#F1F4F5;"><b>Value</b></td>\
</tr>\
<tr>\
<td>Oid</td>\
<td>{{data.Grid[\'Oid\'][\'$oid\']}}</td>\
</tr>\
<tr ng-repeat="(key, value) in data.Grid track by $index" ng-if="key != \'Oid\'">\
<td>{{key}}</td>\
<td ng-if="key != \'CreateTime\'">{{value}}</td>\
<td ng-if="key == \'CreateTime\'">{{value[\'$timestamp\']}}</td>\
</tr>\
</table>' ;
         $scope.Components.Modal.noOK = true ;
      }

      $scope.queryAll();

      $scope.GoToCL = function(){
         SdbFunction.LocalData( 'SdbFullName', $scope.fullName ) ;
         $location.path( 'Data/SDB-Database/Index' ).search( { 'r': new Date().getTime() } ) ;
      }
   } ) ;
}()) ;