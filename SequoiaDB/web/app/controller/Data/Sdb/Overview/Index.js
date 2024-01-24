//@ sourceURL=Index.js
(function(){
   var sacApp = window.SdbSacManagerModule ;
   //控制器
   sacApp.controllerProvider.register( 'Data.Overview.Index.Ctrl', function( $scope, $location, SdbRest, SdbFunction ){

      var clusterName = SdbFunction.LocalData( 'SdbClusterName' ) ;
      if( clusterName == null )
      {
         SdbFunction.LocalData( 'SdbModuleMode', null ) ;
         SdbFunction.LocalData( 'SdbModuleName', null ) ;
         window.location.href = '/deployment/index.html' ;
         return;
      }

      var moduleType = SdbFunction.LocalData( 'SdbModuleType' ) ;
      if( moduleType != 'sequoiadb' )
      {
         window.location.href = '/deployment/index.html' ;
      }

      //业务模板列表
      $scope.moduleTemplate = [] ;
      $scope.lastRecord = [] ;

      //获取业务模板
      _DataOverviewIndex.getModuleTemplate( $scope, SdbRest, clusterName ) ;

      //跳转到数据库页面
      $scope.gotoDatabase = function( moduleIndex ){
         _DataOverviewIndex.gotoDatabase( $scope, $location, SdbFunction, moduleIndex ) ;
      }

   } ) ;
}());