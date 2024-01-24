
// --------------------- Index ---------------------
var _IndexPublic = {} ;

//创建社区版不支持提示的弹窗
_IndexPublic.createCommunityModel = function( $scope ){
   $scope.Components.Confirm.type = 1 ;
   $scope.Components.Confirm.noOK = false ;
   $scope.Components.Confirm.noClose = true ;
   $scope.Components.Confirm.title = $scope.autoLanguage( '提示' ) ;
   $scope.Components.Confirm.context = $scope.autoLanguage( '社区版不支持这个操作。' ) ;
   $scope.Components.Confirm.isShow = true ;
}

//创建操作不支持提示的弹窗
_IndexPublic.createNotSupportModel = function( $scope ){
   $scope.Components.Confirm.type = 1 ;
   $scope.Components.Confirm.noOK = false ;
   $scope.Components.Confirm.noClose = true ;
   $scope.Components.Confirm.title = $scope.autoLanguage( '提示' ) ;
   $scope.Components.Confirm.context = $scope.autoLanguage( '目前不支持这个操作。' ) ;
   $scope.Components.Confirm.isShow = true ;
}

//检查当前操作是不是不支持的
_IndexPublic.checkEditionAndSupport = function( $scope, moduleType, modelName ){
   var rc = true ;
   /*
   if( window.Config['Edition'] != 'Enterprise' )
   {
      _IndexPublic.createCommunityModel( $scope ) ;
      rc = false ;
   }
   else */
   if( window.Config['Controller'][moduleType][modelName] == false )
   {
      _IndexPublic.createNotSupportModel( $scope ) ;
      rc = false ;
   }
   return rc ;
}

//用于监控页面，社区版就报警告
_IndexPublic.checkMonitorEdition = function( $location ){
   if( window.Config['Edition'] != 'Enterprise' && window.Config['recv'] == true )
   {
      $location.path( '/Monitor/Preview' ).search( { 'r': new Date().getTime() } ) ;
   }
   else if( window.Config['recv'] == false )
   {
      setTimeout( function(){
         _IndexPublic.checkMonitorEdition( $location ) ;
      }, 200 )
   }
}

//创建错误弹窗
_IndexPublic.createErrorModel = function( $scope, context ){
   $scope.Components.Confirm.type = 3 ;
   $scope.Components.Confirm.noOK = true ;
   $scope.Components.Confirm.noClose = true ;
   $scope.Components.Confirm.context = context ;
   $scope.Components.Confirm.isShow = true ;
}

//创建错误提示弹窗
_IndexPublic.ErrorTipsModel = function( $scope, context ){
   $scope.Components.Confirm.type = 3 ;
   $scope.Components.Confirm.noOK = false ;
   $scope.Components.Confirm.okText = $scope.autoLanguage( '确定' ) ;
   $scope.Components.Confirm.noClose = true ;
   $scope.Components.Confirm.context = context ;
   $scope.Components.Confirm.isShow = true ;
}

//关闭错误弹窗
_IndexPublic.closeErrorModel = function( $scope ){
   if( $scope.Components.Confirm.type == 3 )
   {
      $scope.Components.Confirm.isShow = false ;
   }
}

//关闭错误重试弹窗
_IndexPublic.closeRetryModel = function( $scope ){
   if( $scope.Components.Confirm.type == 1 )
   {
      $scope.Components.Confirm.isShow = false ;
   }
}


//创建错误重试弹窗
_IndexPublic.createRetryModel = function( $scope, errorInfo, okFun, title, context, okText, closeText ){

   var defaultError ;

   if( errorInfo )
   {
      if( !errorInfo['cmd'] || errorInfo['cmd'].length == 0 )
      {
         defaultError = sprintf( $scope.autoLanguage( '错误码: ?, ?。需要重试吗?' ), errorInfo['errno'], errorInfo['detail'].length > 0? errorInfo['detail'] : errorInfo['description'] ) ;
      }
      else
      {
         defaultError = sprintf( $scope.autoLanguage( '执行命令: ?, 错误码: ?, ?。需要重试吗?' ), errorInfo['cmd'], errorInfo['errno'], errorInfo['detail'].length > 0? errorInfo['detail'] : errorInfo['description'] ) ;
      }
   }

   $scope.Components.Confirm.isShow = true ;
   $scope.Components.Confirm.type = 1 ;
   $scope.Components.Confirm.noOK = false ;
   $scope.Components.Confirm.noClose = false ;
   $scope.Components.Confirm.title = typeof( title ) == 'string' ? title : $scope.autoLanguage( '获取数据失败' ) ;
   $scope.Components.Confirm.okText = typeof( okText ) == 'string' ? okText : $scope.autoLanguage( '重试' ) ;
   $scope.Components.Confirm.closeText = typeof( closeText ) == 'string' ? closeText : $scope.autoLanguage( '取消' ) ;
   $scope.Components.Confirm.context = typeof( context ) == 'string' ? context : defaultError ;
   $scope.Components.Confirm.ok = function(){
      if( okFun() == true )
      {
         $scope.Components.Confirm.isShow = false ;
      }
   }
}

//创建提示弹窗
_IndexPublic.createInfoModel = function( $scope, context, bt1, bt1Fun ){
   $scope.Components.Confirm.isShow = true ;
   $scope.Components.Confirm.type = 2 ;
   $scope.Components.Confirm.okText = bt1 ;
   $scope.Components.Confirm.closeText = $scope.autoLanguage( '取消' ) ;
   $scope.Components.Confirm.context = context ;
   $scope.Components.Confirm.ok = function(){
      $scope.Components.Confirm.isShow = false ;
      if( typeof( bt1Fun ) == 'function' ) bt1Fun() ;
   }
}

//语言控制器
_IndexPublic.languageCtrl = function( $scope, text ){
   var newText = text ;
   if( $scope.Language == 'en' )
   {
      function setLanguage()
      {
         if( typeof( window.SdbSacLanguage[text] ) == 'undefined' )
         {
            printfDebug( '"' + text + '" 没翻译！' ) ;
         }
         else
         {
            newText = window.SdbSacLanguage[text] ;
         }
      }
      if( typeof( window.SdbSacLanguage ) == 'undefined' )
      {
         window.SdbSacLanguage = {} ;
         //获取语言
         $.ajax( './app/language/English.json', { 'async': false, 'success': function( reqData ){
            window.SdbSacLanguage = JSON.parse( reqData ) ;
            setLanguage() ;
         }, 'error': function( XMLHttpRequest, textStatus, errorThrown ){
            _IndexPublic.createErrorModel( $scope, 'Can not find the language file, please try to refresh your browser by pressing F5.' ) ;
         } } ) ;
      }
      else
      {
         setLanguage() ;
      }
   }
   return newText ;
}

//插件语言控制器
_IndexPublic.pLanguageCtrl = function( $scope, text ){
   var newText = text ;
   var type = localLocalData( 'SdbModuleType' ) ;
   if( type == 'sequoiadb' )
   {
      return text ;
   }
   if( $scope.Language == 'en' )
   {
      function setLanguage()
      {
         if( typeof( window.SdbSacLanguage['_plugins'][type][text] ) == 'undefined' )
         {
            printfDebug( '插件 ' + type + ' "' + text + '" 没翻译！' ) ;
         }
         else
         {
            newText = window.SdbSacLanguage['_plugins'][type][text] ;
         }
      }
      if( typeof( window.SdbSacLanguage ) == 'undefined' )
      {
         window.SdbSacLanguage = { '_plugins' : {} } ;
      }
      if( typeof( window.SdbSacLanguage['_plugins'] ) == 'undefined' )
      {
         window.SdbSacLanguage['_plugins'] = {} ;
      }
      if( typeof( window.SdbSacLanguage['_plugins'][type] ) == 'undefined' )
      {
         window.SdbSacLanguage['_plugins'][type] = {} ;
         //获取语言
         $.ajax( './app/language/English.json', { 'async': false, 'beforeSend': function( jqXHR ){
            var clusterName = localLocalData( 'SdbClusterName' ) ;
	         if( clusterName !== null )
	         {
		         jqXHR.setRequestHeader( 'SdbClusterName', clusterName ) ;
	         }
	         var businessName = localLocalData( 'SdbModuleName' )
	         if( businessName !== null )
	         {
		         jqXHR.setRequestHeader( 'SdbBusinessName', businessName ) ;
	         }
         },'success': function( reqData ){
            window.SdbSacLanguage['_plugins'][type] = JSON.parse( reqData ) ;
            setLanguage() ;
         }, 'error': function( XMLHttpRequest, textStatus, errorThrown ){
            _IndexPublic.createErrorModel( $scope, 'Can not find the language file, please try to refresh your browser by pressing F5.' ) ;
         } } ) ;
      }
      else
      {
         setLanguage() ;
      }
   }
   return newText ;
}

// --------------------- Index.Left ---------------------
var _IndexLeft = {} ;

//激活导航要激活的服务的索引
_IndexLeft.getActiveIndex = function( $rootScope, SdbFunction, navMenu )
{
   var isFind = false ;
   var defaultIndex   = [ -1, -1, -1 ] ;
   var cursorIndex    = [  0, -1, -1 ] ;
   var cursorModule   = $rootScope.Url.Module ;
   var cursorCluster  = SdbFunction.LocalData( 'SdbClusterName' ) ;
   var cursorInstance = SdbFunction.LocalData( 'SdbModuleName' ) ;
   $.each( navMenu, function( key, val ){
      if( val['module'] == cursorModule )
      {
         cursorIndex[0] = key ;
         isFind = true ;
      }
   } ) ;
   if( isFind )
   {
      defaultIndex[0] = cursorIndex[0] ;
   }
   if( typeof( navMenu[ cursorIndex[0] ]['list'] ) != 'undefined' )
   {
      $.each( navMenu[ cursorIndex[0] ]['list'], function( index1, moduleNav ){
         $.each( moduleNav['list'], function( index2, instanceNav ){
            if( defaultIndex[1] == -1 )
            {
               defaultIndex[1] = index1 ;
            }
            if( defaultIndex[2] == -1 )
            {
               defaultIndex[2] = index2 ;
            }
            if( instanceNav['cluster'] == cursorCluster &&
                instanceNav['name'] == cursorInstance )
            {
               cursorIndex[1] = index1 ;
               cursorIndex[2] = index2 ;
               return false ;
            }
         } ) ;
         if( cursorIndex[0] >= 0 && cursorIndex[1] >= 0 && cursorIndex[2] >= 0 )
         {
            return false
         }
      } ) ;
   }
   if( cursorIndex[0] >= 0 && cursorIndex[1] >= 0 && cursorIndex[2] >= 0 )
   {
      return cursorIndex ;
   }
   else
   {
      return defaultIndex ;
   }
}

// --------------------- Index.Bottom ---------------------
var _IndexBottom = {} ;

//获取系统时间
_IndexBottom.getSystemTime = function( $scope )
{
   setInterval( function(){
      var times = $.now() ;
      var date = new Date( times ) ;
      var year = date.getFullYear() ;
      var hour = date.getHours() ;
      var minute = date.getMinutes() ;
      var second = date.getSeconds() ;
      $scope.Bottom.year = year ;
      $scope.Bottom.nowtime = pad( hour, 2 ) + ':' + pad( minute, 2 ) + ':' + pad( second, 2 ) ;
      $scope.$digest();
   }, 1000 ) ;
}

//获取ping值
_IndexBottom.checkPing = function( $scope, $interval, SdbRest )
{
   var isShowErrorModel = false ;
   $interval( function(){
      var status = SdbRest.getNetworkStatus() ;
      if( status == 1 )
      {
         $scope.Bottom.sysStatus = $scope.autoLanguage( '良好' ) ;
         $scope.Bottom.statusColor = 'success' ;
         if( isShowErrorModel )
         {
            _IndexPublic.closeErrorModel( $scope ) ;
         }
      }
      else if( status == 2 )
      {
         $scope.Bottom.sysStatus = $scope.autoLanguage( '网络不稳定' ) ;
         $scope.Bottom.statusColor = 'warning' ;
      }
      else if( status == 3 )
      {
         $scope.Bottom.sysStatus = $scope.autoLanguage( '网络错误' ) ;
         $scope.Bottom.statusColor = 'error' ;
         isShowErrorModel = true ;
         _IndexPublic.createErrorModel( $scope, $scope.autoLanguage( '服务端连接断开，正在尝试重新连接...' ) ) ;
      }
   }, 1000 ) ;
}

// --------------------- Index.Top ---------------------
var _IndexTop = {} ;

//创建修改密码弹窗
_IndexTop.createPasswdModel = function( $scope, SdbRest ){
   $scope.Components.Modal.icon = 'fa-lock' ;
   $scope.Components.Modal.title = $scope.autoLanguage( '修改密码' ) ;
   $scope.Components.Modal.isShow = true ;
   $scope.Components.Modal.form = {
      inputList: [
         {
            "name": "password",
            "webName": $scope.autoLanguage( "密码" ),
            "type": "password",
            "required": true,
            "value": "",
            "valid": {
               "min": 1,
               "max": 1024
            }
         },
         {
            "name": "newPassword",
            "webName": $scope.autoLanguage( "新密码" ),
            "type": "password",
            "required": true,
            "value": "",
            "valid": {
               "min": 1,
               "max": 1024
            }
         }
      ]
   } ;
   $scope.Components.Modal.Context = '<div form-create para="data.form"></div>' ;
   $scope.Components.Modal.ok = function(){
      var isAllClear = $scope.Components.Modal.form.check() ;
      if( isAllClear )
      {
         var value = $scope.Components.Modal.form.getValue() ;
         SdbRest.ChangePasswd( 'admin', value['password'], value['newPassword'], function( json ){
            $scope.Components.Confirm.isShow = true ;
            $scope.Components.Confirm.type = 4 ;
            $scope.Components.Confirm.closeText = $scope.autoLanguage( '取消' ) ;
            $scope.Components.Confirm.context = $scope.autoLanguage( '修改密码成功。' ) ;
            $scope.Components.Confirm.noOK = true ;
         }, function( errorInfo ){
            _IndexPublic.createRetryModel( $scope, errorInfo, function(){
               _IndexTop.createPasswdModel( $scope, SdbRest ) ;
               return true ;
            }, $scope.autoLanguage( '修改密码失败。' ) ) ;
         }, function(){
            //_IndexPublic.createErrorModel( $scope, $scope.autoLanguage( '网络连接错误，请尝试按F5刷新浏览器。' ) ) ;
         }, function(){
            //关闭弹窗
            $scope.Components.Modal.isShow = false ;
         } ) ;
      }
      return isAllClear ;
   }
}

//登出
_IndexTop.logout = function( $location, SdbFunction ){
   //删除会话
   SdbFunction.LocalData( 'SdbSessionID', null ) ;
   //删除用户名
   SdbFunction.LocalData( 'SdbUser', null ) ;
   //删除选中的集群
   SdbFunction.LocalData( 'SdbClusterName', null ) ;
   //删除选中的服务
   SdbFunction.LocalData( 'SdbModuleType', null ) ;
   SdbFunction.LocalData( 'SdbModuleMode', null ) ;
   SdbFunction.LocalData( 'SdbModuleName', null ) ;
   //删除cs
   SdbFunction.LocalData( 'SdbCsName', null ) ;
   //删除cl
   SdbFunction.LocalData( 'SdbClType', null ) ;
   SdbFunction.LocalData( 'SdbClName', null ) ;
   window.location.href = '/login.html' ;
}

// --------------------- Deploy ---------------------

var _Deploy = {} ;

_Deploy.GotoStep = function( $location, action ){
   $location.path( '/Deploy/' + action ).search( { 'r': new Date().getTime() } ) ;
}

//生成部署步骤图
_Deploy.BuildSdbStep = function( $scope, $location, deployModel, action, deployModule ){
   var stepList = {
      'step': 0,
      'info': [] 
   } ;
   switch( deployModel )
   {
   case 'Host':
   {
      switch( action )
      {
      case 'ScanHost':
         stepList['step'] = 1 ;
         break ;
      case 'AddHost':
         stepList['step'] = 2 ;
         break ;
      case 'Host':
         stepList['step'] = 3 ;
         break ;
      }
      stepList['info'].push( { 'text': $scope.autoLanguage( '扫描主机' ), 'click': function(){ _Deploy.GotoStep( $location, 'ScanHost' ); } } ) ;
      stepList['info'].push( { 'text': $scope.autoLanguage( '检查主机' ), 'click': function(){ _Deploy.GotoStep( $location, 'AddHost'  ); } } ) ;
      stepList['info'].push( { 'text': $scope.autoLanguage( '安装主机' ), 'click': function(){ _Deploy.GotoStep( $location, 'Task/Host'  ); } } ) ;
      break ;
   }
   case 'Module':
   {
      switch( action )
      {
      case 'SDB-Conf':
      case 'SSQL-Conf':
         stepList['step'] = 1 ;
         break ;
      case 'SDB-Mod':
      case 'SSQL-Mod':
         stepList['step'] = 2 ;
         break ;
      case 'ZKP-Mod':
         stepList['step'] = 1 ;
         break ;
      case 'Module':
         if( deployModule == 'sequoiadb' || deployModule == 'sequoiasql' )
         {
            stepList['step'] = 3 ;
         }
         else if( deployModule == 'zookeeper' )
         {
            stepList['step'] = 2 ;
         }
         break ;
      }
      if( deployModule == 'sequoiadb' )
      {
         stepList['info'].push( { 'text': $scope.autoLanguage( '配置服务' ), 'click': function(){ _Deploy.GotoStep( $location, 'SDB-Conf' ); } } ) ;
         stepList['info'].push( { 'text': $scope.autoLanguage( '修改服务' ), 'click': function(){ _Deploy.GotoStep( $location, 'SDB-Mod'  ); } } ) ;
      }
      else if( deployModule == 'sequoiasql' )
      {
         stepList['info'].push( { 'text': $scope.autoLanguage( '配置服务' ), 'click': function(){ _Deploy.GotoStep( $location, 'SSQL-Conf' ); } } ) ;
         stepList['info'].push( { 'text': $scope.autoLanguage( '修改服务' ), 'click': function(){ _Deploy.GotoStep( $location, 'SSQL-Mod'  ); } } ) ;
      }
      else if( deployModule == 'zookeeper' )
      {
         stepList['info'].push( { 'text': $scope.autoLanguage( '修改服务' ), 'click': function(){ _Deploy.GotoStep( $location, 'ZKP-Mod'  ); } } ) ;
      }
      stepList['info'].push( { 'text': $scope.autoLanguage( '安装服务' ), 'click': function(){ _Deploy.GotoStep( $location, 'Task/Module'  ); } } ) ;
      break ;
   }
   case 'Deploy':
   {
      switch( action )
      {
      case 'ScanHost':
         stepList['step'] = 1 ;
         break ;
      case 'AddHost':
         stepList['step'] = 2 ;
         break ;
      case 'Host':
         stepList['step'] = 3 ;
         break ;
      case 'SDB-Conf':
      case 'SSQL-Conf':
         stepList['step'] = 4 ;
         break ;
      case 'SDB-Mod':
      case 'SSQL-Mod':
         stepList['step'] = 5 ;
         break ;
      case 'Module':
         if( deployModule == 'zookeeper' )
         {
            stepList['step'] = 5 ;
         }
         else
         {
            stepList['step'] = 6 ;
         }
         break ;
      }
      stepList['info'].push( { 'text': $scope.autoLanguage( '扫描主机' ), 'click': function(){ _Deploy.GotoStep( $location, 'ScanHost' ); } } ) ;
      stepList['info'].push( { 'text': $scope.autoLanguage( '检查主机' ), 'click': function(){ _Deploy.GotoStep( $location, 'AddHost'  ); } } ) ;
      stepList['info'].push( { 'text': $scope.autoLanguage( '安装主机' ), 'click': function(){ _Deploy.GotoStep( $location, 'Task/Host'  ); } } ) ;
      if( deployModule == 'sequoiadb' )
      {
         stepList['info'].push( { 'text': $scope.autoLanguage( '配置服务' ), 'click': function(){ _Deploy.GotoStep( $location, 'SDB-Conf' ); } } ) ;
         stepList['info'].push( { 'text': $scope.autoLanguage( '修改服务' ), 'click': function(){ _Deploy.GotoStep( $location, 'SDB-Mod'  ); } } ) ;
      }
      else if( deployModule == 'sequoiasql' )
      {
         stepList['info'].push( { 'text': $scope.autoLanguage( '配置服务' ), 'click': function(){ _Deploy.GotoStep( $location, 'SSQL-Conf' ); } } ) ;
         stepList['info'].push( { 'text': $scope.autoLanguage( '修改服务' ), 'click': function(){ _Deploy.GotoStep( $location, 'SSQL-Mod'  ); } } ) ;
      }
      else if( deployModule == 'zookeeper' )
      {
         stepList['info'].push( { 'text': $scope.autoLanguage( '修改服务' ), 'click': function(){ _Deploy.GotoStep( $location, 'ZKP-Mod'  ); } } ) ;
      }
      stepList['info'].push( { 'text': $scope.autoLanguage( '安装服务' ), 'click': function(){ _Deploy.GotoStep( $location, 'Task/Module'  ); } } ) ;
      break ;
   }
   }
   return stepList ;
}

//生成扩容步骤图
_Deploy.BuildSdbExtStep = function( $scope, $location, action, deployModule ){
   var stepList = {
      'step': 0,
      'info': [] 
   } ;

   switch( action )
   {
   case 'SDB-ExtendConf':
      stepList['step'] = 1 ;
      break ;
   case 'SDB-Extend':
      stepList['step'] = 2 ;
      break ;
   case 'SDB-ExtendInstall':
      stepList['step'] = 3 ;
      break ;
   }
   if( deployModule == 'sequoiadb' )
   {
      stepList['info'].push( { 'text': $scope.autoLanguage( '扩容配置' ), 'click': function(){ _Deploy.GotoStep( $location, 'SDB-ExtendConf' ); } } ) ;
      stepList['info'].push( { 'text': $scope.autoLanguage( '修改服务' ), 'click': function(){ _Deploy.GotoStep( $location, 'SDB-Extend'  ); } } ) ;
   }
   stepList['info'].push( { 'text': $scope.autoLanguage( '安装服务' ), 'click': function(){ _Deploy.GotoStep( $location, 'SDB-ExtendInstall'  ); } } ) ;

   return stepList ;
}

//生成安装主机-发现服务步骤图
_Deploy.BuildDiscoverStep = function( $scope, $location, action, deployModule ){
   var stepList = {
      'step': 0,
      'info': [] 
   } ;

   switch( action )
   {
   case 'ScanHost':
      stepList['step'] = 1 ;
      break ;
   case 'AddHost':
      stepList['step'] = 2 ;
      break ;
   case 'Host':
      stepList['step'] = 3 ;
      break ;
   case 'SDB-Discover':
   case 'MYSQL-Discover':
      stepList['step'] = 4 ;
      break ;
   }
   stepList['info'].push( { 'text': $scope.autoLanguage( '扫描主机' ), 'click': function(){ _Deploy.GotoStep( $location, 'ScanHost') ; } } ) ;
   stepList['info'].push( { 'text': $scope.autoLanguage( '检查主机' ), 'click': function(){ _Deploy.GotoStep( $location, 'AddHost' ) ; } } ) ;
   stepList['info'].push( { 'text': $scope.autoLanguage( '安装主机' ), 'click': function(){ _Deploy.GotoStep( $location, 'Task/Host' ) ; } } ) ;

   if( deployModule == 'sequoiadb' )
   {
      stepList['info'].push( { 'text': $scope.autoLanguage( '发现服务' ), 'click': function(){ _Deploy.GotoStep( $location, 'SDB-Discover'  ) ; } } ) ;
   }
   else if( deployModule == 'sequoiasql-mysql' )
   {
      stepList['info'].push( { 'text': $scope.autoLanguage( '发现服务' ), 'click': function(){ _Deploy.GotoStep( $location, 'MYSQL-Discover'  ) ; } } ) ;
   }
   return stepList ;
}

//生成安装主机-同步服务步骤图
_Deploy.BuildSyncStep = function( $scope, $location, action, deployModule ){
   var stepList = {
      'step': 0,
      'info': [] 
   } ;

   switch( action )
   {
   case 'ScanHost':
      stepList['step'] = 1 ;
      break ;
   case 'AddHost':
      stepList['step'] = 2 ;
      break ;
   case 'Host':
      stepList['step'] = 3 ;
      break ;
   case 'SDB-Sync':
      stepList['step'] = 4 ;
      break ;
   }
   stepList['info'].push( { 'text': $scope.autoLanguage( '扫描主机' ), 'click': function(){ _Deploy.GotoStep( $location, 'ScanHost') ; } } ) ;
   stepList['info'].push( { 'text': $scope.autoLanguage( '检查主机' ), 'click': function(){ _Deploy.GotoStep( $location, 'AddHost' ) ; } } ) ;
   stepList['info'].push( { 'text': $scope.autoLanguage( '安装主机' ), 'click': function(){ _Deploy.GotoStep( $location, 'Task/Host' ) ; } } ) ;

   if( deployModule == 'sequoiadb' )
   {
      stepList['info'].push( { 'text': $scope.autoLanguage( '同步服务' ), 'click': function(){ _Deploy.GotoStep( $location, 'SDB-Sync'  ) ; } } ) ;
   }
   return stepList ;
}

//生成服务减容步骤图
_Deploy.BuildSdbShrinkStep = function( $scope, $location, action, deployModule ){
   var stepList = {
      'step': 0,
      'info': [] 
   } ;
   switch( action )
   {
   case 'SDB-ShrinkConf':
      stepList['step'] = 1 ;
      break ;
   case 'Module':
      stepList['step'] = 2 ;
      break ;
   }
   stepList['info'].push( { 'text': $scope.autoLanguage( '减容配置' ), 'click': function(){ _Deploy.GotoStep( $location, 'SDB-ShrinkConf') ; } } ) ;
   stepList['info'].push( { 'text': $scope.autoLanguage( '服务减容' ), 'click': function(){ _Deploy.GotoStep( $location, 'Task/Module'  ) ; } } ) ;
   return stepList ;
}

//生成安装sequoiasql-postgresql步骤图
_Deploy.BuildSdbPgsqlStep = function( $scope, $location, action, deployModule ){
   var stepList = {
      'step': 0,
      'info': []
   }
   switch( action )
   {
   case 'PostgreSQL-Mod':
      stepList['step'] = 1 ;
      break ;
   case 'Module':
      stepList['step'] = 2 ;
      break ;
   }
   stepList['info'].push( { 'text': $scope.autoLanguage( '配置实例' ), 'click': function(){ _Deploy.GotoStep( $location, 'PostgreSQL-Mod') ; } } ) ;
   stepList['info'].push( { 'text': $scope.autoLanguage( '创建实例' ), 'click': function(){ _Deploy.GotoStep( $location, 'Task/Module'  ) ; } } ) ;
   return stepList ;
}

//生成安装sequoiasql-mysql步骤图
_Deploy.BuildSdbMysqlStep = function( $scope, $location, action, deployModule ){
   var stepList = {
      'step': 0,
      'info': []
   }
   switch( action )
   {
   case 'MySQL-Mod':
      stepList['step'] = 1 ;
      break ;
   case 'Module':
      stepList['step'] = 2 ;
      break ;
   }
   stepList['info'].push( { 'text': $scope.autoLanguage( '配置实例' ), 'click': function(){ _Deploy.GotoStep( $location, 'MySQL-Mod') ; } } ) ;
   stepList['info'].push( { 'text': $scope.autoLanguage( '创建实例' ), 'click': function(){ _Deploy.GotoStep( $location, 'Task/Module'  ) ; } } ) ;
   return stepList ;
}

//生成部署安装包步骤图
_Deploy.BuildDeployPackageStep = function( $scope, $location, action, deployModule ){
   var stepList = {
      'step': 0,
      'info': []
   }
   switch( action )
   {
   case 'Package':
      stepList['step'] = 1 ;
      break ;
   case 'Host':
      stepList['step'] = 2 ;
      break ;
   }
   stepList['info'].push( { 'text': $scope.autoLanguage( '配置' ), 'click': function(){ _Deploy.GotoStep( $location, 'Package') ; } } ) ;
   stepList['info'].push( { 'text': $scope.autoLanguage( '部署' ), 'click': function(){ _Deploy.GotoStep( $location, 'Task/Host'  ) ; } } ) ;
   return stepList ;
}

//参数模板转换
_Deploy.ConvertTemplate = function( templateList, level, canEmpty, checkConfType, showName ){
   var setLevel = 0 ;
   if( typeof( level ) != 'undefined' )
   {
      setLevel = level ;
   }
   if( typeof( showName ) == 'undefined' )
   {
      showName = false ;
   }
   var newTemplateList = [] ;
   $.each( templateList, function( index, templateInfo ){
      if( typeof( templateInfo['Name'] ) == 'string' )
      {
         if( setLevel != templateInfo['Level'] )
         {
            return true ;
         }
         var newTemplateInfo = {
            'name':     templateInfo['Name'],
            'showName': showName,
            'value':    isUndefined( templateInfo['Default'] ) ? '' : templateInfo['Default'],
            'webName':  templateInfo['WebName'],
            'disabled': templateInfo['Edit'] == "false" ? true : false,
            'desc':     templateInfo['Desc'],
            'type':     '',
            'valid':    ''
         } ;
         if( checkConfType == true )
         {
            newTemplateInfo['confType'] = templateInfo['ConfType'] ;
         }

         if( templateInfo['Display'] == 'select box' )
         {
            newTemplateInfo['type'] = 'select' ;
            newTemplateInfo['valid'] = [] ;
            var validArr = templateInfo['Valid'].split( ',' ) ;
            $.each( validArr, function( index2 ){
               newTemplateInfo['valid'].push( { 'key': validArr[index2], 'value': validArr[index2] } ) ;
            } ) ;
         }
         else if( templateInfo['Display'] == 'edit box' )
         {
            if( templateInfo['Type'] == 'int' )
            {
               newTemplateInfo['type'] = 'int' ;
               newTemplateInfo['valid'] = {} ;
               var pos1 = templateInfo['Valid'].indexOf( '-' ) ;
               if( templateInfo['Valid'] !== '' && pos1 !== -1 )
			      {
                  var minValue ;
                  var maxValue ;
                  var pos2 = templateInfo['Valid'].indexOf( '-', pos1 + 1 ) ;
                  if( pos2 == -1 )
                  {
                     var splitValue = templateInfo['Valid'].split( '-' ) ;
                     minValue = splitValue[0] ;
                     maxValue = splitValue[1] ;
                  }
                  else
                  {
                     minValue = templateInfo['Valid'].substr( 0, pos2 ) ;
                     maxValue = templateInfo['Valid'].substr( pos2 + 1 ) ;
                  }
                  if( isNaN( minValue ) == false )
                  {
                     newTemplateInfo['valid']['min'] = parseInt( minValue ) ;
                  }
                  if( isNaN( maxValue ) == false )
                  {
                     newTemplateInfo['valid']['max'] = parseInt( maxValue ) ;
                  }
               }
               else if ( canEmpty === true )
               {
                  newTemplateInfo['valid']['empty'] = true ;
               }
            }
            else if( templateInfo['Type'] == 'double' )
            {
               newTemplateInfo['type'] = 'double' ;
               newTemplateInfo['valid'] = {} ;
               var pos1 = templateInfo['Valid'].indexOf( '-' ) ;
               if( templateInfo['Valid'] !== '' && pos1 !== -1 )
			      {
                  var minValue ;
				      var maxValue ;
                  var pos2 = templateInfo['Valid'].indexOf( '-', pos1 + 1 ) ;
                  if( pos2 == -1 )
                  {
				         var splitValue = templateInfo['Valid'].split( '-' ) ;
				         minValue = splitValue[0] ;
				         maxValue = splitValue[1] ;
                  }
                  else
                  {
                     minValue = templateInfo['Valid'].substr( 0, pos2 ) ;
                     maxValue = templateInfo['Valid'].substr( pos2 + 1 ) ;
                  }
                  if( isNaN( minValue ) == false )
                  {
                     newTemplateInfo['valid']['min'] = parseFloat( minValue ) ;
                  }
                  if( isNaN( maxValue ) == false )
                  {
                     newTemplateInfo['valid']['max'] = parseFloat( maxValue ) ;
                  }
			      }
               else if ( canEmpty === true )
               {
                  newTemplateInfo['valid']['empty'] = true ;
               }
            }
            else if( templateInfo['Type'] === 'port' )
		      {
               newTemplateInfo['type'] = 'port' ;
               newTemplateInfo['valid'] = {} ;
               if( templateInfo['Valid'] !== '' && templateInfo['Valid'].indexOf('-') !== -1 )
			      {
				      var splitValue = templateInfo['Valid'].split( '-' ) ;
				      var minValue = splitValue[0] ;
				      var maxValue = splitValue[1] ;
				      if( isNaN( minValue ) == false )
                  {
                     newTemplateInfo['valid']['min'] = parseInt( minValue ) ;
                  }
                  if( isNaN( maxValue ) == false )
                  {
                     newTemplateInfo['valid']['max'] = parseInt( maxValue ) ;
                  }
			      }
               else
               {
                  newTemplateInfo['valid']['empty'] = true ;
               }
		      }
            else if( templateInfo['Type'] === 'string' )
		      {
               newTemplateInfo['type'] = 'string' ;
               newTemplateInfo['valid'] = {} ;
               if( templateInfo['Valid'] !== '' && templateInfo['Valid'].indexOf('-') !== -1 )
			      {
				      var splitValue = templateInfo['Valid'].split( '-' ) ;
				      var minValue = splitValue[0] ;
				      var maxValue = splitValue[1] ;
				      if( isNaN( minValue ) == false )
                  {
                     newTemplateInfo['valid']['min'] = parseInt( minValue ) ;
                  }
                  if( isNaN( maxValue ) == false )
                  {
                     newTemplateInfo['valid']['max'] = parseInt( maxValue ) ;
                  }
			      }
		      }
         }
         else if( templateInfo['Display'] == 'text box' )
         {
            newTemplateInfo['type'] = 'text' ;
            if( templateInfo['Type'] == 'path' )
            {
               newTemplateInfo['valid'] = {
                  'min': 1
               } ;
            }
         }
         if( templateInfo['Display'] == 'hidden' )
         {
            return true ;
         }
         newTemplateList.push( newTemplateInfo ) ;
      }
      else
      {
         newTemplateList.push( templateInfo ) ;
      }
   } ) ;
   return newTemplateList ;
}

/*
   计算cpu性能百分比
   hostList 主机性能列表
   oldCpu   上一次汇总的cpu性能
*/
function getCpuUsePercent( hostList, oldCpu )
{
   var cpu = {
      'Idle': {
         'Megabit': 0, 
         'Unit': 0
      },
      'Sys': {
         'Megabit': 0, 
         'Unit': 0
      }, 
      'Other': {
         'Megabit': 0, 
         'Unit': 0
      }, 
      'User': {
         'Megabit': 0, 
         'Unit': 0
      }
   } ;
   var isError = false ;
   //把所有主机的cpu对应属性累加
   $.each( hostList, function( index, hostInfo ){
      if( isNaN( hostInfo['errno'] ) == false && hostInfo['errno'] != 0 )
      {
         isError = true ;
         return true ;
      }
      cpu['Idle']['Megabit']  += hostInfo['CPU']['Idle']['Megabit'] ;
      cpu['Idle']['Unit']     += hostInfo['CPU']['Idle']['Unit'] ;
      cpu['Sys']['Megabit']   += hostInfo['CPU']['Sys']['Megabit'] ;
      cpu['Sys']['Unit']      += hostInfo['CPU']['Sys']['Unit'] ;
      cpu['Other']['Megabit'] += hostInfo['CPU']['Other']['Megabit'] ;
      cpu['Other']['Unit']    += hostInfo['CPU']['Other']['Unit'] ;
      cpu['User']['Megabit']  += hostInfo['CPU']['User']['Megabit'] ;
      cpu['User']['Unit']     += hostInfo['CPU']['User']['Unit'] ;
   } ) ;
   if( typeof( oldCpu ) == 'object' && oldCpu !== null )
   {
      //得出cpu总资源
      var newCpuSum = {
         'Megabit': cpu['Idle']['Megabit'] + cpu['Sys']['Megabit'] + cpu['Other']['Megabit'] + cpu['User']['Megabit'],
         'Unit': cpu['Idle']['Unit'] + cpu['Sys']['Unit'] + cpu['Other']['Unit'] + cpu['User']['Unit']
      } ;
      //得出上一次的cpu总资源
      var oldCpuSum = {
         'Megabit': oldCpu['Idle']['Megabit'] + oldCpu['Sys']['Megabit'] + oldCpu['Other']['Megabit'] + oldCpu['User']['Megabit'],
         'Unit': oldCpu['Idle']['Unit'] + oldCpu['Sys']['Unit'] + oldCpu['Other']['Unit'] + oldCpu['User']['Unit']
      } ;
      //计算出idle资源差值
      var idleDiff = ( cpu['Idle']['Megabit'] - oldCpu['Idle']['Megabit'] ) * 1024 + ( cpu['Idle']['Unit'] - oldCpu['Idle']['Unit'] ) / 1024 ;
      //计算出总资源差值
      var sumDiff  = ( newCpuSum['Megabit'] - oldCpuSum['Megabit'] ) * 1024 + ( newCpuSum['Unit'] - oldCpuSum['Unit'] ) / 1024 ;
      //计算出cpu使用率
      var percent = fixedNumber( ( 1 - idleDiff / sumDiff ) * 100, 2 ) ;
      oldCpu['Idle']  = cpu['Idle'] ;
      oldCpu['Sys']   = cpu['Sys'] ;
      oldCpu['Other'] = cpu['Other'] ;
      oldCpu['User']  = cpu['User'] ;
      if( isError )
         percent = 0 ;
      return percent ;
   }
   else
   {
      return cpu ;
   }
}

/*
   计算内存占用百分比
   hostList 主机性能列表
*/
function getMemoryUsePercent( hostList )
{
   var used = 0 ;
   var size = 0 ;
   var hostNum = 0 ;
   $.each( hostList, function( index, hostInfo ){
      if( isNaN( hostInfo['errno'] ) == false && hostInfo['errno'] != 0 )
      {
         return true ;
      }
      ++hostNum ;
      used += hostInfo['Memory']['Used'] ;
      size += hostInfo['Memory']['Size'] ;
   } ) ;
   if( size == 0 )
      return 0 ;
   var percent = fixedNumber( used / size * 100, 2 ) ;
   return percent ;
}

/*
   计算硬盘占用百分比
   hostList 主机性能列表
*/
function getDiskUsePercent( hostList )
{
   var used = 0 ;
   var size = 0 ;
   var diskNum = 0 ;
   $.each( hostList, function( index, hostInfo ){
      if( isNaN( hostInfo['errno'] ) == false && hostInfo['errno'] != 0 )
      {
         return true ;
      }
      $.each( hostInfo['Disk'], function( index2, diskInfo ){
         ++diskNum ;
         used += diskInfo['Size'] - diskInfo['Free'] ;
         size += diskInfo['Size'] ;
      } ) ;
   } ) ;
   if( size == 0 )
      return 0 ;
   var percent = fixedNumber( used / size * 100, 2 ) ;
   return percent ;
}