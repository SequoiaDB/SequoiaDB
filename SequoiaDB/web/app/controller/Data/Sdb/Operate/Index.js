//@ sourceURL=Index.js
(function(){
   var sacApp = window.SdbSacManagerModule ;
   var GridId ;
   sacApp.controllerProvider.register( 'Data.Operate.Index.Ctrl', function( $scope, $compile, $location, SdbRest, SdbFunction ){
      var clusterName = SdbFunction.LocalData( 'SdbClusterName' ) ;
      var moduleType = SdbFunction.LocalData( 'SdbModuleType' ) ;
      var moduleMode = SdbFunction.LocalData( 'SdbModuleMode' ) ;
      var moduleName = SdbFunction.LocalData( 'SdbModuleName' ) ;
      if( clusterName == null || moduleType != 'sequoiadb' || moduleMode == null || moduleName == null )
      {
         window.location.href = '/deployment/index.html' ;
         return;
      }

      printfDebug( 'Cluster: ' + clusterName + ', Type: ' + moduleType + ', Module: ' + moduleName + ', Mode: ' + moduleMode ) ;
      
      //初始化
      _DataOperateIndex.init( $scope, moduleName, moduleMode ) ;

      //页面跳转
      $scope.gotoRecord = function( listIndex ){
         _DataOperateIndex.gotoRecord( $scope, $location, SdbFunction, listIndex ) ;
      }

      //Lob页面跳转
      $scope.gotoLob = function( listIndex ){
         _DataOperateIndex.gotoLob( $scope, $location, SdbFunction, listIndex ) ;
      }

      //上一页
      $scope.previous = function(){
         _DataOperateIndex.previous( $scope, $compile ) ;
      }

      //检查输入的页数格式
      $scope.checkCurrent = function(){
         _DataOperateIndex.checkCurrent( $scope ) ;
      }

      //跳转到指定页
      $scope.gotoPate = function( event ){
         _DataOperateIndex.gotoPate( $scope, $compile, event ) ;
      }

      //下一页
      $scope.nextPage = function(){
         _DataOperateIndex.nextPage( $scope, $compile ) ;
      }

      //获取集合列表
      $scope.getCLList = function(){
         _DataOperateIndex.getCLList( $scope, $compile, SdbRest, moduleName, moduleMode ) ;
      }
      
      _DataOperateIndex.getCLList( $scope, $compile, SdbRest, moduleName, moduleMode ) ;
   } ) ;
}());