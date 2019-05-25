//@ sourceURL=Index.js
(function(){
   var sacApp = window.SdbSacManagerModule ;
   //控制器
   sacApp.controllerProvider.register( 'Index.Index.Index.Ctrl', function( $scope, SdbRest ){

      function getCLusterList(){
         var data = { 'cmd': 'query cluster' } ;
         SdbRest.OmOperation( data, {
            'success': function( json ){
               $scope.QueryCluster = json ;
               $scope.$apply() ;
            }
         }, {
            'showLoading': false
         } ) ;
      }
      getCLusterList() ;
   } ) ;
}());