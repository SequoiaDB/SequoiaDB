//"use strict" ;
(function(){
   var sacApp = window.SdbSacManagerModule ;
   sacApp.service( 'SdbRest', function( $q, $rootScope, $location, Loading, SdbFunction ){
      var g = this ;
      function restBeforeSend( jqXHR )
      {
	      var id = SdbFunction.LocalData( 'SdbSessionID' ) ;
	      if( id !== null )
	      {
		      jqXHR.setRequestHeader( 'SdbSessionID', id ) ;
	      }
	      var language = SdbFunction.LocalData( 'SdbLanguage' )
	      if( language !== null )
	      {
		      jqXHR.setRequestHeader( 'SdbLanguage', language ) ;
	      }
      }

      //网络状态
      var NORMAL     = 1 ;
      var INSTABLE   = 2 ;
      var ERROR      = 3 ;

      //网络当前状态
      g._status = NORMAL ;

      //网络连接错误的次数
      g._errorNum = 0 ;

      //记录网络错误之后，所有的请求
      g._errorTask = [] ;

      //第一个网络错误的请求的命令
      g._lastErrorTaskCmd = '' ;

      //第一个网络错误的请求的错误事件
      g._lastErrorTask = null ;

      //获取网络状态
      g.getNetworkStatus = function(){
         return g._status ;
      }

      //(即将废弃)
      g._lastErrorEvent = null ;

      //执行器状态(即将废弃)
      var IDLE = 0
      var RUNNING = 1 ;

      //执行器当前状态(即将废弃)
      g._runStatus = IDLE ;

      //网络请求队列(即将废弃)
      g._queue = [] ;

      //执行循环模块(即将废弃)
      g._execLoop = function( task, loop ){
         if( task['scope'] === false || task['scope'] === $location.url() )
         {
            if( task['loop'] === loop )
            {
               if( task['delay'] > 0 )
               {
                  setTimeout( function(){
                     g._queue.push( task ) ;
                  }, task['delay'] ) ;
               }
               else
               {
                  g._queue.push( task ) ;
               }
            }
         }
      }

      //网络执行器(即将废弃)
      g._run = function(){
         g._runStatus = RUNNING ;
         if( g._status == NORMAL )
         {
            if( g._queue.length > 0 )
            {
               var task = g._queue.shift() ;
               if( task['scope'] !== false && task['scope'] !== $location.url() )
               {
                  //作用域不是全页面 并且 已经切换页面了
                  setTimeout( g._run, 1 ) ;
                  return ;
               }
               if( typeof( task['init'] ) == 'function' )
               {
                  var value = task['init']() ;
                  if( typeof( value ) !== 'undefined' )
                  {
                     task['data'] = value ;
                  }
               }
               g._post( task['type'], task['url'], task['data'], task['before'],
                        function( json, textStatus, jqXHR ){
                           g._errorNum = 0 ;
                           g._status = NORMAL ;
                           if( typeof( task['success'] ) == 'function' )
                           {
                              task['success']( json, textStatus, jqXHR ) ;
                           }
                           g._execLoop( task, 'success' ) ;
                        },
                        function( errInfo ){
                           g._errorNum = 0 ;
                           g._status = NORMAL ;
                           if( typeof( task['failed'] ) == 'function' )
                           {
                              (function( task ){
                                 task['failed']( errInfo, function(){
                                    g._queue.push( task ) ;
                                 } ) ;
                              }( task )) ;
                           }
                           g._execLoop( task, 'failed' ) ;
                        },
                        function( XMLHttpRequest, textStatus, errorThrown ){
                           ++g._errorNum ;
                           g._status = INSTABLE ;
                           if( typeof( task['error'] ) == 'function' )
                           {
                              task['error']( XMLHttpRequest, textStatus, errorThrown ) ;
                           }
                           //g._queue.unshift( task ) ;
                           g._lastErrorEvent = task['failed'] ;
                        },
                        function( XMLHttpRequest, textStatus ){
                           if( typeof( task['complete'] ) == 'function' )
                           {
                              task['complete']( XMLHttpRequest, textStatus ) ;
                           }
                           g._execLoop( task, true ) ;
                           task = null ;
                        },
                        task['showLoading'], task['errJson'] ) ;
               setTimeout( g._run, 1 ) ;
            }
            else
            {
               setTimeout( g._run, 100 ) ;
            }
         }
         else
         {
            g.getPing( function( times ){
               if( times >= 0 )
               {
                  g._errorNum = 0 ;
                  g._status = NORMAL ;
                  if( typeof( g._lastErrorEvent ) == 'function' )
                  {
                     g._lastErrorEvent( { "cmd": '', "errno": -15, "description": "Network error", "detail": "Network error, request status unknown." } ) ;
                     g._lastErrorEvent = null ;
                  }
               }
               else
               {
                  ++g._errorNum ;
                  if( g._errorNum >= 10 )
                  {
                     g._status = ERROR ;
                  }
               }
               setTimeout( g._run, 1000 ) ;
            } ) ;
         }
      }

      /*(即将废弃)
         网络调度器
         type: POST,GET
         url: 路径
         data: post的数据
         event: 事件
               init     初始化      如果init的返回值不是undefined，将会代替data的值
               before   post前
               success  成功
               failed   失败
               error    错误
               complete 完成
         options 选项
               delay: 延迟多少毫秒
               loop: 循环; 'success':执行成功时循环 'failed':执行失败时循环 true:成功失败都循环 false:不循环, 默认是false
               scope: 作用域; false:所有页面  true:当前页面, 默认是true
               showLoading:  显示Loading动画; true:显示, false:不显示, 默认是false

      */
      g._insert = function( type, url, data, event, options, errJson ){
         if( typeof( options ) != 'object' || options === null )
         {
            options = {} ;
         }
         if( typeof( options['delay'] ) != 'number' )
         {
            options['delay'] = 0 ;
         }
         if( typeof( options['scope'] ) == 'undefined' )
         {
            options['scope'] = $location.url() ;
         }
         else
         {
            options['scope'] = options['scope'] ? $location.url() : false ;
         }
         var task = $.extend( {}, { 'type': type, 'url': url, 'data': data, 'errJson': errJson }, event, options ) ;
         if( task['delay'] > 0 && !task['loop'] )
         {
            (function( task ){
               setTimeout( function(){
                  g._queue.push( task ) ;
                  task = null ;
               }, task['delay'] ) ;
            }( task ) ) ;
         }
         else
         {
            g._queue.push( task ) ;
            task = null ;
         }
         if( g._runStatus == IDLE )
         {
            g._run() ;
         }
      }

      //发送请求(即将废弃)
      g._post = function( type, url, data, before, success, failed, error, complete, showLoading, errJson ){
         if( typeof( showLoading ) == 'undefined' ) showLoading = true ;
         if( showLoading )
         {
            Loading.create() ;
         }
         var cmd = '' ;
         if( typeof( data['cmd'] ) == 'string' )
         {
            cmd = data['cmd'] ;
         }
         $.ajax( { 'type': type, 'url': url, 'data': data, 'success': function( json, textStatus, jqXHR ){
            json = trim( json ) ;
            if( json.length == 0 && typeof( failed ) === 'function' )
            {
               //收到响应，但是没有任何数据
               try
               {
                  failed( { "errno": -10, "description": "System error", "detail": "No rest response data.", "cmd": cmd } ) ;
               }
               catch( e )
               {
                  printfDebug( e.stack ) ;
                  $rootScope.Components.Confirm.isShow = true ;
                  $rootScope.Components.Confirm.type = 1 ;
                  $rootScope.Components.Confirm.title = 'System error' ;
                  $rootScope.Components.Confirm.context = 'Javascript error: ' + e.message ;
                  $rootScope.Components.Confirm.ok = function(){
                     Loading.close() ;
                     $rootScope.Components.Confirm.isShow = false ;
                  }
               }
            }
            else
            {
               var jsonArr = g._parseJsons( json, errJson ) ;
               if( jsonArr.length == 0 )
               {
                  //有数据，但是没有记录，理论上不会发生
                  if( typeof( failed ) === 'function' )
                  {
                     try
                     {
                        failed( { "errno": -10, "description": "System error", "detail": "Rest response data error.", "cmd": cmd } ) ;
                     }
                     catch( e )
                     {
                        printfDebug( e.stack ) ;
                        $rootScope.Components.Confirm.isShow = true ;
                        $rootScope.Components.Confirm.type = 1 ;
                        $rootScope.Components.Confirm.title = 'System error' ;
                        $rootScope.Components.Confirm.context = 'Javascript error: ' + e.message ;
                        $rootScope.Components.Confirm.ok = function(){
                           Loading.close() ;
                           $rootScope.Components.Confirm.isShow = false ;
                        }
                     }
                  }
               }
               else if( jsonArr[0]['errno'] === 0 && typeof( success ) == 'function' )
               {
                  if( isArray( errJson ) )
                  {
                     errJson.splice( 0, 1 ) ;
                  }
                  jsonArr.splice( 0, 1 ) ;
                  try
                  {
                     success( jsonArr, textStatus, jqXHR ) ;
                  }
                  catch( e )
                  {
                     if( window.SdbDebug === true )
                     {
                        success( jsonArr, textStatus, jqXHR ) ;
                     }
                     else
                     {
                        printfDebug( e.stack ) ;
                        $rootScope.Components.Confirm.isShow = true ;
                        $rootScope.Components.Confirm.type = 1 ;
                        $rootScope.Components.Confirm.title = 'System error' ;
                        $rootScope.Components.Confirm.context = 'Javascript error: ' + e.message ;
                        $rootScope.Components.Confirm.ok = function(){
                           Loading.close() ;
                           $rootScope.Components.Confirm.isShow = false ;
                        }
                     }
                  }
               }
               else if( jsonArr[0]['errno'] === -62 )
               {
                  //session id 不存在
                  window.location.href = './login.html#/Login' ;
               }
               else if( typeof( failed ) === 'function' )
               {
                  //其他错误
                  jsonArr[0]['cmd'] = cmd ;
                  try
                  {
                     failed( jsonArr[0] ) ;
                  }
                  catch( e )
                  {
                     if( window.SdbDebug === true )
                     {
                        failed( jsonArr[0] ) ;
                     }
                     else
                     {
                        printfDebug( e.stack ) ;
                        $rootScope.Components.Confirm.isShow = true ;
                        $rootScope.Components.Confirm.type = 1 ;
                        $rootScope.Components.Confirm.title = 'System error' ;
                        $rootScope.Components.Confirm.context = 'Javascript error: ' + e.message ;
                        $rootScope.Components.Confirm.ok = function(){
                           Loading.close() ;
                           $rootScope.Components.Confirm.isShow = false ;
                        }
                     }
                  }
               }
            }
         }, 'error': function( XMLHttpRequest, textStatus, errorThrown ) {
            if( typeof( error ) === 'function' )
            {
               try
               {
                  error( XMLHttpRequest, textStatus, errorThrown ) ;
               }
               catch( e )
               {
                  printfDebug( e.stack ) ;
                  $rootScope.Components.Confirm.isShow = true ;
                  $rootScope.Components.Confirm.type = 1 ;
                  $rootScope.Components.Confirm.title = 'System error' ;
                  $rootScope.Components.Confirm.context = 'Javascript error: ' + e.message ;
                  $rootScope.Components.Confirm.ok = function(){
                     Loading.close() ;
                     $rootScope.Components.Confirm.isShow = false ;
                  }
               }
            }
         }, 'complete': function ( XMLHttpRequest, textStatus ) {
            if( typeof( complete ) == 'function' )
            {
               try
               {
                  complete( XMLHttpRequest, textStatus ) ;
               }
               catch( e )
               {
                  printfDebug( e.stack ) ;
                  $rootScope.Components.Confirm.isShow = true ;
                  $rootScope.Components.Confirm.type = 1 ;
                  $rootScope.Components.Confirm.title = 'System error' ;
                  $rootScope.Components.Confirm.context = 'Javascript error: ' + e.message ;
                  $rootScope.Components.Confirm.ok = function(){
                     Loading.close() ;
                     $rootScope.Components.Confirm.isShow = false ;
                  }
               }
            }
            if( showLoading )
            {
               Loading.cancel() ;
            }
         }, 'beforeSend': function( XMLHttpRequest ){
            restBeforeSend( XMLHttpRequest ) ;
            if( typeof( before ) === 'function' )
            {
               try
               {
                  return before( XMLHttpRequest ) ;
               }
               catch( e )
               {
                  printfDebug( e.stack ) ;
                  $rootScope.Components.Confirm.isShow = true ;
                  $rootScope.Components.Confirm.type = 1 ;
                  $rootScope.Components.Confirm.title = 'System error' ;
                  $rootScope.Components.Confirm.context = 'Javascript error: ' + e.message ;
                  $rootScope.Components.Confirm.ok = function(){
                     Loading.close() ;
                     $rootScope.Components.Confirm.isShow = false ;
                  }
               }
            }
         } } ) ;
      }

      var emptyFunc = function(){} ;

      //校验参数
      g._checkEvent = function( event ){
         if( !event )
         {
            event = {} ;
         }
         if( typeof( event['init'] ) == 'undefined' )
         {
            event['init'] = emptyFunc ;
         }
         if( typeof( event['before'] ) == 'undefined' )
         {
            event['before'] = emptyFunc ;
         }
         if( typeof( event['success'] ) == 'undefined' )
         {
            event['success'] = emptyFunc ;
         }
         if( typeof( event['failed'] ) == 'undefined' )
         {
            event['failed'] = emptyFunc ;
         }
         if( typeof( event['error'] ) == 'undefined' )
         {
            event['error'] = emptyFunc ;
         }
         if( typeof( event['complete'] ) == 'undefined' )
         {
            event['complete'] = emptyFunc ;
         }
         return event ;
      }

      g._checkOptions = function( options ){
         if( !options )
         {
            options = {} ;
         }
         if( isNaN( options['delay'] ) == true )
         {
            options['delay'] = 0 ;
         }
         if( typeof( options['loop'] ) == 'undefined' )
         {
            options['loop'] = false ;
         }
         if( typeof( options['scope'] ) == 'undefined' )
         {
            options['scope'] = true ;
         }
         if( options['scope'] == true )
         {
            options['scope'] = $location.url() ;

            if ( options['scope'].length == 0 )
            {
               options['scope'] = '__init__' ;
            }
         }
         if( typeof( options['parseJson'] ) == 'undefined' )
         {
            options['parseJson'] = true ;
         }
         if( typeof( options['showLoading'] ) == 'undefined' )
         {
            options['showLoading'] = true ;
         }
         if( typeof( options['v'] ) == 'undefined' )
         {
            options['v'] = 'v2' ;
         }
         return options ;
      }

      //解析响应的Json
      g._parseJson2 = parseJson2 ;

      //循环模块
      g._eventLoop = function( type, url, data, event, options, loop ){
         if( options['scope'] === false || options['scope'] !== '__init__' || options['scope'] === $location.url() )
         {
            if( options['loop'] === loop )
            {
               setTimeout( function(){
                  g._sendAjax( type, url, data, event, options ) ;
               }, options['delay'] ) ;
            }
         }
      }

      //发送前
      g._eventBefore = function( type, url, data, event, options, XMLHttpRequest ){
         try
         {
            return event['before']( XMLHttpRequest ) ;
         }
         catch( e )
         {
            printfDebug( e.stack ) ;
            $rootScope.Components.Confirm.isShow = true ;
            $rootScope.Components.Confirm.type = 1 ;
            $rootScope.Components.Confirm.title = 'System error' ;
            $rootScope.Components.Confirm.context = 'Javascript error: ' + e.message ;
            $rootScope.Components.Confirm.ok = function(){
               Loading.close() ;
               $rootScope.Components.Confirm.isShow = false ;
            }
         }
      }

      //发送成功
      g._eventSuccess = function( type, url, data, event, options, json, textStatus, jqXHR ){
         var str, cmd ;
         var jsonArr ;
         var errJson = [] ;
         var arrLen ;

         if ( typeof( data['Sql'] ) == 'string' )
         {
            cmd = 'sql' ;
         }
         else
         {
            cmd = data['cmd'] ;
         }
         options['errJson'] = errJson ;

         if( options['v'] == 'v1' )
         {
            jsonArr = g._parseJson2( json, options['parseJson'], errJson ) ;
            arrLen = jsonArr.length ;
            if( options['parseJson'] == false && arrLen > 0 )
            {
               jsonArr[0] = JSON.parse( jsonArr[0] ) ;
            }
         }
         else if( options['v'] == 'v2' )
         {
            if( typeof( json ) == 'string' )
            {
               jsonArr = JSON.parse( json ) ;
               arrLen = jsonArr.length ;
            }
            else
            {
               jsonArr = json ;
               arrLen = jsonArr.length ;
            }

            if( isArray( jsonArr ) == false )
            {
               jsonArr = [ jsonArr ] ;
               arrLen = jsonArr.length ;
            }

            if( options['parseJson'] == false )
            {
               var tmpArr = jsonArr ;

               jsonArr = [] ;

               $.each( tmpArr, function( index, record ){
                  if ( index == 0 )
                  {
                     jsonArr.push( record ) ;
                  }
                  else
                  {
                     jsonArr.push( JSON.stringify( record ) ) ;
                  }
               } ) ;
            }
         }

         if ( arrLen > 0 )
         {
            if( isNaN( jsonArr[0]['status'] ) == false )
            {
               jsonArr[0]['errno'] = jsonArr[0]['status'] ;
            }
         }

         if( arrLen == 0 )
         {
            //有数据，但是没有记录，理论上不会发生
            try
            {
               event['failed']( { "errno": -10, "description": "System error", "detail": "Invalid rest response.", "cmd": cmd } ) ;
               g._eventLoop( type, url, data, event, options, 'failed' ) ;
            }
            catch( e )
            {
               printfDebug( e.stack ) ;
               $rootScope.Components.Confirm.isShow = true ;
               $rootScope.Components.Confirm.type = 1 ;
               $rootScope.Components.Confirm.title = 'System error' ;
               $rootScope.Components.Confirm.context = 'Javascript error: ' + e.message ;
               $rootScope.Components.Confirm.ok = function(){
                  Loading.close() ;
                  $rootScope.Components.Confirm.isShow = false ;
               }
            }
         }
         else if( jsonArr[0]['errno'] === 0 )
         {
            jsonArr.splice( 0, 1 ) ;

            try
            {
               event['success']( jsonArr, textStatus, jqXHR ) ;
               g._eventLoop( type, url, data, event, options, 'success' ) ;
            }
            catch( e )
            {
               if( window.SdbDebug === true )
               {
                  throw e ;
               }

               $rootScope.Components.Confirm.isShow = true ;
               $rootScope.Components.Confirm.type = 1 ;
               $rootScope.Components.Confirm.title = 'System error' ;
               $rootScope.Components.Confirm.context = 'Javascript error: ' + e.message ;
               $rootScope.Components.Confirm.ok = function(){
                  Loading.close() ;
                  $rootScope.Components.Confirm.isShow = false ;
               }
            }
         }
         else if( jsonArr[0]['errno'] === -62 )
         {
            //session id 不存在
            window.location.href = './login.html#/Login' ;
         }
         else
         {
            //其他错误
            jsonArr[0]['cmd'] = cmd ;

            if ( typeof( jsonArr[0]['description'] ) == 'undefined' && typeof( jsonArr[0]['message'] ) == 'string' )
            {
               jsonArr[0]['description'] = jsonArr[0]['message'] ;
               jsonArr[0]['detail'] = jsonArr[0]['message'] ;
            }

            try
            {
               event['failed']( jsonArr[0] ) ;
               g._eventLoop( type, url, data, event, options, 'failed' ) ;
            }
            catch( e )
            {
               if( window.SdbDebug === true )
               {
                  throw e ;
               }

               $rootScope.Components.Confirm.isShow = true ;
               $rootScope.Components.Confirm.type = 1 ;
               $rootScope.Components.Confirm.title = 'System error' ;
               $rootScope.Components.Confirm.context = 'Javascript error: ' + e.message ;
               $rootScope.Components.Confirm.ok = function(){
                  Loading.close() ;
                  $rootScope.Components.Confirm.isShow = false ;
               }
            }
         }
      }

      //发送错误
      g._eventError = function( type, url, data, event, options, XMLHttpRequest, textStatus, errorThrown ) {
         try
         {
            if( event['error'] === emptyFunc )
            {
               Loading.close() ;
            }
            else
            {
               event['error']( XMLHttpRequest, textStatus, errorThrown ) ;
            }
         }
         catch( e )
         {
            printfDebug( e.stack ) ;
            $rootScope.Components.Confirm.isShow = true ;
            $rootScope.Components.Confirm.type = 1 ;
            $rootScope.Components.Confirm.title = 'System error' ;
            $rootScope.Components.Confirm.context = 'Javascript error: ' + e.message ;
            $rootScope.Components.Confirm.ok = function(){
               Loading.close() ;
               $rootScope.Components.Confirm.isShow = false ;
            }
         }
      }

      //发送完成
      g._eventComplete = function ( type, url, data, event, options, XMLHttpRequest, textStatus ) {
         try
         {
            event['complete']( XMLHttpRequest, textStatus ) ;
            g._eventLoop( type, url, data, event, options, true ) ;
         }
         catch( e )
         {
            printfDebug( e.stack ) ;
            $rootScope.Components.Confirm.isShow = true ;
            $rootScope.Components.Confirm.type = 1 ;
            $rootScope.Components.Confirm.title = 'System error' ;
            $rootScope.Components.Confirm.context = 'Javascript error: ' + e.message ;
            $rootScope.Components.Confirm.ok = function(){
               Loading.close() ;
               $rootScope.Components.Confirm.isShow = false ;
            }
         }
         if( options['showLoading'] == true )
         {
            Loading.cancel() ;
         }
      }

      //检查网络
      g._checkNetwork = function(){
         g.getPing( function( times ){
            if( times >= 0 )
            {
               g._errorNum = 0 ;
               g._status = NORMAL ;
               g._lastErrorTask( { "cmd": g._lastErrorTaskCmd, "errno": -15, "description": "Network error", "detail": "Network error, request status unknown." } ) ;
               g._lastErrorTask = null ;

               if( g._errorTask.length > 0 )
               {
                  Loading.cancel() ;
               }

               $.each( g._errorTask, function( index, task ){
                  g._sendAjax( task['type'], task['url'], task['data'], task['event'], task['options'] ) ;
               } ) ;
               g._errorTask = [] ;
            }
            else
            {
               ++g._errorNum ;
               if( g._errorNum >= 10 )
               {
                  g._status = ERROR ;
               }
               setTimeout( g._checkNetwork, 1000 ) ;
            }
         } ) ;
      }

      /*
         发送请求
         type:    POST,GET
         url:     路径
         data:    post的数据
         event:   事件
                     init     初始化，如果开启循环，init会每次都执行
                                 返回值是true, 将不发送本次消息（如果开启了循环, 不会退出循环）
                                 返回值是false, 将不发送消息（如果开启了循环, 会退出循环）
                                 返回值是object, 将会代替data
                     before   发送消息前
                                 返回值是false, 将不发送消息（如果开启了循环, 会退出循环）, complete也不会执行
                     success  执行成功
                     failed   执行失败
                     error    错误
                     complete 完成
         options  选项
                     delay: 延迟多少毫秒再执行，默认0
                              如果开启循环，第一次不延迟，第二次开始都会延迟
                              如果不开启循环，延迟后执行。
                     loop:  循环, 默认是false
                              'success':  执行成功时循环
                              'failed':   执行失败时循环（不包含网络异常）
                              true:       成功失败都循环（包含网络异常）
                              false:      不循环
                     scope: 循环的作用域, 默认true
                              true:当前页面
                              false:所有页面
                     showLoading:  显示Loading动画, 默认true
                              true:显示
                              false:不显示
                     parseJson:  是否解析记录, 默认true
                              true:  解析记录    返回格式 [ {xxx}, {xxx}, ... ]
                              false: 不解析记录  返回格式 [ "xxx", "xxx", ... ]
      */
      g._sendAjax = function( type, url, data, event, options ){

         if( g._errorNum > 0 )
         {
            g._errorTask.push( { 'type': type, 'url': url, 'data': data, 'event': event, 'options': options } ) ;
            return ;
         }

         if( options['scope'] !== false && options['scope'] !== '__init__' && options['scope'] !== $location.url() )
         {
            return ;
         }

         if( options['loop'] === false && options['delay'] > 0 )
         {
            setTimeout( function(){
               g._sendAjax( type, url, data, event, options ) ;
            }, options['delay'] ) ;
            return ;
         }

         var initValue = event['init']() ;
         if( initValue === true )
         {
            return ;
         }
         else if( initValue === false )
         {
            return ;
         }
         else if( typeof( initValue ) === 'object' )
         {
            data = initValue ;
         }

         if( options['showLoading'] == true )
         {
            Loading.create() ;
         }

         $.ajax( { 'type': type, 'url': url, 'data': data, 'dataType': 'json',
            'beforeSend': function( XMLHttpRequest ){
               if( options['scope'] !== false && options['scope'] !== '__init__' && options['scope'] !== $location.url() )
               {
                  return ;
               }
               restBeforeSend( XMLHttpRequest ) ;
               return g._eventBefore( type, url, data, event, options, XMLHttpRequest ) ;
            },
            'success': function( json, textStatus, jqXHR ){
               if( options['scope'] !== false && options['scope'] !== '__init__' && options['scope'] !== $location.url() )
               {
                  return ;
               }
               g._errorNum = 0 ;
               g._eventSuccess( type, url, data, event, options, json, textStatus, jqXHR ) ;
            },
            'error': function( XMLHttpRequest, textStatus, errorThrown ) {
               if( options['scope'] !== false && options['scope'] !== '__init__' && options['scope'] !== $location.url() )
               {
                  return ;
               }
               if( XMLHttpRequest.status != 0 ) //网络错误是0
               {
                  g._eventError( type, url, data, event, options, XMLHttpRequest, textStatus, errorThrown ) ;
                  return ;
               }
               if( g._errorNum == 0 )
               {
                  ++g._errorNum ;
                  g._eventError( type, url, data, event, options, XMLHttpRequest, textStatus, errorThrown ) ;
                  g._lastErrorTask = event['failed'] ;
                  g._lastErrorTaskCmd = data['cmd'] ;
                  g._checkNetwork() ;
               }
               g._errorTask.push( { 'type': type, 'url': url, 'data': data, 'event': event, 'options': options } ) ;
            },
            'complete': function ( XMLHttpRequest, textStatus ) {
               g._eventComplete( type, url, data, event, options, XMLHttpRequest, textStatus ) ;
            }
         } ) ;
      }

      //发送请求(废弃)
      g._post2 = function( data, showLoading ){
         var defferred = $q.defer() ;
         if( typeof( showLoading ) == 'undefined' ) showLoading = true ;
         if( showLoading )
         {
            Loading.create() ;
         }
         $.ajax( { 'type': 'POST', 'url': '/', 'data': data, 'success': function( json, textStatus, jqXHR ){
            json = trim( json ) ;
            if( json.length == 0 )
            {
               //收到响应，但是没有任何数据
               defferred.reject( { "errno": -10, "description": "System error", "detail": "No rest response data." } ) ;
            }
            else
            {
               var jsonArr = g._parseJsons( json ) ;
               if( jsonArr.length == 0 )
               {
                  //有数据，但是没有记录，理论上不会发生
                  defferred.reject( { "errno": -10, "description": "System error", "detail": "Rest response data error." } ) ;
               }
               else if( jsonArr[0]['errno'] === 0 )
               {
                  jsonArr.splice( 0, 1 ) ;
                  defferred.resolve( jsonArr ) ;
               }
               else if( jsonArr[0]['errno'] === -62 )
               {
                  //session id 不存在
                  window.location.href = './login.html#/Login' ;
               }
               else
               {
                  //其他错误
                  defferred.reject( jsonArr[0] ) ;
               }
            }
         }, 'error': function( XMLHttpRequest, textStatus, errorThrown ) {
            defferred.reject() ;
         }, 'complete': function ( XMLHttpRequest, textStatus ) {
            if( showLoading )
            {
               Loading.cancel() ;
            }
         }, 'beforeSend': function( XMLHttpRequest ){
            restBeforeSend( XMLHttpRequest ) ;
         } } ) ;
         return defferred.promise ;
      }

      //测试发送(测试用)
      g._postTest = function( url, success, failed, error )
      {
         //Loading.create() ;
         g.getFile( url, true, function( jsons ){
            var jsonList = g._parseJsons( jsons ) ;
            if( jsonList[0]['errno'] == 0 )
            {
               jsonList.splice( 0, 1 ) ;
               if( typeof( success ) == 'function' )
               {
                  success( jsonList ) ;
               }
            }
            else
            {
               if( typeof( failed ) == 'function' )
               {
                  failed( jsonList[0] ) ;
               }
            }
            //Loading.cancel() ;
         } ) ;
      }

      //解析响应的Json
      g._parseJsons = function( str, errJson )
      {
	      var json_array = [] ;
	      var i = 0, len = str.length ;
	      var chars, level, isEsc, isString, start, end, subStr, json, errType ;
         errType = isArray( errJson ) ;
	      while( i < len )
	      {
		      while( i < len ){	chars = str.charAt( i ) ;	if( chars === '{' ){	break ;	}	++i ;	}
		      level = 0, isEsc = false, isString = false, start = i ;
		      while( i < len )
		      {
               var isJson = true ;
			      chars = str.charAt( i ) ;
			      if( isEsc ){	isEsc = false ;	}
			      else
			      {
				      if( ( chars === '{' || chars === '[' ) && isString === false ){	++level ;	}
				      else if( ( chars === '}' || chars === ']' ) && isString === false )
				      {
					      --level ;
					      if( level === 0 )
					      {
						      ++i ;
						      end = i ;
						      subStr = str.substring( start, end ) ;
                        try{
                           json = JSON.parse( subStr ) ;
                        }catch(e){
                           isJson = false ;
                           json = { " ": subStr } ;
                        }
                        if( errType == true )
                        {
                           errJson.push( !isJson ) ;
                        }
						      json_array.push( json ) ;
						      break ;
					      }
				      }
				      else if( chars === '"' ){	isString = !isString ;	}
				      else if( chars === '\\' ){	isEsc = true ;	}
			      }
			      ++i ;
		      }
	      }
	      return json_array ;
      }     

      //获取文件
      g.getFile = function( url, async, success, error )
      {
         $.ajax( {
            'async': async,
            'url': url,
            'type': 'GET',
            'dataType': 'text',
            'success': success,
            'error': error
         } ) ;
      }

      //获取配置文件(废弃)
      g.getConfig = function( fileName, success )
      {
         var language = SdbFunction.LocalData( 'SdbLanguage' ) ;
         var newUrl = './config/' + fileName + '_' + language ;
         $.get( newUrl, {}, function( reData ){
            success( reData ) ;
         }, 'json' ) ;
      }

      //获取语言文件
      g.getLanguage = function( fileName, success )
      {
         var language = SdbFunction.LocalData( 'SdbLanguage' ) ;
         var newUrl = './app/language/' + fileName + '_' + language ;
         $.get( newUrl, {}, function( reData ){
            success( reData ) ;
         }, 'json' ) ;
      }
      
      //om系统操作
      g.OmOperation = function( data, event, options ){
         event = g._checkEvent( event ) ;
         options = g._checkOptions( options ) ;
         g._sendAjax( 'POST', '/', data, event, options ) ;
         //g._insert( 'POST', '/', data, event, options ) ;
      }

      //数据操作
      g.DataOperation = function( data, event, options ){
         var oldBefore = event ? event['before'] : null ;
         event['before'] = function( jqXHR ){
	         var clusterName = SdbFunction.LocalData( 'SdbClusterName' ) ;
	         if( clusterName !== null )
	         {
		         jqXHR.setRequestHeader( 'SdbClusterName', clusterName ) ;
	         }
	         var businessName = SdbFunction.LocalData( 'SdbModuleName' )
	         if( businessName !== null )
	         {
		         jqXHR.setRequestHeader( 'SdbBusinessName', businessName ) ;
	         }
            if( typeof( oldBefore ) == 'function' )
            {
               return oldBefore( jqXHR ) ;
            }
         }
         event = g._checkEvent( event ) ;
         options = g._checkOptions( options ) ;
         g._sendAjax( 'POST', '/', data, event, options ) ;
      }

      g.DataOperationV2 = function( path, data, event, options ){
         var oldBefore = event ? event['before'] : null ;
         event['before'] = function( jqXHR ){
	         var clusterName = SdbFunction.LocalData( 'SdbClusterName' ) ;
	         if( clusterName !== null )
	         {
		         jqXHR.setRequestHeader( 'SdbClusterName', clusterName ) ;
	         }
	         var businessName = SdbFunction.LocalData( 'SdbModuleName' )
	         if( businessName !== null )
	         {
		         jqXHR.setRequestHeader( 'SdbBusinessName', businessName ) ;
	         }
            if( typeof( oldBefore ) == 'function' )
            {
               return oldBefore( jqXHR ) ;
            }
         }
         event = g._checkEvent( event ) ;
         options = g._checkOptions( options ) ;
         g._sendAjax( 'POST', path, data, event, options ) ;
      }

      //手工设置cluster和module
      g.DataOperationV21 = function( clusterName, businessName, path, data, event, options ){
         var oldBefore = event ? event['before'] : null ;
         event['before'] = function( jqXHR ){
	         if( clusterName !== null )
	         {
		         jqXHR.setRequestHeader( 'SdbClusterName', clusterName ) ;
	         }
	         if( businessName !== null )
	         {
		         jqXHR.setRequestHeader( 'SdbBusinessName', businessName ) ;
	         }
            if( typeof( oldBefore ) == 'function' )
            {
               return oldBefore( jqXHR ) ;
            }
         }
         event = g._checkEvent( event ) ;
         options = g._checkOptions( options ) ;
         g._sendAjax( 'POST', path, data, event, options ) ;
      }

      //数据操作( 手工设置cluster和module )
      g.DataOperation2 = function( clusterName, businessName, data, event, options, errJson ){
         var oldBefore = event ? event['before'] : null ;
         event['before'] = function( jqXHR ){
	         if( clusterName !== null )
	         {
		         jqXHR.setRequestHeader( 'SdbClusterName', clusterName ) ;
	         }
	         if( businessName !== null )
	         {
		         jqXHR.setRequestHeader( 'SdbBusinessName', businessName ) ;
	         }
            if( typeof( oldBefore ) == 'function' )
            {
               return oldBefore( jqXHR ) ;
            }
         }
         event = g._checkEvent( event ) ;
         options = g._checkOptions( options ) ;
         g._sendAjax( 'POST', '/', data, event, options ) ;
         //g._insert( 'POST', '/', data, event, options, errJson ) ;
      }

      //sequoiasql操作
      g.SequoiaSQL = function( data, success, failed, error, complete ){
         var clusterName = SdbFunction.LocalData( 'SdbClusterName' ) ;
	      if( clusterName != null )
	      {
		      data['ClusterName'] = clusterName ;
	      }
	      var businessName = SdbFunction.LocalData( 'SdbModuleName' )
	      if( businessName != null )
	      {
            data['BusinessName'] = businessName ;
	      }
         Loading.create() ;
         g._insert( 'POST', '/', data, {
            'success': function( returnData ){
               var taskID = returnData[0] ;
               var queryTask = function(){
                  var taskData = { 'cmd': 'query task', 'filter': JSON.stringify( taskID ) } ;
                  g._insert( 'POST', '/', taskData, {
                     'success': function( taskInfo ){
                        if( taskInfo[0]['Status'] == 0 )
                        {
                           queryTask()
                           return ;
                        }
                        if( taskInfo[0]['Status'] == 3 || taskInfo[0]['Status'] == 4 )
                        {
                           success( taskInfo[0]['ResultInfo'], true ) ;
                           Loading.cancel() ;
                           return ;
                        }
                        success( taskInfo[0]['ResultInfo'], false ) ;
                        setTimeout( queryTask, 100 ) ;
                     },
                     'failed': function( errorInfo ){
                        Loading.cancel() ;
                        if( typeof( failed ) == 'function')
                        {
                           failed( errorInfo ) ;
                        }
                     },
                     'error': function( XMLHttpRequest, textStatus, errorThrown ){
                        Loading.cancel() ;
                        if( typeof( error ) === 'function' )
                        {
                           error( XMLHttpRequest, textStatus, errorThrown ) ;
                        }
                     },
                     'comolete': complete
                  }, {
                     'showLoading': false
                  } ) ;
               }
               queryTask() ;
            },
            'failed': function( errorInfo ){
               Loading.cancel() ;
               if( typeof( failed ) == 'function')
               {
                  failed( errorInfo ) ;
               }
            },
            'error': function( XMLHttpRequest, textStatus, errorThrown ){
               Loading.cancel() ;
               if( typeof( error ) === 'function' )
               {
                  error( XMLHttpRequest, textStatus, errorThrown ) ;
               }
            },
            'complete': complete
         } ) ;
      }

      //SQL(自动获取cluster和module)
      g.Exec = function( sql, event, options ){
         var data = { 'cmd': 'exec', 'sql': sql } ;
         var oldBefore = event ? event['before'] : null ;
         event['before'] = function( jqXHR ){
	         var clusterName = SdbFunction.LocalData( 'SdbClusterName' ) ;
	         if( clusterName !== null )
	         {
		         jqXHR.setRequestHeader( 'SdbClusterName', clusterName ) ;
	         }
	         var businessName = SdbFunction.LocalData( 'SdbModuleName' )
	         if( businessName !== null )
	         {
		         jqXHR.setRequestHeader( 'SdbBusinessName', businessName ) ;
	         }
            if( typeof( oldBefore ) == 'function' )
            {
               return oldBefore( jqXHR ) ;
            }
         }
         event = g._checkEvent( event ) ;
         options = g._checkOptions( options ) ;
         g._sendAjax( 'POST', '/', data, event, options ) ;
         //g._insert( 'POST', '/', data, event, options ) ;
      }

      //SQL(手工设置cluster和module)
      g.Exec2 = function( clusterName, businessName, sql, event, options ){
         var data = { 'cmd': 'exec', 'sql': sql } ;
         var oldBefore = event ? event['before'] : null ;
         event['before'] = function( jqXHR ){
	         if( clusterName !== null )
	         {
		         jqXHR.setRequestHeader( 'SdbClusterName', clusterName ) ;
	         }
	         if( businessName !== null )
	         {
		         jqXHR.setRequestHeader( 'SdbBusinessName', businessName ) ;
	         }
            if( typeof( oldBefore ) == 'function' )
            {
               return oldBefore( jqXHR ) ;
            }
         }
         event = g._checkEvent( event ) ;
         options = g._checkOptions( options ) ;
         g._sendAjax( 'POST', '/', data, event, options ) ;
         //g._insert( 'POST', '/', data, event, options ) ;
      }

      //登录
      g.Login = function( username, password, success, failed, error, complete ){
         password = $.md5( password ) ;
         var timestamp = parseInt( ( new Date().getTime() ) / 1000 ) ;
	      var data = { 'cmd' : 'login', 'user': username, 'passwd': password, 'Timestamp': timestamp } ;
         g._post( 'POST', '/', data, null, success, failed, error, complete, false ) ;
      }

      //修改密码
      g.ChangePasswd = function( username, password, newPassword, success, failed, error, complete ){
         var timestamp = parseInt( ( new Date().getTime() ) / 1000 ) ;
         password = $.md5( password ) ;
         newPassword = $.md5( newPassword ) ;
	      var data = { 'cmd' : 'change passwd', 'User': username, 'Passwd': password, 'Newpasswd': newPassword, 'Timestamp': timestamp } ;
         var event = {
            'success': success,
            'failed': failed,
            'error': error,
            'complete': complete
         } ;
         var options = { 'showLoading': false } ;
         event = g._checkEvent( event ) ;
         options = g._checkOptions( options ) ;
         g._sendAjax( 'POST', '/', data, event, options ) ;
         /*
         g._insert( 'POST', '/', data, {
            'success': success,
            'failed': failed,
            'error': error,
            'complete': complete
         }, {
            'showLoading': false
         } ) ;
         */
      }

      g.getPing = function( complete ){
         var time1 = $.now() ;
         g.getFile( './app/language/test.txt', true, function( text ){
            var time2 = $.now() ;
            complete( time2 - time1 ) ;
         }, function(){
            complete( -1 ) ;
         } ) ;
      }

      g.GetLog = function( data, success, error, complete, showLoading ){
         if( typeof( showLoading ) == 'undefined' ) showLoading = true ;
         if( showLoading )
         {
            Loading.create() ;
         }
         $.ajax( { 'type': 'POST', 'url': '/', 'data': data, 'success': function( text, textStatus, jqXHR ){
            if( typeof( success ) === 'function' ) success( text, textStatus, jqXHR ) ;
         }, 'error': function( XMLHttpRequest, textStatus, errorThrown ) {
            if( typeof( error ) === 'function' ) error( XMLHttpRequest, textStatus, errorThrown ) ;
         }, 'complete': function ( XMLHttpRequest, textStatus ) {
            if( typeof( complete ) == 'function' ) complete( XMLHttpRequest, textStatus ) ;
            if( showLoading )
            {
               Loading.cancel() ;
            }
         }, 'beforeSend': function( XMLHttpRequest ){
            restBeforeSend( XMLHttpRequest ) ;
         } } ) ;
      }
   } ) ;
}());