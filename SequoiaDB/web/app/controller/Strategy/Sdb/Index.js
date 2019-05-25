//@ sourceURL=Index.js
(function(){
   var sacApp = window.SdbSacManagerModule ;
   //控制器
   sacApp.controllerProvider.register( 'Data.SQL.Index.Ctrl', function( $scope, $location, $compile, SdbFunction, SdbRest ){
      var clusterName = SdbFunction.LocalData( 'SdbClusterName' ) ;
      var moduleType = SdbFunction.LocalData( 'SdbModuleType' ) ;
      var moduleName = SdbFunction.LocalData( 'SdbModuleName' ) ;
      if( clusterName == null || moduleType != 'sequoiadb' || moduleName == null )
      {
         $location.path( '/Transfer' ).search( { 'r': new Date().getTime() } ) ;
         return;
      }

      printfDebug( 'Cluster: ' + clusterName + ', Type: ' + moduleType + ', Module: ' + moduleName ) ;

      var fieldGridData = {
         'title': [
            { 'text': '#' },
            { 'text': $scope.autoLanguage( '任务ID' ) },
            { 'text': $scope.autoLanguage( '任务名' ) },
            { 'text': $scope.autoLanguage( '优先级' ) }
         ],
         'body': [],
         'tool': {
            'position': 'bottom',
            'left': [
               { 'text': '' }
            ]
         },
         'options': {
            'grid': { 'tdModel': 'auto', 'tool': true, 'gridModel': 'fixed', titleWidth: [ '60px', '200px', 100, '70px' ] } 
         }
      } ;
      //SdbRest.getTaskList( '', function( taskList ){
         var taskList = [ { 'TaskID': 1, 'TaskName': 'aaa', 'Nice': 100 } ] ;
         $.each( taskList, function( index, taskInfo){
            fieldGridData['body'].push( [
               { 'text': index + 1 },
               { 'text': taskInfo['TaskID'] },
               { 'text': taskInfo['TaskName'] },
               { 'text': taskInfo['Nice'] }
            ] ) ;
         } ) ;
         fieldGridData['tool']['left'][0]['text'] = $scope.sprintf( $scope.autoLanguage( '一共?个任务' ), fieldGridData['body'].length ) ;
         $scope.fieldGridData = fieldGridData ;
         //$scope.$apply() ;
      //} ) ;
   } ) ;
}());