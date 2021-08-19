(function(){
   window.SdbSacName = 'Login' ;
   window.SdbSacManagerConf.nowRoute = [
      { path: '/',
        options: {
           templateUrl: './app/template/Public/Login.html',
           resolve: resolveFun( [ './app/controller/Login.js' ] )
        }
      }
   ] ;
   window.SdbSacManagerConf.defaultRoute = { redirectTo: '/' } ;
}());