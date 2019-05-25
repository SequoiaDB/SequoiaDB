(function(){
   var sacApp = window.SdbSacManagerModule ;
   function _getPluginRoute( async, clusterName, businessName, func )
   {
      $.ajax( {
         'url': './app/config/Route.json',
         'dataType': 'json',
         'async': async,
         'beforeSend': function( jqXHR ){
            jqXHR.setRequestHeader( 'SdbClusterName', clusterName ) ;
	         jqXHR.setRequestHeader( 'SdbBusinessName', businessName ) ;
         },
         'success': function( response, status ){
            func( response ) ;
         }
      } ) ;
   }
   function _getBusinessList( async, func )
   {
      $.ajax( {
         'method': 'post',
         'data': { 'cmd': 'query business', 'sort': JSON.stringify( { 'BusinessType': 1, 'BusinessName': 1, 'ClusterName': 1 } ) },
         'url': '/',
         'async': async,
         'dataType': 'text',
         'beforeSend': function( jqXHR ){
            var id = localLocalData( 'SdbSessionID' ) ;
	         if( id !== null )
	         {
		         jqXHR.setRequestHeader( 'SdbSessionID', id ) ;
	         }
	         var language = localLocalData( 'SdbLanguage' )
	         if( language !== null )
	         {
		         jqXHR.setRequestHeader( 'SdbLanguage', language ) ;
	         }
         },
         'success': function( response, status ){
            var businessList = parseJson2( response, true, null ) ;
            if( businessList[0]['errno'] === 0 )
            {
               businessList.splice( 0, 1 ) ;
            }
            else
            {
               businessList = [] ;
            }
            func( businessList ) ;
         }
      } ) ;
   }
   function _loadPluginRoute( async, $routeProvider )
   {
      _getBusinessList( async, function( buzList ){
         $.each( buzList, function( index, buzInfo ){
            if( buzInfo['BusinessType'] != 'sequoiadb' )
            {
               _getPluginRoute( async, buzInfo['ClusterName'], buzInfo['BusinessName'], function( routes ){
                  $.each( routes['route'], function( index, aRoute ){
                     aRoute['options']['resolve'] = resolveFun( aRoute['options']['resolve'] ) ;
                     $routeProvider.when( aRoute['path'], aRoute['options'] ) ;
                  } ) ;
               } ) ;
            }
         } ) ;
      } ) ;
   }
   sacApp.config( function( $sceProvider, $routeProvider, $controllerProvider, $compileProvider ){
      //控制器工厂
      window.SdbSacManagerModule.controllerProvider = $controllerProvider ;
      //指令工厂
      window.SdbSacManagerModule.compileProvider = $compileProvider ;
      //兼容IE7
      $sceProvider.enabled( false ) ;
      /* --- 路由列表 -- */
      var aRoute = window.SdbSacManagerConf.nowRoute ;
      var len = aRoute.length ;
      for( var i = 0; i < len; ++i )
      {
         $routeProvider.when( aRoute[i]['path'], aRoute[i]['options'] ) ;
      }
      //动态注册插件的路由表
      _loadPluginRoute( false, $routeProvider ) ;
      setInterval( function(){
         _loadPluginRoute( true, $routeProvider ) ;
      }, 10000 ) ;
      //默认访问
      $routeProvider.otherwise( window.SdbSacManagerConf.defaultRoute ) ;
   } ) ;
   //自定义排序
   sacApp.filter( 'orderObjectBy', function(){
      return function( items, field, reverse ){
         var filtered = [] ;
         var fields = field.split( '.' ) ;
         var fieldLength = fields.length ;
         var length = items.length ;
         var sortValue = null ;
         for( var i = 0; i < length; ++i )
         {
            sortValue = items[i] ;
            for( var k = 0; k < fieldLength; ++k )
               sortValue = sortValue[ fields[k] ] ;
            if( isNaN( sortValue ) )
               sortValue = String( sortValue ) ;
            else
               sortValue = Number( sortValue ) ;
            filtered.push( { 'v': sortValue, 's': items[i] } ) ;
         }
         filtered.sort( function( a, b ){
            var t1 = typeof( a['v'] ) ;
            var t2 = typeof( b['v'] ) ;
            if( t1 != t2 )
               return t1 > t2 ? 1 : -1 ;
            return ( a['v'] > b['v'] ? 1 : -1 ) ;
         } ) ;
         if( reverse )
         {
            filtered.reverse() ;
         }
         var result = [] ;
         for( var i = 0; i < length; ++i )
         {
            result.push( filtered[i]['s'] ) ;
         }
         return result ;
      } ;
   } ) ;
}());