function loginTransfer( $scope, SdbRest, SdbFunction ){
   var data = { 'cmd': 'query business', 'sort': JSON.stringify( { 'BusinessName': 1, 'ClusterName': 1 } ) } ;
   SdbRest.OmOperation( data, {
      'success': function( moduleList ){
         if( moduleList.length > 0 )
         {
            var moduleInfo = moduleList[0] ;
            SdbFunction.LocalData( 'SdbClusterName', moduleInfo['ClusterName'] ) ;
            SdbFunction.LocalData( 'SdbModuleName', moduleInfo['BusinessName'] ) ;
            SdbFunction.LocalData( 'SdbModuleType', moduleInfo['BusinessType'] ) ;
            SdbFunction.LocalData( 'SdbModuleMode', moduleInfo['DeployMod'] ) ;
            if( moduleInfo['BusinessType'] == 'sequoiadb' )
            {
               window.location.href = '/#/Monitor/SDB/Index' ;
            }
            else if( moduleInfo['BusinessType'] == 'sequoiasql' )
            {
               window.location.href = '/#/Data/SQL-Metadata/Index' ;
            }
            else if( moduleInfo['BusinessType'] == 'hdfs' )
            {
               window.location.href = '/#/Data/HDFS-web/Index' ;
            }
            else if( moduleInfo['BusinessType'] == 'yarn' )
            {
               window.location.href = '/#/Data/YARN-web/Index' ;
            }
            else
            {
               window.location.href = '/' ;
            }
         }
         else
         {
            window.location.href = '/' ;
         }
      },
      'failed': function( errorInfo ){
         window.location.href = '/' ;
      },
      'error': function(){
         window.location.href = '/' ;
      }
   } ) ;
}

(function(){
   var sacApp = window.SdbSacManagerModule ;
   //全局模板
   sacApp.controllerProvider.register( 'Login.Ctrl', function ( $scope, $rootScope, $window, $compile, SdbFunction, SdbRest ) {
      //默认语言
      if( SdbFunction.LocalData( 'SdbLanguage' ) == null )
      {
	      var language = navigator.userLanguage ;
	      if( typeof( language ) === 'undefined' )
	      {
		      language = navigator.language ;
	      }
	      language = language.substr( 0, 2 ) ;
	      if( language === 'zh' )
	      {
		      language = 'zh-CN' ;
	      }
	      if( typeof( language ) === 'undefined' || language === 'undefined' )
	      {
		      language = 'en' ;
	      }
	      SdbFunction.LocalData( 'SdbLanguage', language ) ;
      }

      //获取语言
      $scope.Language = SdbFunction.LocalData( 'SdbLanguage' ) ;
      //语言控制
      $rootScope.autoLanguage = function( text ){
         return _IndexPublic.languageCtrl( $scope, text ) ;
      }
      $scope.isLoading = false ;
      //用户名
      $scope.username = $scope.autoLanguage( '用户名' ) ;
      //密码
      $scope.password = '' ;
      //语言列表
      $scope.LanguageList = [ { 'key': 'en', 'value': 'English' }, { 'key': 'zh-CN', 'value': '中文' } ] ;
      //执行结果
      $scope.result = '' ;
      //用户名和密码的动画事件
      setTimeout( function(){
         $( '.animateTip' ).show() ;
         $scope.username = '' ;
         $scope.$apply() ;
         $( '.animateTip' ).animate( { 'right': '180px' }, 'normal', 'swing', function(){
            $scope.username = 'admin' ;
            $scope.$apply() ;
         } ) ;
      }, 400 ) ;
      //登陆
      $scope.login = function(){
         $scope.isLoading = true ;
         $scope.result = '' ;
	      SdbRest.Login( $scope.username, $scope.password, function( json, textStatus, jqXHR ){
		      var id = jqXHR.getResponseHeader( 'SdbSessionID' ) ;
		      SdbFunction.LocalData( 'SdbSessionID', id ) ;
		      SdbFunction.LocalData( 'SdbUser', $scope.username ) ;
            loginTransfer( $scope, SdbRest, SdbFunction ) ;
	      }, function( errorInfo ){
            $scope.result = errorInfo['detail'] ;
            $scope.isLoading = false
	      }, function( XMLHttpRequest, textStatus, errorThrown ){
            $scope.result = $scope.autoLanguage( '网络错误' ) ;
            $scope.isLoading = false
         }, function(){
            $scope.$apply() ;
         } ) ;
      }
      var mask = $( '<div class="mask-screen unalpha"></div>' ).on( 'click', function(){
         $( '#languageMenu' ).hide() ;
         mask.detach() ;
      } ) ;
      $scope.showLanguageMenu = function(){     
         $( '#languageMenu' ).show() ;
         mask.appendTo( document.body ) ;
      }
      $scope.chooseLanguage = function( listIndex ){
         var newLanguage = $scope.LanguageList[listIndex]['key'] ;
         $scope.Language = newLanguage ;
         SdbFunction.LocalData( 'SdbLanguage', newLanguage ) ;
         $( '#languageMenu' ).hide() ;
         mask.detach() ;
      }
      //回车事件
      angular.element( $window ).bind( 'keydown', function( event ){
         var e = event || $window.event;
         if( !e.ctrlKey && e.keyCode == 13 )
         {
            $scope.login() ;
         }
      } ) ;
   } ) ;
}());