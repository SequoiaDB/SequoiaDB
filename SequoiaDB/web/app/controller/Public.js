(function(){
   var sacApp = window.SdbSacManagerModule ;
   //部署包的缩写列表
   var packageShortName = {
      'sequoiadb': 'sdb',
      'sequoiasql-postgresql': 'pgsql',
      'sequoiasql-mysql': 'mysql',
      'sequoiasql-mariadb': 'mariadb'
   } ;

   //全局模板
   sacApp.controller( 'Index.Ctrl', function( $scope, $window, $rootScope, $location, Tip, SdbFunction, Loading, SdbRest, SdbSignal ){
      //校验登录信息
      if( SdbFunction.LocalData( 'SdbUser' ) === null || SdbFunction.LocalData( 'SdbSessionID' ) === null )
		{
			window.location.href = '/login.html' ;
         return;
      }
      //设置默认语言
      if( SdbFunction.LocalData( 'SdbLanguage' ) == null )
      {
         SdbFunction.LocalData( 'SdbLanguage', 'zh-CN' ) ;
      }
      //预加载模板
      $scope.Templates = {} ;
      $scope.Templates.Top = './app/template/Public/Top.html' ;
      $scope.Templates.Left = './app/template/Public/Left.html' ;
      $scope.Templates.Bottom = './app/template/Public/Bottom.html' ;
      //获取语言
      $scope.Language = SdbFunction.LocalData( 'SdbLanguage' ) ;
      //初始化提示标签
      Tip.create() ;
      Tip.auto() ;
      //-------- 全局变量 ---------
      $rootScope.Url = { Module: '', Action: '', Method: '', Type: '' } ;
      //临时存储, 用于跨页用
      $rootScope.TempStorage = {} ;
      //用于触发自定义重绘
      $rootScope.onResize = 0 ;
      //当前所有任务的列表
      $rootScope.OmTaskList = [] ;
      //用于获取窗口宽度
      $rootScope.WindowWidth = $( 'body' ).width() ;
      //用于获取窗口高度
      $rootScope.WindowHeight = $( 'body' ).height() ;
      //-------- 全局组件 ---------
      $rootScope.Components = {} ;
      //错误信息的窗体
      $rootScope.Components.Confirm = {} ;
      //子窗口的窗体
      $rootScope.Components.Modal = {} ;
      //落地窗的窗体
      $rootScope.Components.French = {} ;
      //排版参数
      $rootScope.layout = {} ;
      $rootScope.layout.top     = { height: 40 } ;
      $rootScope.layout.content = { offsetY: -40 } ;
      $rootScope.layout.bottom  = { height: 40 } ;
      $rootScope.layout.left    = { width: 80 } ;
      $rootScope.layout.centre  = { offsetX: -80, marginLeft: 80 } ;
      //-------- 全局函数 ---------
      //格式化
      $rootScope.sprintf = sprintf ;
      //保留几位小数
      $rootScope.fixedNumber = fixedNumber ;
      //判断数组
      $rootScope.isArray = isArray ;
      //Json转字符串
      $rootScope.json2String = JSON.stringify ;
      //数值补位
      $rootScope.pad = pad ;
      //语言控制
      $rootScope.autoLanguage = function( text ){
         return _IndexPublic.languageCtrl( $scope, text ) ;
      }
      //插件语言控制
      $rootScope.pAutoLanguage = function( text ){
         return _IndexPublic.pLanguageCtrl( $scope, text ) ;
      }
      $rootScope.autoLanguage('确定') ;//马上调用，原因是firefox有bug，如果不调用会造成后续子页面加载后不执行代码。
      //读写临时存储
      $rootScope.tempData = function( domain, key, value ){
         if( typeof( key ) == 'undefined' && typeof( value ) == 'undefined' )
         {
            if( typeof( $rootScope.TempStorage[domain] ) == 'undefined' )
            {
               return ;
            }
            $rootScope.TempStorage[domain] = {} ;
         }
         else if( typeof( value ) == 'undefined' )
         {
            if( typeof( $rootScope.TempStorage[domain] ) == 'undefined' )
            {
               return null ;
            }
            return typeof( $rootScope.TempStorage[domain][key] ) == 'undefined' ? null : $rootScope.TempStorage[domain][key] ;
         }
         else
         {
            if( typeof( $rootScope.TempStorage[domain] ) == 'undefined' )
            {
               $rootScope.TempStorage[domain] = {} ;
            }
            $rootScope.TempStorage[domain][key] = value ;
         }
      }
      //初始化导航（实际实现在下面Left）
      $rootScope.initNav = function(){} ;
      //更新导航(实际实现在下面Left)
      $rootScope.updateNav = function(){} ;
      //更新Url变量
      $rootScope.updateUrl = function(){
         var url = $location.url() ;
         if( url.indexOf( '?' ) >= 0 )
         {
            url = url.split( '?' )[0] ;
         }
         var route = url.split( '/' ) ;
         $rootScope.Url.Module = route[1] ;
         $rootScope.Url.Action = route[2] ;
         $rootScope.Url.Method = route[3] ;
         $rootScope.Url.Type   = route[4] ;
      } ;
      //触发自定义的onResize
      $rootScope.bindResize = function(){
         var random = 0 ;
         do{
            random = Math.random() ;
         } while( random == $rootScope.onResize ) ;
         $rootScope.onResize = random ;
      } ;
      //Package包名缩写
      $rootScope.abbreviate = function( name ){
         var newName = packageShortName[name] ;
         if( typeof( newName ) == 'undefined' )
         {
            newName = name ;
         }
         return newName ;
      }
      //禁止F5做全局刷新，改成局部刷新
      $(document).bind("keydown",function(e){
         switch( e.keyCode )  
         {
         case 116:
            if( $rootScope.Url.Module == 'Deploy' )
            {
               var params = { 'r': new Date().getTime() } ;
               $location.path( $location.path() ).search( params ) ;
               e.keyCode = 0;
               return false;
            }
            break ;
         }
      } ) ;
      angular.element( $window ).bind( 'resize', function(){
         $rootScope.WindowWidth = $( 'body' ).width() ;
         $rootScope.WindowHeight = $( 'body' ).height() ;
      } ) ;

      var getOMSysInfo = function(){
         var data = { 'cmd': 'get system info' } ;
         SdbRest.OmOperation( data, {
            'success': function( systemInfo ){
               $.each( systemInfo[0], function( key, value ){
                  window.Config[ key ] = value ;
               } ) ;
               window.Config['recv'] = true ;

               SdbSignal.commit( 'sac_version', window.Config['Version'] ) ;
            },
            'failed': function( errorInfo ){
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  getOMSysInfo() ;
                  return true ;
               } ) ;
            }
         } ) ;
      }

      getOMSysInfo() ;
   } ) ;

   //顶部
   sacApp.controller( 'Index.Top.Ctrl', function( $scope, $location, $compile, $rootScope, SdbFunction, SdbRest ){

      var data = {
         'cmd': 'query task',
         'filter': JSON.stringify( { 'TaskName': { '$ne': 'SSQL_EXEC' } } ),
         'selector': JSON.stringify( { 'TaskID': 1, 'TaskName': 1, 'TaskID': 1, 'Info.BusinessName': 1, 'Progress': 1, 'Status': 1, 'errno': 1 } ),
         'sort': JSON.stringify( { 'Status': 1, 'TaskID': -1 } ),
         'returnnum': 50
      } ;

      $scope.Top = {} ;
      $scope.Top.TaskList = [] ;
      $scope.Top.ExecTaskNum = 0 ;

      //修改密码弹窗
      $scope.Top.ShowChangePasswd = function(){
         _IndexTop.createPasswdModel( $scope, SdbRest ) ;
      }
      //登出
      $scope.Top.Logout = function(){
         _IndexTop.logout( $location, SdbFunction ) ;
      }

      //用户操作下拉菜单
      $scope.Top.UserOperateDropdown = {
         'config': [
            { 'key': $scope.autoLanguage( '系统配置' ) },
            { 'key': $scope.autoLanguage( '历史记录' ) },
            { 'divider': true },
            { 'key': $scope.autoLanguage( '修改密码' ) },
            { 'key': $scope.autoLanguage( '注销' ) }
         ],
         'OnClick': function( index ){
            if( index == 0 )
            {
               openSetConfigWindow() ;
            }
            else if( index == 1 )
            {
               gotoHistory() ;
            }
            else if( index == 3 )
            {
               $scope.Top.ShowChangePasswd() ;
            }
            else
            {
               $scope.Top.Logout() ;
            }
            $scope.Top.UserOperateDropdown['callback']['Close']() ;
         },
         'callback': {}
      }

      var gotoHistory = function(){
         $location.path( '/System/History' ).search( { 'r': new Date().getTime() } ) ;
      }

      $scope.Top.SetConfigWindow = {
         'config': {},
         'callback': {}
      }
      var openSetConfigWindow = function( scope, SdbRest ){
         $scope.Top.SetConfigWindow['config'] = {
            'keyWidth':'200px',
            'inputList': [
               {
                  "name": 'max',
                  "webName": 'max_execution_time',
                  "desc": $scope.autoLanguage( '最大执行时间，目前仅执行 SQL 时有效，默认 30 秒，最小值 0 表示不超时，最大值 1800 秒' ),
                  "type": "int",
                  "required": true,
                  "value": 30,
                  "valid": {
                     "min": 0,
                     "max": 1800
                  }
               }
            ]
         } ;

         queryConfig() ;
      }

      function queryConfig()
      {
         var data = { 'cmd': 'list settings' } ;
         SdbRest.OmOperation( data, {
            'success': function( result ){
               $.each( result, function( index, info ){
                  if( info['Key'] == 'max_execution_time' )
                  {
                     info['Value'] = info['Value'] / 1000 ;
                     $scope.Top.SetConfigWindow['config']['inputList'][0]['value'] = info['Value'] ;
                  }
               } ) ;
               $scope.Top.SetConfigWindow['callback']['SetOkButton']( $scope.autoLanguage( '确定' ), function () {
                  var isAllClear = $scope.Top.SetConfigWindow['config'].check();
                  if( isAllClear )
                  {
                     var formVal = $scope.Top.SetConfigWindow['config'].getValue() ;
                     formVal['max'] = formVal['max'] * 1000 ;
                     setConfig( formVal['max'] ) ;
                  }
                  return isAllClear ;
               } );

               $scope.Top.SetConfigWindow['callback']['SetTitle']( $scope.autoLanguage( '系统配置' ) ) ;
               $scope.Top.SetConfigWindow['callback']['SetIcon']( '' ) ;
               $scope.Top.SetConfigWindow['callback']['Open']() ;
               $scope.$digest() ;
            },
            'failed': function( errorInfo ){
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  queryConfig() ;
                  return true ;
               } ) ;
            }
         } ) ;
      }

      function setConfig( max )
      {
         var settings = { 'max_execution_time': max } ;
         var data = { 'cmd': 'set settings', 'settings': JSON.stringify( settings ) };
         SdbRest.OmOperation( data, {
            'success': function(){
               $scope.Components.Confirm.type = 4 ;
               $scope.Components.Confirm.context = $scope.autoLanguage( '修改配置成功' ) ;
               $scope.Components.Confirm.isShow = true ;
               $scope.Components.Confirm.noClose = true ;
               $scope.Components.Confirm.normalOK = true ;
               $scope.Components.Confirm.okText = $scope.autoLanguage( '确定' ) ;
               $scope.Components.Confirm.ok = function(){
                  $scope.Components.Confirm.isShow = false ;
                  $scope.Components.Confirm.noClose = false ;
                  $scope.Components.Confirm.normalOK = false ;
               }
            },
            'failed': function( errorInfo ){
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  setConfig( max ) ;
                  return true ;
               } ) ;
            }
         }, {
            'showLoading': true
         } ) ;
      }

      //打开用户操作下拉菜单
      $scope.Top.OpenUserOperate = function( event ){
         $scope.Top.UserOperateDropdown['callback']['Open']( event.currentTarget ) ;
      }

      var NoticeFrench = {
         'title': $scope.autoLanguage( '通知列表' ),
         'empty': $scope.autoLanguage( '没有消息' ),
         'html': null
      } ;

      $scope.Components.French.TaskList = [] ;

      //创建 任务 落地窗
      $scope.CreateTaskFrench = function(){
         $scope.Components.French.GotoTaskPage = function( taskInfo ){
            $rootScope.tempData( 'Deploy', 'Model', 'Task' ) ;
            $rootScope.tempData( 'Deploy', 'Module', 'None' ) ;
            var params = { 'r': new Date().getTime() } ;
            if( taskInfo['TaskName'] == 'ADD_HOST' || taskInfo['TaskName'] == 'REMOVE_HOST' || taskInfo['TaskName'] == 'DEPLOY_PACKAGE' )
            {
               $rootScope.tempData( 'Deploy', 'HostTaskID', taskInfo['TaskID'] ) ;
               $location.path( '/Deploy/Task/Host' ).search( params ) ;
            }
            else if( taskInfo['TaskName'] == 'EXTEND_BUSINESS' )
            {
               $rootScope.tempData( 'Deploy', 'ModuleTaskID', taskInfo['TaskID'] ) ;
               $location.path( '/Deploy/SDB-ExtendInstall' ).search( params ) ;
            }
            else if( taskInfo['TaskName'] == 'ADD_BUSINESS' )
            {
               $rootScope.tempData( 'Deploy', 'ModuleTaskID', taskInfo['TaskID'] ) ;
               $location.path( '/Deploy/Task/Module' ).search( params ) ;
            }
            else if( taskInfo['TaskName'] == 'RESTART_BUSINESS' )
            {
               $rootScope.tempData( 'Deploy', 'ModuleTaskID', taskInfo['TaskID'] ) ;
               $location.path( '/Deploy/Task/Restart' ).search( params ) ;
            }
            else
            {
               $rootScope.tempData( 'Deploy', 'ModuleTaskID', taskInfo['TaskID'] ) ;
               $location.path( '/Deploy/Task/Module' ).search( params ) ;
            }
         }
         $scope.Components.French.title = $scope.autoLanguage( '任务列表' ) ;
         $scope.Components.French.empty = $scope.autoLanguage( '没有任务' ) ;
         $scope.Components.French.Context = '\
<div>\
   <div class="linkButton frenchWindowBox" ng-repeat="taskInfo in data.TaskList track by $index" ng-click="data.GotoTaskPage(taskInfo)">\
      <div class="Ellipsis frenchWindowText">{{taskInfo[\'TaskID\']}} {{taskInfo[\'TaskName\']}}</div>\
      <div progress-bar="taskInfo[\'barChart\']"></div>\
   </div>\
</div>' ;
         $scope.Components.French['isShow'] = true ;
      }

      SdbRest.OmOperation( data, {
         'success': function( taskList ){
            $rootScope.OmTaskList = taskList ;
            $scope.Top.ExecTaskNum = 0 ;
            $.each( taskList, function( index, taskInfo ){
               if( taskInfo['errno'] == 0 )
               {
                  if( taskInfo['Status'] == 4 )
                  {
                     taskList[index]['barChart'] = { 'percent': taskInfo['Progress'], 'style': { 'progress': { 'background': '#00CC66' } } } ;
                  }
                  else
                  {
                     if( taskInfo['Progress'] == 100 )
                     {
                        taskInfo['Progress'] = 90 ;
                     }
                     ++$scope.Top.ExecTaskNum ;
                     if( taskInfo['Status'] == 2 )
                     {
                        taskList[index]['barChart'] = { 'percent': taskInfo['Progress'], 'style': { 'progress': { 'background': '#FF9933' } } } ;
                     }
                     else
                     {
                        taskList[index]['barChart'] = { 'percent': taskInfo['Progress'], 'style': { 'progress': { 'background': '#188FF1' } } } ;
                     }
                  }
               }
               else
               {
                  if( taskInfo['TaskName'] !== 'EXTEND_BUSINESS' )
                  {
                     taskInfo['Progress'] = 100 ;
                  }
                  taskList[index]['barChart'] = { 'percent': taskInfo['Progress'], 'style': { 'progress': { 'background': '#D9534F' } } } ;
               }
               $scope.Components.French.TaskList = taskList ;
               $scope.Top.TaskList = taskList ;
            } ) ;
         }
      }, {
         'showLoading': false,
         'delay': 5000,
         'loop': true,
         'scope': false
      } ) ;
   } ) ;

   //左边
   sacApp.controller( 'Index.Left.Ctrl', function( $scope, $rootScope, $location, SdbRest, SdbFunction ){

      //导航标题列表
      var navTitleName = {
         'sequoiadb':               $scope.autoLanguage( '分布式存储' ),
         'sequoiasql-postgresql':   $scope.autoLanguage( '数据库实例' ),
         'sequoiasql-mysql':        $scope.autoLanguage( '数据库实例' ),
         'sequoiasql-mariadb':      $scope.autoLanguage( '数据库实例' ),
         'hdfs': 'HDFS',
         'yarn': 'YARN'
      } ;

      $scope.showModuleIndex = -1 ;
      $scope.Left = {} ;
      $scope.Left.nav1 = { width: 80 } ;
      $scope.Left.nav2 = { width: 180, marginLeft: 80 } ;
      $scope.Left.nav2Show = false ;
      $scope.Left.nav1Btn = { 'visibility': 'hidden' } ;
      $scope.Left.nav2Btn = { 'visibility': 'hidden' } ;
      $scope.Left.navMenu = [
         {
            'text': $scope.autoLanguage( '数据' ),
            'module': 'Data',
            'icon': 'fa-database',
            'list': [],
            'target': function( moduleType ){
               var params = { 'r': new Date().getTime() } ;
               switch( moduleType )
               {
               case 'sequoiadb':
                  $location.path( '/Data/SDB-Database/Index' ).search( params ) ; break ;
               case 'hdfs':
                  $location.path( '/Data/HDFS-web/Index' ).search( params ) ; break ;
               case 'spark':
                  $location.path( '/Data/SPARK-web/Index' ).search( params ) ; break ;
               case 'yarn':
                  $location.path( '/Data/YARN-web/Index' ).search( params ) ; break ;
               case 'sequoiasql-postgresql':
                  $location.path( '/Data/SequoiaSQL/PostgreSQL/Database/Index' ).search( params ) ; break ;
               case 'sequoiasql-mysql':
                  $location.path( '/Data/SequoiaSQL/MySQL/Database/Index' ).search( params ) ; break ;
               case 'sequoiasql-mariadb':
                  $location.path( '/Data/SequoiaSQL/MariaDB/Database/Index' ).search( params ) ; break ;
               default:
                  break ;
               }
            }
         },
         {
            'text': $scope.autoLanguage( '监控' ),
            'module': 'Monitor',
            'icon': 'fa-flash',
            'list': [],
            'target': function( moduleType ){
               var params = { 'r': new Date().getTime() } ;
               switch( moduleType )
               {
               case 'sequoiadb':
                  $location.path( '/Monitor/SDB/Index' ).search( params ) ; break ;
               default:
                  break ;
               }
            }
         },
         {
            'text': $scope.autoLanguage( '部署' ),
            'module': 'Deploy',
            'icon': 'fa-share-alt',
            'action': '/#/Deploy/Index'
         },
         {
            'text': $scope.autoLanguage( '策略' ),
            'module': 'Strategy',
            'icon': 'fa-tasks',
            'list': [],
            'target': function( moduleType ){
               var params = { 'r': new Date().getTime() } ;
               switch( moduleType )
               {
               case 'sequoiadb':
                  $location.path( '/Strategy/SDB/Index' ).search( params ) ; break ;
               default:
                  break ;
               }
            }
         },
         {
            'text': $scope.autoLanguage( '配置' ),
            'module': 'Config',
            'icon': 'fa-cogs',
            'list': [],
            'target': function( moduleType ){
               var params = { 'r': new Date().getTime() } ;
               switch( moduleType )
               {
               case 'sequoiadb':
                  $location.path( '/Config/SDB/Index' ).search( params ) ; break ;
               case 'sequoiasql-postgresql':
                  $location.path( '/Config/SequoiaSQL/PostgreSQL/Index' ).search( params ) ; break ;
               case 'sequoiasql-mysql':
                  $location.path( '/Config/SequoiaSQL/MySQL/Index' ).search( params ) ; break ;
               case 'sequoiasql-mariadb':
                  $location.path( '/Config/SequoiaSQL/MariaDB/Index' ).search( params ) ; break ;
               default:
                  break ;
               }
            }
         }
      ] ;

      var pluginList = [] ;

      function getPlugins()
      {
         var data = { 'cmd': 'list plugins' } ;
         SdbRest.OmOperation( data, {
            'success': function( list ){
               pluginList = list ;
            }
         }, {
            'delay': 5000,
            'loop': true,
            'scope': false,
            'showLoading': false
         } ) ;
      }

      function hasPlugin( type )
      {
         var has = false ;
         if( type == 'sequoiadb' ||
             type == 'hdfs' ||
             type == 'yarn' )
         {
            return true ;
         }
         $.each( pluginList, function( index, plugin ){
            if( plugin['BusinessType'] == type )
            {
               has = true ;
               return false ;
            }
         } ) ;
         return has ;
      }

      function showPluginNotExist( type, func )
      {
         var has = hasPlugin( type ) ;
         if( has == false )
         {
            var errorInfo = {
               'cmd': '',
               'errno': -10,
               'detail': sprintf( $scope.autoLanguage( '插件不存在: ?' ), type ),
               'description': ''
            } ;
            _IndexPublic.createRetryModel( $scope, errorInfo, function(){
               func() ;
               return true ;
            } ) ;
         }
         return has ;
      }

      function getModuleList( moduleName )
      {
         var list = [] ;
         $.each( $scope.Left.navMenu, function( index, info ){
            if ( info['module'] == moduleName )
            {
               list = info['list'] ;
               return false ;
            }
         } ) ;
         return list ;
      }

      function addBusinessTitle( titles, name )
      {
         var index = -1 ;

         $.each( titles, function( idx, info ){
            if( info['title'] == name )
            {
               index = idx ;
               return false ;
            }
         } ) ;

         if( index == -1 )
         {
            index = titles.length ;

            titles.push( {
               'title': name,
               'list': []
            } ) ;
         }

         return titles[index] ;
      }

      function getNavTitle( type )
      {
         var title = navTitleName[type] ;
         if( typeof( title ) == 'undefined' )
         {
            title = type ;
         }
         return title ;
      }

      function addDataOperation( businessInfo )
      {
         var title = getNavTitle( businessInfo['type'] ) ;
         var titleInfo = addBusinessTitle( getModuleList( 'Data' ), title ) ;
         titleInfo['list'].push( businessInfo ) ;
      }

      function addMonitor( businessInfo )
      {
         if(businessInfo['type'] !== 'sequoiadb' )
         {
            return ;
         }

         var title = getNavTitle( businessInfo['type'] ) ;
         var titleInfo = addBusinessTitle( getModuleList( 'Monitor' ), title ) ;
         titleInfo['list'].push( businessInfo ) ;
      }

      function addStrategy( businessInfo )
      {
         if( businessInfo['type'] == 'sequoiadb' )
         {
            var title = getNavTitle( businessInfo['type'] ) ;
            var titleInfo = addBusinessTitle( getModuleList( 'Strategy' ), title ) ;
            titleInfo['list'].push( businessInfo ) ;
         }
      }

      function addConfig( businessInfo )
      {
         if( businessInfo['type'] == 'sequoiadb' ||
             businessInfo['type'] == 'sequoiasql-postgresql' ||
             businessInfo['type'] == 'sequoiasql-mysql' ||
             businessInfo['type'] == 'sequoiasql-mariadb' )
         {
            var title = getNavTitle( businessInfo['type'] ) ;
            var titleInfo = addBusinessTitle( getModuleList( 'Config' ), title ) ;
            titleInfo['list'].push( businessInfo ) ;
         }
      }

      function addBusiness( businessInfo )
      {
         var thisModule = {
            'name': businessInfo['BusinessName'],
            'type': businessInfo['BusinessType'],
            'mode': businessInfo['DeployMod'],
            'cluster': businessInfo['ClusterName']
         } ;

         if( thisModule['type'] == 'spark' )
         {
            thisModule['href'] = 'http://' + businessInfo['BusinessInfo']['HostName'] + ':' + businessInfo['BusinessInfo']['WebServicePort'] ;
         }

         addDataOperation( thisModule ) ;
         addMonitor( thisModule ) ;
         addStrategy( thisModule ) ;
         addConfig( thisModule ) ;
      }

      function getBusiness()
      {
         var data = { 'cmd': 'query business', 'sort': JSON.stringify( { 'BusinessType': 1, 'BusinessName': 1, 'ClusterName': 1 } ) } ;
         SdbRest.OmOperation( data, {
            'success': function( list ){

               $.each( $scope.Left.navMenu, function( index ){
                  $scope.Left.navMenu[index]['list'] = [] ;
               } ) ;

               $.each( list, function( index, businessInfo ){
                  addBusiness( businessInfo ) ;
               } ) ;

               $scope.cursorIndex = _IndexLeft.getActiveIndex( $rootScope, SdbFunction, $scope.Left.navMenu ) ;
               if( $scope.showModuleIndex == -1 )
               {
                  $scope.showModuleIndex = $scope.cursorIndex[0] ;
               }
               if( $scope.Left.navMenu.length > 1 && $scope.showModuleIndex >=0 && $scope.Left.navMenu[$scope.showModuleIndex]['module'] != 'Deploy' && $scope.Left.nav2Show == false )
               {
                  $scope.Left.nav1Btn = { 'visibility': 'visible' } ;
               }
               else
               {
                  $scope.Left.nav1Btn = { 'visibility': 'hidden' } ;
               }
            }
         }, {
            'delay': 5000,
            'loop': true,
            'scope': false,
            'showLoading': false
         } ) ;
      }

      getPlugins() ;
      getBusiness() ;

      //更新url地址信息
      $rootScope.updateUrl() ;

      function flodNav()
      {
         if( $scope.Left.navMenu.length > 1 && $scope.Left.navMenu[$scope.showModuleIndex]['module'] != 'Deploy' )
         {
            $scope.Left.nav1Btn = { 'visibility': 'visible' } ;
         }
         else
         {
            $scope.Left.nav1Btn = { 'visibility': 'hidden' } ;
         }
         $scope.Left.nav2Btn = { 'visibility': 'hidden' } ;
         $rootScope.layout.left    = { width: 80 } ;
         $rootScope.layout.centre  = { offsetX: -80, marginLeft: 80 } ;
         $scope.Left.nav2 = { width: 0, marginLeft: 0 } ;
         $scope.Left.nav2Show = false ;
      }

      function unflodNav()
      {
         $scope.Left.nav1Btn = { 'visibility': 'hidden' } ;
         $scope.Left.nav2Btn = { 'visibility': 'visible' } ;
         $rootScope.layout.left    = { width: 260 } ;
         $rootScope.layout.centre  = { offsetX: -260, marginLeft: 260 } ;
         $scope.Left.nav1 = { width: 80 } ;
         $scope.Left.nav2 = { width: 180, marginLeft: 80 } ;
         $scope.Left.nav2Show = true ;
      }

      $scope.toggleNav2 = function(){
         if( $scope.Left.nav2Show == true )
         {
            flodNav() ;
         }
         else
         {
            unflodNav() ;
         }
         $rootScope.bindResize() ;
      }

      $rootScope.gotoModule = function( moduleIndex, activeIndex, instanceIndex ){
         var clusterName = $scope.Left.navMenu[ moduleIndex ]['list'][ activeIndex ]['list'][ instanceIndex ]['cluster'] ;
         var moduleName  = $scope.Left.navMenu[ moduleIndex ]['list'][ activeIndex ]['list'][ instanceIndex ]['name'] ;
         var moduleType  = $scope.Left.navMenu[ moduleIndex ]['list'][ activeIndex ]['list'][ instanceIndex ]['type'] ;
         var moduleMode  = $scope.Left.navMenu[ moduleIndex ]['list'][ activeIndex ]['list'][ instanceIndex ]['mode'] ;

         var hasPlugin = showPluginNotExist( moduleType, function(){
            setTimeout( function(){
               $rootScope.gotoModule( moduleIndex, activeIndex, instanceIndex ) ;
            }, 100 ) ;
         } ) ;

         if( hasPlugin == false )
         {
            return ;
         }

         SdbFunction.LocalData( 'SdbClusterName', clusterName ) ;
         SdbFunction.LocalData( 'SdbModuleType', moduleType ) ;
         SdbFunction.LocalData( 'SdbModuleMode', moduleMode ) ;
         SdbFunction.LocalData( 'SdbModuleName', moduleName ) ;

         var params = { 'r': new Date().getTime() } ;

         if( typeof( $scope.Left.navMenu[ moduleIndex ]['target'] ) == 'function' )
         {
            $scope.Left.navMenu[ moduleIndex ]['target']( moduleType ) ;
            $scope.cursorIndex[0] = moduleIndex ;
            $scope.cursorIndex[1] = activeIndex ;
            $scope.cursorIndex[2] = instanceIndex ;
         }

         flodNav() ;
      }

      $rootScope.$on( '$locationChangeStart', function( event, newUrl, oldUrl ){
         printfDebug( '切换路由 ' + newUrl + ' form ' + oldUrl ) ;
         $rootScope.updateUrl() ;
         $scope.cursorIndex = _IndexLeft.getActiveIndex( $rootScope, SdbFunction, $scope.Left.navMenu ) ;
         $scope.showModuleIndex = $scope.cursorIndex[0] ;
         if( $scope.Left.navMenu[ $scope.showModuleIndex ]['module'] == 'Deploy' )
         {
            $scope.Left.nav1Btn = { 'visibility': 'hidden' } ;
            $scope.Left.nav2Btn = { 'visibility': 'hidden' } ;
            $rootScope.layout.left    = { width: 80 } ;
            $rootScope.layout.centre  = { offsetX: -80, marginLeft: 80 } ;
            $scope.Left.nav2 = { width: 0, marginLeft: 0 } ;
            $scope.cursorIndex[1] = -1 ;
            $scope.cursorIndex[2] = -1 ;
         }
         else if( $scope.Left.nav2Show == false )
         {
         }
         else
         {
            $scope.Left.nav1Btn = { 'visibility': 'hidden' } ;
            $scope.Left.nav2Btn = { 'visibility': 'visible' } ;
            $rootScope.layout.left    = { width: 260 } ;
            $rootScope.layout.centre  = { offsetX: -260, marginLeft: 260 } ;
            $scope.Left.nav1 = { width: 80 } ;
            $scope.Left.nav2 = { width: 180, marginLeft: 80 } ;
            $scope.Left.nav2Show = true ;
         }
      } ) ;

      $scope.selectLeftModule = function( moduleIndex ){
         $scope.showModuleIndex = moduleIndex ;
         $rootScope.bindResize() ;
         if( $scope.Left.navMenu[ $scope.showModuleIndex ]['module'] == 'Deploy' )
         {
            flodNav() ;
            try
            {
               $scope.cursorIndex[1] = -1 ;
               $scope.cursorIndex[2] = -1 ;
            }
            catch( e )
            {
            }
         }
         else
         {
            unflodNav() ;
         }
      }
   } ) ;
   //底部
   sacApp.controller( 'Index.Bottom.Ctrl', function( $scope, $interval, SdbRest, SdbSignal ){
      $scope.Bottom = {} ;
      //获取系统时间
      _IndexBottom.getSystemTime( $scope ) ;
      //获取系统状态
      _IndexBottom.checkPing( $scope, $interval, SdbRest ) ;

      function setVersion( info )
      {
         var version = info['Major'] ;
         version = version + '.' + info['Minor'] ;
         if( info['Fix'] > 0 )
         {
            version = version + '.' + info['Fix'] ;
         }
         $scope.Bottom['version'] = version ;
      }

      SdbSignal.on( 'sac_version', function( info ){
         setVersion( info ) ;
      } ) ;

      if( window.Config['recv'] == true )
      {
         setVersion( window.Config['Version'] ) ;
      }
   } ) ;
}());