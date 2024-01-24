(function(){
   var sacApp = window.SdbSacManagerModule ;
   //提示标签
   sacApp.service( 'Tip', function( $window ){
      var g = this ;
      g.tipEle = null ;
      g.create = function(){
         g.tipEle = $( '<div></div>' ).addClass( 'tooltip' ).html( '<div class="arrow"></div><div class="inner"></div>' ).appendTo( $( 'body' ) ) ;
      }
      g.hide = function(){
         $( g.tipEle ).hide() ;
      }
      g.show = function( text, mTop, mLeft ){
         var left = 0 ;
         var top = 0 ;
         var buttonL = mLeft ;
         var buttonT = mTop ;
         var buttonW = 22 ;
         var buttonH = 22 ;
         $( g.tipEle ).removeClass().addClass( 'tooltip' ).show().children( '.inner' ).text( text ) ;
         var tooltipW = $( g.tipEle ).outerWidth() ;
         var tooltipH = $( g.tipEle ).outerHeight() ;
         var className = '' ;
         var sdbWidth  = $( window ).width() ;
         var sdbHeight = $( window ).height() ;
         if( buttonL + buttonW + tooltipW < sdbWidth && className === '' )
         {
            className = 'tooltip right' ;
            left = buttonL + buttonW ;
            if( buttonT + parseInt( buttonH * 0.5 ) + tooltipH < sdbHeight )
            {
               top = buttonT + parseInt( buttonH * 0.5 ) ;
            }
            else if ( tooltipH - parseInt( buttonH * 0.5 ) < buttonT )
            {
               className += ' right-bottom' ;
               top = buttonT + parseInt( buttonH * 0.5 ) - tooltipH ;
            }
            else
            {
               className = '' ;
            }
         }
         else if( tooltipW + 10 < buttonL && className === '' )
         {
            className = 'tooltip left' ;
            left = buttonL - tooltipW - 10 ;
            if( buttonT + parseInt( buttonH * 0.5 ) + tooltipH < sdbHeight )
            {
               top = buttonT + parseInt( buttonH * 0.5 ) ;
            }
            else if ( tooltipH - 18 - parseInt( buttonH * 0.5 ) < buttonT )
            {
               className += ' left-bottom' ;
               top = buttonT + parseInt( buttonH * 0.5 ) - tooltipH ;
            }
            else
            {
               className = '' ;
            }
         }
         else if( tooltipW < buttonL && className === '' )
         {
            className = 'tooltip top top-right' ;
            left = buttonL - tooltipW ;
            if( tooltipH < buttonT )
            {
               top = buttonT - tooltipH ;
            }
            else if ( tooltipH + buttonT + buttonH < sdbHeight )
            {
               className = 'tooltip bottom bottom-right' ;
               top = buttonT + buttonH ;
            }
            else
            {
               className = '' ;
            }
         }
         else if ( buttonL + buttonW + tooltipW < sdbWidth && className === '' )
         {
            className = 'tooltip top top-left' ;
            left = buttonL + buttonW ;
            if( tooltipH < buttonT )
            {
               top = buttonT - tooltipH ;
            }
            else if ( tooltipH + buttonT + buttonH < sdbHeight )
            {
               className = 'tooltip bottom bottom-left' ;
               top = buttonT + buttonH ;
            }
            else
            {
               className = '' ;
            }
         }
         else if ( parseInt( tooltipW * 0.5 ) < buttonL && 
                   parseInt( tooltipW * 0.5 ) + buttonL + parseInt( buttonW * 0.5 ) < sdbWidth && 
                   className === '' )
         {
            className = 'tooltip' ;
            if( tooltipH < buttonT )
            {
               className += ' top' ;
               top = buttonT - tooltipH ;
               left = buttonL + parseInt( buttonW * 0.5 ) - parseInt( tooltipW * 0.5 ) ;
            }
            else if ( buttonT + buttonH + tooltipH < sdbHeight )
            {
               className += ' bottom' ;
               top = buttonT + buttonH ;
               left = buttonL + parseInt( buttonW * 0.5 ) - parseInt( tooltipW * 0.5 ) ;
            }
            else
            {
               className = '' ;
            }
         }
         if( className === '' )
         {
            try{
               throw new Error( '该组件无法在页面正常显示' ) ;
            }
            catch( e ){
               printfDebug( e.stack ) ;
            }
            $( g.tipEle ).removeClass().addClass( 'tooltip right right-bottom' ).css( { 'left': mLeft + buttonW, 'top': mTop + buttonH } ) ;
            return ;
         }
         $( g.tipEle ).removeClass().addClass( className ).css( { 'left': left, 'top': top } ) ;
      }
      g.auto = function(){
         var timeSet = null ;
         var timeSet2 = null ;
         var showTime = 1500 ;
         var hideTime = 7000 ;
         var isShow = false ;
         //angular.element( $window ).bind( 'mousemove', function ( ele ) {
         $( document ).on( 'mousemove', function( ele ){
            var pageX = ele['pageX'] ;
            var pageY = ele['pageY'] ;
            ele = $( ele['target'] ) ;
            function checkEleParent( element )
            {
               var isEvent = false ;
               if( element !== null )
               {
                  if( $( element ).attr( 'data-desc' ) )
                  {
                     isEvent = true ;
                     var text = $( element ).attr( 'data-desc' ) ;
                     g.show( text, pageY, pageX ) ;
                  }
                  else if( $( element ).hasClass( 'Ellipsis' ) )
                  {
                     isEvent = true ;
                     var text = $( element ).text() ;
                     text = trim( text ) ;
                     if( text.length > 0 )
                     {
                        if( timeSet != null )
                        {
                           clearTimeout( timeSet ) ;
                           timeSet = null ;
                        }
                        if( timeSet2 != null )
                        {
                           clearTimeout( timeSet2 ) ;
                        }
                        if( isShow == false )
                        {
                           timeSet = setTimeout( function(){
                              g.show( text, pageY, pageX ) ;
                              timeSet = null ;
                              isShow = true ;
                           }, showTime ) ;
                           timeSet2 = setTimeout( function(){
                              g.hide() ;
                              timeSet2 = null ;
                              isShow = false ;
                           }, hideTime ) ;
                        }
                        else
                        {
                           g.show( text, pageY, pageX ) ;
                           timeSet2 = setTimeout( function(){
                              g.hide() ;
                              timeSet2 = null ;
                              isShow = false ;
                           }, hideTime - showTime ) ;
                        }
                     }
                  }
                  if( $( element ).get(0) !== document && $( element ).get(0).parentNode !== document.body && isEvent === false )
                  {
                     checkEleParent( $( element ).get(0).parentNode ) ;
                  }
               }
            }
            checkEleParent( ele ) ;
         } ) ;
         //angular.element( $window ).bind( 'mouseout', function ( ele ) {
         $( document ).on( 'mouseout', function( ele ){
            if( timeSet != null )
            {
               clearTimeout( timeSet ) ;
               timeSet = null ;
            }
            g.hide() ;
         } ) ;
      }
   } ) ;

   //创建loading
   sacApp.service( 'Loading', function( $window ){
      var g = this ;
      var counter = 0 ;
      var mask = $( '<div></div>' ).addClass( 'mask-screen alpha30' ).css( { 'background-color': '#FFF', 'z-index': 99999 } ) ;
      var icon = $( '<div></div>').addClass( 'roundLoading' ).html( '<img src="./images/loading.gif">' ).css( { 'z-index': 100000 } ) ; ;
      var tip = $( '<div>loading</div>' ).appendTo( icon ) ;
      var timer = null ;
      var pointNum = 0 ;
      $( document ).keydown( function( event ){
         if( counter > 0 )
         {
            if ( ( event.altKey ) && ( ( event.keyCode == 37 ) || ( event.keyCode == 39 ) ) )    
            { 
               return false;
            }
            if( event.keyCode == 8 )
            {
               return false ;
            }
         }
      } ) ;
      g.create = function(){
         counter += 1 ;
         if( counter == 1 )
         {
            mask.appendTo( document.body ) ;
            icon.appendTo( document.body ) ;
            timer = setInterval( function(){
               ++pointNum ;
               if( pointNum == 1 )
               {
                  $( tip ).text( "loading." ) ;
               }
               else if( pointNum == 2 )
               {
                  $( tip ).text( "loading.." ) ;
               }
               else if( pointNum == 3 )
               {
                  $( tip ).text( "loading..." ) ;
                  pointNum = 0 ;
               }
            }, 800 ) ;
            g.resize() ;
         }
      }
      
      g.resize = function(){
         var left ;
         var top ;
         var bodyWidth = $( window ).width() ;  
         var bodyHeight = $( window ).height() ;
         left = ( bodyWidth - $( icon ).width() ) * 0.5 ;
         top = ( bodyHeight - $( icon ).height() - 50 ) * 0.5 ;
         $( icon ).css( { 'left': left, 'top': top } ) ;
      }

      g.cancel = function(){
         if( counter > 0 )
         {
            counter -= 1 ;
            if( counter == 0 )
            {
               clearInterval( timer ) ;
               timer = null ;
               icon.detach() ;
               mask.detach() ;
            }
         }
      }

      g.close = function(){
         counter = 0 ;
         clearInterval( timer ) ;
         timer = null ;
         icon.detach() ;
         mask.detach() ;
      }

      angular.element( $window ).bind( 'resize', function(){
         g.resize() ;
      } ) ;
   } ) ;

   //异步消息，同步调用
   sacApp.service( 'SdbPromise', function(){
      this['init'] = function( maxNum, use ){
         if ( isNaN( use ) )
         {
            use = -1 ;
         }
         return {
            '_use': 0,
            '_useMax': use,
            '_max': maxNum,
            '_total': 0,
            '_valHandler': {},
            '_error': false,
            '_faile': false ,
            '_thenFun': null,
            '_errFun': null,
            'then': function( thenFun ){
               this._thenFun = thenFun ;
               if( this._total >= this._max && this._faile == false )
               {
                  this._thenFun( this._valHandler ) ;
                  this._total = 0 ;
                  this._valHandler = {} ;
                  ++this._use ;
               }
            },
            'error': function( errFun ){
               this._errFun = errFun ;
               if( this._faile == true )
               {
                  this._errFun( this._error ) ;
                  this._faile = false ;
               }
            },
            'resolve': function( key, value ){
               if ( this._useMax > 0 && this._use >= this._useMax )
               {
                  return ;
               }

               this._valHandler[key] = value ;
               ++this._total ;
               if( this._total >= this._max && this._faile == false )
               {
                  if( this._thenFun === null )
                  {
                     return ;
                  }
                  this._thenFun( this._valHandler ) ;
                  this._total = 0 ;
                  this._valHandler = {} ;
                  ++this._use ;
               }
            },
            'throw': function( error ){
               this._faile = true ;
               this._error = error ;
               if( this._errFun === null )
               {
                  return ;
               }
               this._errFun( this._error ) ;
            },
            'clear': function(){
               this._total = 0 ;
               this._valHandler = {} ;
               this._faile = false ;
            }
         } ;
      }
   } ) ;

   //交换器，用于同页面跨控制器
   sacApp.service( 'SdbSwap', function( $rootScope ){
      var swap = {} ;
      $rootScope.$on( '$locationChangeStart', function( event, newUrl, oldUrl ){
         for( key in swap ) {
            delete swap[key] ;
         }
      } ) ;
      return swap ;
   } ) ;

   //消息通知，用于同页面直接发生通知消息
   sacApp.service( 'SdbSignal', function( $rootScope ){
      var queue = [] ;
      var commitQueue = [] ;
      /*
      注册信号监听
         event:      string   监听的信号名
         callback:   function 回调函数
         isGlobal:   bool     是不是全局监听，如果不是，页面切换就会注销监听，一般情况设置false即可
      */
      this['on'] = function( event, callback, isGlobal ){
         queue.push( { 'event': event, 'callback': callback, 'global': isGlobal } ) ;

         $.each( commitQueue, function( index, caller ){
            if( caller['event'] === event )
            {
               callback( caller['data'] ) ;
            }
         } ) ;
      }

      /*
      发送信号
         event:   string      监听的信号名
         data:    interface   数据
      */
      this['commit'] = function( event, data ) {
         var results = [] ;
         $.each( queue, function( index, listener ){
            if( listener['event'] === event )
            {
               results.push( listener['callback']( data ) ) ;
            }
         } ) ;
         if ( results.length == 0 )
         {
            commitQueue.push( { 'event': event, 'data': data } ) ;
         }
         return results ;
      }
      $rootScope.$on( '$locationChangeStart', function( event, newUrl, oldUrl ){
         var newQueue = [] ;
         $.each( queue, function( index, listener ){
            if( listener['global'] === true )
            {
               newQueue.push( listener ) ;
            }
         } ) ;
         queue = newQueue ;
      } ) ;
   } ) ;
}());