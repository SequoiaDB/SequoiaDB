//@ sourceURL=Procedure.js
(function(){
   var sacApp = window.SdbSacManagerModule ;
   //控制器
   sacApp.controllerProvider.register( 'Monitor.SdbResource.Procedure.Ctrl', function( $scope, $compile, $location, SdbRest, SdbFunction ){
      
      var clusterName = SdbFunction.LocalData( 'SdbClusterName' ) ;
      var moduleType = SdbFunction.LocalData( 'SdbModuleType' ) ;
      var moduleMode = SdbFunction.LocalData( 'SdbModuleMode' ) ;
      var moduleName = SdbFunction.LocalData( 'SdbModuleName' ) ;
      if( clusterName == null || moduleType != 'sequoiadb' || moduleMode == null || moduleName == null )
      {
         $location.path( '/Transfer' ).search( { 'r': new Date().getTime() } ) ;
         return;
      }

      //初始化
      //当前选择的存储过程函数的id
      $scope.currentProcedure = -1 ;
      //存储过程表格
      $scope.procedureTable = {
         'title': {
            'name': $scope.autoLanguage( '函数名' ),
            'func': $scope.autoLanguage( '代码' ),
            'funcType': $scope.autoLanguage( '类型' )
         },
         'body': [],
         'options': {
            'width': {
               'name': '200px',
               'funcType': '150px'
            },
            'sort': {
               'name': true,
               'func': true,
               'funcType': true
            },
            'max': 50,
            'filter': {
               'name': 'indexof',
               'func': 'indexof',
               'funcType': [
                  { 'key': $scope.autoLanguage( '全部' ), 'value': '' },
                  { 'key': $scope.autoLanguage( 'JS代码' ), 'value': 0 }
               ]
            }
         }
      } ;
      //创建存储过程窗口
      $scope.CreateProcedure = {
         'config': {
            'code': ''
         },
         'callback': {}
      } ;
      //删除存储过程窗口
      $scope.RemoveProcedure = {
         'config': {},
         'callback': {}
      }

      //获取存储过程列表
      var getProcedureList = function(){
         var data = { 'cmd': 'list procedures' } ;
         SdbRest.DataOperation( data, {
            'success': function( procedureList ){
               $.each( procedureList, function( index, procedure ){
                  if( index == 0 )
                  {
                     $scope.currentProcedure = 0 ;
                  }
                  procedureList[index]['i'] = index ;
                  procedureList[index]['func'] = js_beautify( procedureList[index]['func']['$code'], 3, ' ' ) ;
               } ) ;
               $scope.procedureTable['body'] = procedureList ;
            },
            'failed': function( errorInfo ){
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  getProcedureList() ;
                  return true ;
               } ) ;
            }
         }, { 'showLoading': false } ) ;
      }
      getProcedureList() ;

      //创建存储过程
      var createProcedure = function( code ){
         var check = false ;
         try{
            eval( 'var test = ' + code ) ;
            check = true ;
            if( hasChinese( test.getName() ) )
            {
               throw { 'message': 'Function name can not be Chinese.' } ;
            }
         }
         catch( e ){
            check = false ;
            alert( e.message ) ;
         }
         if( check == false )
            return ;
         $scope.CreateProcedure['callback']['Close']() ;
         var data = { 'cmd': 'create procedure', 'Code': code } ;
         SdbRest.DataOperation( data, {
            'success': function( procedureList ){
               getProcedureList() ;
            },
            'failed': function( errorInfo ){
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  createProcedure( code ) ;
                  return true ;
               } ) ;
            }
         } ) ;
      }

      //打开 创建存储过程 窗口
      $scope.OpenCreateProcedureWindow = function(){
         $scope.CreateProcedure['config']['code'] = '' ;
         //设置确定按钮
         $scope.CreateProcedure['callback']['SetOkButton']( $scope.autoLanguage( '确定' ), function(){
            createProcedure( $scope.CreateProcedure['config']['code'] ) ;
            return false ;
         } ) ;
         //关闭窗口滚动条
         $scope.CreateProcedure['callback']['DisableBodyScroll']() ;
         //设置标题
         $scope.CreateProcedure['callback']['SetTitle']( $scope.autoLanguage( '创建存储过程' ) ) ;
         //设置图标
         $scope.CreateProcedure['callback']['SetIcon']( 'fa-plus' ) ;
         //打开窗口
         $scope.CreateProcedure['callback']['Open']() ;
      }

      //删除存储过程
      var removeProcedure = function( func ){
         var data = { 'cmd': 'remove procedure', 'function': func } ;
         SdbRest.DataOperation( data, {
            'success': function( procedureList ){
               getProcedureList() ;
            },
            'failed': function( errorInfo ){
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  removeProcedure( func ) ;
                  return true ;
               } ) ;
            }
         } ) ;
      }

      //删除存储过程
      $scope.OpenProcedureInfoWindow = function( index ){
         $scope.RemoveProcedure['config'] = $scope.procedureTable['body'][index] ;
         //设置确定按钮
         $scope.RemoveProcedure['callback']['SetOkButton']( $scope.autoLanguage( '删除' ), function(){
            removeProcedure( $scope.RemoveProcedure['config']['name'] ) ;
            return true ;
         } ) ;
         //设置标题
         $scope.RemoveProcedure['callback']['SetTitle']( $scope.autoLanguage( '存储过程信息' ) ) ;
         //设置图标
         $scope.RemoveProcedure['callback']['SetIcon']( 'fa-plus' ) ;
         //打开窗口
         $scope.RemoveProcedure['callback']['Open']() ;
      }
     
       //跳转至资源
      $scope.GotoResources = function(){
         $location.path( '/Monitor/SDB-Resources/Session' ).search( { 'r': new Date().getTime() } ) ;
      } ;

      //跳转至主机列表
      $scope.GotoHosts = function(){
         $location.path( '/Monitor/SDB-Host/List/Index' ).search( { 'r': new Date().getTime() } ) ;
      } ;
      
      
      //跳转至节点列表
      $scope.GotoNodes = function(){
         $location.path( '/Monitor/SDB-Nodes/Nodes' ).search( { 'r': new Date().getTime() } ) ;
      } ;
   } ) ;
}());