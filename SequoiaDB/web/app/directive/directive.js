(function(){
   var sacApp = window.SdbSacManagerModule ;

   //创建网格的指令(免html构造)(准备废弃)
   sacApp.directive( 'createGrid', function( $filter, $compile, $window, $rootScope, SdbFunction ){
      var dire = {
         restrict: 'A',
         scope: {
            data: '=createGrid'
         },
         templateUrl: './app/template/Component/Grid.html',
         replace: false,
         controller: function( $scope, $element ){
            $scope.isFirst_firefox = 0 ;
            $scope.Setting = {
               titleWidth: [],
               bodyWidth: [],
               grid:{
                  borderBottom: '',
                  height: '0px',
                  maxHeight: 'none',
                  tdHeight: []
               }
            } ;
            $scope.Grid = { 'orderName': '0.text', 'reverse': false } ;
         },
         compile: function( element, attributes ){
            //设置列宽
            function setColumnWidth( scope, parentEle, isFirefox )
            {
               var gridEle = $( '> .Grid:first', parentEle ) ;
               var bodyEle = $( '> .GridBody:first', gridEle ) ;
               var tdBorderEle = $( '> .GridHeader:first > .GridTr:first > .GridTd:first', gridEle ) ;
               var tdBorderWidth = numberCarry( tdBorderEle.outerWidth() - tdBorderEle.width() ) ;
               var titleNum = scope.data.title.length ;
               var width = parseInt( gridEle.outerWidth() - numberCarry( gridEle.outerWidth() - gridEle.width() ) - titleNum * tdBorderWidth ) ;
               var bodyWidth = parseInt( $( bodyEle ).outerWidth() - titleNum * tdBorderWidth ) ;//width - 18 ;
               if( isFirefox == true  && scope.isFirst_firefox < 2 )
               {
                  ++scope.isFirst_firefox ;
                  bodyWidth -= 17 ;
               }
               var scrollWidth = width - bodyWidth ;
               var titleWidth = scope.data.options.grid.titleWidth ;
               var sumWidth = 0 ;
               var sumBodyWidth = 0 ;
               scope.Setting.titleWidth = [] ;
               scope.Setting.bodyWidth = [] ;
               if( typeof( titleWidth ) == 'undefined' || titleWidth == 'auto' )
               {
                  var aveWidth = parseInt( width / titleNum ) ;
                  for( var i = 0; i < titleNum; ++i )
                  {
                     //标题
                     var tmpTdWidth = aveWidth ;
                     if( i + 1 == titleNum )
                     {
                        tmpTdWidth = width - sumWidth - scrollWidth ;
                     }
                     scope.Setting.titleWidth.push( tmpTdWidth + 'px' ) ;
                     sumWidth += tmpTdWidth ;
                     //内容
                     tmpTdWidth = aveWidth ;
                     if( i + 1 == titleNum )
                     {
                        tmpTdWidth = bodyWidth - sumBodyWidth ;
                     }
                     scope.Setting.bodyWidth.push( tmpTdWidth + 'px' ) ;
                     sumBodyWidth += tmpTdWidth ;
                  }
               }
               else
               {
                  $.each( titleWidth, function( index ){
                     scope.Setting.titleWidth.push( 0 ) ;
                     scope.Setting.bodyWidth.push( 0 ) ;
                  } ) ;
                  var lastIndex = 0 ;
                  $.each( titleWidth, function( index ){
                     if( typeof( titleWidth[index] ) == 'string' )
                     {
                        //标题
                        var tmpTdWidth = parseInt( titleWidth[index] ) ;
                        var bodyTdWidth = tmpTdWidth ;
                        /*
                        if( index + 1 == titleNum )
                        {
                           tmpTdWidth += scrollWidth ;
                        }
                        */
                        scope.Setting.titleWidth[index] = ( tmpTdWidth + 'px' ) ;
                        sumWidth += tmpTdWidth ;
                        //内容
                        tmpTdWidth = bodyTdWidth ;
                        sumBodyWidth += tmpTdWidth ;
                        scope.Setting.bodyWidth[index] = ( tmpTdWidth + 'px' ) ;
                     }
                     else if( typeof( titleWidth[index] ) == 'number' )
                     {
                        lastIndex = index ;
                     }
                  } ) ;
                  var lastSumWidth = bodyWidth - sumWidth ;
                  $.each( titleWidth, function( index ){
                     if( typeof( titleWidth[index] ) == 'number' )
                     {
                        //标题
                        var tmpTdWidth = 0 ;
                        tmpTdWidth = parseInt( titleWidth[index] * lastSumWidth * 0.01 ) ;
                        var bodyTdWidth = tmpTdWidth ;
                        if( index == lastIndex )
                        {
                           tmpTdWidth = bodyWidth - sumWidth ;
                        }
                        if( tmpTdWidth < 0 ) tmpTdWidth = 0 ;
                        scope.Setting.titleWidth[index] = ( tmpTdWidth + 'px' ) ;
                        sumWidth += tmpTdWidth ;
                        //内容
                        tmpTdWidth = bodyTdWidth ;
                        if( index == lastIndex )
                        {
                           tmpTdWidth = bodyWidth - sumBodyWidth ;
                        }
                        if( tmpTdWidth < 0 ) tmpTdWidth = 0 ;
                        scope.Setting.bodyWidth[index] = ( tmpTdWidth + 'px' ) ;
                        sumBodyWidth += tmpTdWidth ;
                     }
                  } ) ;
               }
            }
            //设置行高
            function setRowHeight( scope, parentEle )
            {
               var model = scope.data.options.grid.tdModel ;
               if( typeof( model ) == 'undefined' ) model = 'auto' ;
               scope.Setting.grid.tdHeight = [] ;
               if( model == 'dynamic' )
               {
                  $.each( scope.data['body'], function( index, row ){
                     scope.Setting.grid.tdHeight.push( 'auto' ) ;
                  } ) ;
               }
               else if( model == 'fixed' && typeof( scope.data.options.grid.tdHeight ) != 'undefined' )
               {
                  if( typeof( scope.data.options.grid.tdHeight ) == 'string' )
                  {
                     $.each( scope.data['body'], function( index, row ){
                        scope.Setting.grid.tdHeight.push( scope.data.options.grid.tdHeight ) ;
                     } ) ;
                  }
                  else if( typeof( scope.data.options.grid.tdHeight ) == 'number' )
                  {
                     $.each( scope.data['body'], function( index, row ){
                        scope.Setting.grid.tdHeight.push( scope.data.options.grid.tdHeight + 'px' ) ;
                     } ) ;
                  }
               }
               else if( model == 'auto' )
               {
                  scope.$apply() ;
                  var columnNum = 0 ;
                  if( scope.data['body'].length > 0 )
                  {
                     columnNum = scope.data['body'][0].length ;
                  }
                  var tdEle = $( '> .Grid:first > .GridBody:first > .GridTr > .GridTd', parentEle ) ;
                  $.each( scope.data['body'], function( index, row ){
                     var maxHeight = 0 ;
                     $.each( row, function( index2, column ){
                        var tdHeight = $( tdEle[ index * columnNum + index2 ] ).height() ;
                        if( tdHeight > maxHeight )
                        {
                           maxHeight = tdHeight ;
                        }
                     } ) ;
                     scope.Setting.grid.tdHeight.push( maxHeight + 'px' ) ;
                  } ) ;
               }
            }
            //设置网格高度
            function setGridHeight( scope, parentEle )
            {
               scope.$apply( function(){
                  var height = $( parentEle ).outerHeight() ;
                  var gridModel = scope.data.options.grid.gridModel ;
                  if( typeof( gridModel ) == 'undefined' ) gridModel = 'auto' ;
                  if( typeof( scope.data['tool'] ) == 'object' )
                  {
                     if( typeof( scope.data.tool['position'] ) == 'string' )
                     {
                        if( scope.data.tool.position == 'top' || scope.data.tool.position == 'bottom' )
                        {
                           height -= 30 ;
                        }
                        else
                        {
                           scope.Setting.grid.borderBottom = '1px solid #E3E7E8' ;
                        }
                     }
                  }
                  else
                  {
                     scope.Setting.grid.borderBottom = '1px solid #E3E7E8' ;
                  }
                  if( gridModel == 'auto' )
                  {
                     scope.Setting.grid.height = 'auto' ;
                     scope.Setting.grid.maxHeight = height + 'px' ;
                  }
                  else if( gridModel == 'maxHeight' )
                  {
                     scope.Setting.grid.height = 'auto' ;
                     scope.Setting.grid.maxHeight = scope.data.options.grid.maxHeight + 'px' ;
                  }
                  else
                  {
                     //gridModel = fixed
                     scope.Setting.grid.height = height + 'px' ;
                     scope.Setting.grid.maxHeight = 'none' ;
                  }
               } ) ;
            }
            //网格副标题追加html代码和事件
            function setGridSubTitle( scope, parentEle )
            {
               if( typeof( scope.data['subtitle'] ) == 'object' )
               {
                  var rowNum = scope.data['subtitle'].length ;
                  var tdEle = $( '> .Grid:first > .GridHeader:first > .GridTr:last > .GridTd', parentEle ) ;
                  for( var index = 0; index < rowNum; ++index )
                  {
                     if( typeof( scope.data['subtitle'][index]['html'] ) == 'string' )
                     {
                        var newEle = $compile( scope.data['subtitle'][index]['html'] )( scope ) ;
                        $( tdEle[ index ] ).append( newEle ) ;
                     }
                     else if( typeof( scope.data['subtitle'][index]['html'] ) == 'object' )
                     {
                        $( tdEle[ index ] ).append( scope.data['subtitle'][index]['html'] ) ;
                     }
                     else
                     {
                        $( tdEle[ index ] ).text( scope.data['subtitle'][index]['text'] ) ;
                     }
                  }
               }
            }
            //网格内容追加html代码和事件
            function setGridBody( scope, parentEle )
            {
               if( typeof( scope.data['subtitle'] ) == 'object' )
               {
                  var marTop = $( '> .Grid:first > .GridHeader:first', parentEle ).height() ;
                  $( '> .Grid:first > .GridBody:first', parentEle ).css( 'marginTop', marTop ) ;
               }
               var rowNum = scope.data['body'].length ;
               var columnNum = 0 ;
               var tdEle = $( '> .Grid:first > .GridBody:first > .GridTr > .GridTd', parentEle ) ;
               for( var index = 0; index < rowNum; ++index )
               {
                  if( index == 0 ) columnNum = scope.data['body'][index].length ;
                  for( var index2 = 0; index2 < columnNum; ++index2 )
                  {
                     if( typeof( scope.data['body'][index][index2]['html'] ) == 'string' )
                     {
                        var newEle = $compile( scope.data['body'][index][index2]['html'] )( scope ) ;
                        $( tdEle[ index * columnNum + index2 ] ).append( newEle ) ;
                        scope.data['body'][index][index2]['string'] = $( newEle ).text() ;
                     }
                     else if( typeof( scope.data['body'][index][index2]['html'] ) == 'object' )
                     {
                        $( tdEle[ index * columnNum + index2 ] ).append( scope.data['body'][index][index2]['html'] ) ;
                        scope.data['body'][index][index2]['string'] = $( scope.data['body'][index][index2]['html'] ).text() ;
                     }
                     else
                     {
                        $( tdEle[ index * columnNum + index2 ] ).text( scope.data['body'][index][index2]['text'] ) ;
                        scope.data['body'][index][index2]['string'] = scope.data['body'][index][index2]['text'] ;
                     }
                  }
               }
            }
            //网格工具栏追加html代码和事件
            function setGridTool( scope, parentEle )
            {
               if( typeof( scope.data['tool'] ) != 'undefined' )
               {
                  if( typeof( scope.data['tool']['left'] ) != 'undefined' )
                  {
                     $.each( scope.data['tool']['left'], function( index, left ){
                        if( typeof( left['html'] ) == 'string' )
                        {
                           var ele = $( '> .GridTool > .ToolLeft > span:eq(' + index + ')', parentEle ) ;
                           var newEle = $compile( left['html'] )( scope ) ;
                           angular.element( ele.get(0) ).empty() ;
                           angular.element( ele.get(0) ).append( newEle ) ;
                        }
                        else if( typeof( left['html'] ) == 'object' )
                        {
                           var ele = $( '> .GridTool > .ToolLeft > span:eq(' + index + ')', parentEle ) ;
                           var newEle = left['html'] ;
                           $( ele ).empty() ;
                           $( ele ).append( newEle ) ;
                        }
                     } ) ;
                  }
                  if( typeof( scope.data['tool']['right'] ) != 'undefined' )
                  {
                     $.each( scope.data['tool']['right'], function( index, right ){
                        if( typeof( right['html'] ) == 'string' )
                        {
                           var ele = $( '> .GridTool > .ToolRight > span:eq(' + index + ')', parentEle ) ;
                           var newEle = $compile( right['html'] )( scope ) ;
                           angular.element( ele.get(0) ).empty() ;
                           angular.element( ele.get(0) ).append( newEle ) ;
                        }
                        else if( typeof( right['html'] ) == 'object' )
                        {
                           var ele = $( '> .GridTool > .ToolRight > span:eq(' + index + ')', parentEle ) ;
                           var newEle = right['html'] ;
                           $( ele ).empty() ;
                           $( ele ).append( newEle ) ;
                        }
                     } ) ;
                  }
               }
            }
            //清除网格内容
            function clearup( scope, parentEle )
            {
               $( '> .Grid:first > .GridBody:first > .GridTr > .GridTd', parentEle ).empty() ;
            }
            //执行Resize事件
            function onResize( scope, widthArr, heightArr )
            {
               if( scope.data['options']['event'] && typeof( scope.data['options']['event']['onResize'] ) == 'function' )
               {
                  var column = scope.data['title'].length ;
                  $.each( scope.data['body'], function( y, columnInfo ){
                     $.each( columnInfo, function( x ){
                        scope.data['options']['event']['onResize']( x, y, scope.Setting.bodyWidth[x], scope.Setting.grid.tdHeight[y] ) ;
                     } ) ;
                  } ) ;
               }
            }
            return {
               pre: function preLink( scope, element, attributes ){
                  $( element ).css( 'position', 'relative' ) ;
                  var gridOnResize = function () {
                     if( !scope.data )
                     {
                        return;
                     }
                     //设置列宽
                     setColumnWidth( scope, element ) ;
                     //设置行高
                     setRowHeight( scope, element ) ;
                     //设置总高度
                     setGridHeight( scope, element ) ;
                     //响应事件
                     onResize( scope ) ;
                  } ;
                  if( scope.data )
                  {
                     scope.data.onResize = gridOnResize ;
                  }
                  angular.element( $window ).bind( 'resize', gridOnResize ) ;
                  var listener1 = $rootScope.$watch( 'onResize', function(){
                     setTimeout( gridOnResize ) ;
                  } ) ;
                  var browserInfo = SdbFunction.getBrowserInfo() ;
                  var listener2 = scope.$watch( 'data', function(){
                     //清除网格内容
                     if( typeof( scope.data ) == 'object' )
                     {
                        scope.data.onResize = gridOnResize ;
                        clearup( scope, element ) ;
                        $( '> .Grid:first', element ).css( 'visibility', 'hidden' ) ;
                        $( '> .GridTool:first', element ).css( 'visibility', 'hidden' ) ;
                        setTimeout( function(){
                           //网格副标题追加html代码和事件
                           setGridSubTitle( scope, element ) ;
                           //网格内容追加html代码和事件
                           setGridBody( scope, element ) ;
                           //网格工具栏追加html代码和事件
                           setGridTool( scope, element ) ;
                           //设置列宽
                           setColumnWidth( scope, element, ( browserInfo[0] == 'firefox' ) ) ;
                           //设置行高
                           setRowHeight( scope, element ) ;
                           //设置总高度
                           setGridHeight( scope, element ) ;
                           //响应事件
                           onResize( scope ) ;
                           $( '> .Grid:first', element ).css( 'visibility', 'visible' ) ;
                           $( '> .GridTool:first', element ).css( 'visibility', 'visible' ) ;
                        } ) ;
                     }
                  } ) ;
                  var listener3 = scope.$watch( 'data.body', function(){
                     //清除网格内容
                     if( typeof( scope.data ) == 'object' )
                     {
                        scope.data.onResize = gridOnResize ;
                        clearup( scope, element ) ;
                        $( '> .Grid:first', element ).css( 'visibility', 'hidden' ) ;
                        $( '> .GridTool:first', element ).css( 'visibility', 'hidden' ) ;
                        setTimeout( function(){
                           //网格内容追加html代码和事件
                           setGridBody( scope, element ) ;
                           //网格工具栏追加html代码和事件
                           setGridTool( scope, element ) ;
                           //设置列宽
                           setColumnWidth( scope, element, ( browserInfo[0] == 'firefox' ) ) ;
                           //设置行高
                           setRowHeight( scope, element ) ;
                           //设置总高度
                           setGridHeight( scope, element ) ;
                           //响应事件
                           onResize( scope ) ;
                           $( '> .Grid:first', element ).css( 'visibility', 'visible' ) ;
                           $( '> .GridTool:first', element ).css( 'visibility', 'visible' ) ;
                        } ) ;
                     }
                  } ) ;
                  scope.$on( '$destroy', function(){
                     angular.element( $window ).unbind( 'resize', gridOnResize ) ;
                     listener1() ;
                     listener2() ;
                     listener3() ;
                  } ) ;
               },
               post: function postLink( scope, element, attributes ){
                  //添加排序事件
                  var removeWatch = scope.$watch( 'data', function(){
                     if( typeof( scope.data ) == 'object' )
                     {
                        setTimeout( function(){
                           if( typeof( scope.data['options'] ) == 'object' &&
                               typeof( scope.data['options']['order'] ) != 'undefined' &&
                               scope.data['options']['order']['active'] == true )
                           {
                              var GridHeaderTd = [] ;
                              $.each( scope.data['title'], function( index ){
                                 var tdEle = $( '> .Grid > .GridHeader > .GridTr:eq(0) > .GridTd:eq(' + index + ')', element ).css( 'cursor', 'pointer' ) ;
                                 GridHeaderTd.push( tdEle ) ;
                                 var lastColumn = -1 ;
                                 $( tdEle ).bind( 'click', function(){
                                    (function( ele, column ){
                                       if( lastColumn != column )
                                       {
                                          scope.Grid.reverse = true ;
                                       }
                                       else
                                       {
                                          scope.Grid.reverse = !scope.Grid.reverse ;
                                       }
                                       lastColumn = column ;
                                       var orderBy = $filter( 'orderObjectBy' ) ;
                                       scope.data['body'] = orderBy( scope.data['body'], column + '.string', scope.Grid.reverse ) ;
                                       var g = ele ;
                                       var caret = $( ' > .fa', g ) ;
                                       var isDown = true ;
                                       if( caret.length > 0 )
                                       {
                                          isDown = caret.hasClass( 'fa-caret-up' ) ;
                                       }
                                       $.each( GridHeaderTd, function( index, ele ){
                                          $( ' > .fa ', ele ).remove() ;
                                       } ) ;
                                       if( isDown )
                                       {
                                          $( g ).prepend( '<i class="fa fa-lg fa-caret-down"></i>' ) ;
                                       }
                                       else
                                       {
                                          $( g ).prepend( '<i class="fa fa-lg fa-caret-up"></i>' ) ;
                                       }
                                    }( this, index )) ;
                                    setGridBody( scope, element ) ;
                                 } ) ;
                              } ) ;
                           }
                        } ) ;
                     }
                  } ) ;
                  scope.$on( '$destroy', function(){
                     removeWatch() ;
                  } ) ;
               }
            } ;
         }
      } ;
      return dire ;
   });
  
   //创建网格的指令(需要html构造)(准备废弃)
   sacApp.directive( 'ngGrid', function( $filter, $compile, $window, $rootScope, SdbFunction ){
      var browserInfo = SdbFunction.getBrowserInfo() ;
      var dire = {
         restrict: 'A',
         scope: {
            data: '=ngGrid'
         },
         replace: false,
         controller: function( $scope, $element ){
            $scope.isFirst_firefox = 0 ;
            $scope.Setting = {
               titleWidth: [],
               bodyWidth: [],
               tdHeight: []
            } ;
         },
         compile: function( element, attributes ){
            //设置表格头列宽高
            function setHeader( scope, gridEle, isFirefox )
            {
               var bodyEle    = $( '> .GridBody:first', gridEle ) ;
               var titleTdEle = $( '> .GridHeader:first > .GridTr:first > .GridTd', gridEle ) ;
               var subTitleTdEle = $( '> .GridHeader:first > .GridTr:last > .GridTd', gridEle ) ;
               var tdBorder   = numberCarry( $( titleTdEle[0] ).outerWidth() - $( titleTdEle[0] ).width() ) ;
               var titleNum   = titleTdEle.length ;
               var width      = parseInt( $( gridEle ).outerWidth() - numberCarry( $( gridEle ).outerWidth() - $( gridEle ).width() ) - titleNum * tdBorder ) ;
               var titleWidth = scope.data ? scope.data.titleWidth : undefined ;
               var bodyWidth = parseInt( $( bodyEle ).outerWidth() - titleNum * tdBorder ) ;
               if( isFirefox == true  && scope.isFirst_firefox < 2 )
               {
                  bodyWidth -= 17 ;
               }
               var scrollWidth = width - bodyWidth ;
               var sumWidth = 0 ;
               var cursorTitleWidth = [] ;
               if( typeof( titleWidth ) == 'undefined' || titleWidth == 'auto' )
               {
                  var aveWidth = parseInt( width / titleNum ) ;
                  for( var i = 0; i < titleNum; ++i )
                  {
                     //标题
                     var tmpTdWidth = aveWidth ;
                     if( i + 1 == titleNum )
                     {
                        tmpTdWidth = width - sumWidth - scrollWidth ;
                     }
                     cursorTitleWidth.push( tmpTdWidth ) ;
                     sumWidth += tmpTdWidth ;
                  }
                  for( var c = 0; c < titleNum; ++c )
                  {
                     $( titleTdEle[c] ).width( cursorTitleWidth[c] ) ;
                     if( subTitleTdEle.length == titleNum )
                     {
                        $( subTitleTdEle[c] ).width( cursorTitleWidth[c] ) ;
                     }
                  }
               }
               else
               {
                  for( var index = 0; index < titleNum; ++index )
                  {
                     cursorTitleWidth.push( 0 ) ;
                  }
                  var lastIndex = 0 ;
                  for( var index = 0; index < titleNum; ++index )
                  {
                     if( typeof( titleWidth[index] ) == 'string' )
                     {
                        //标题
                        var tmpTdWidth = parseInt( titleWidth[index] ) ;
                        var bodyTdWidth = tmpTdWidth ;
                        if( index + 1 == titleNum )
                        {
                           tmpTdWidth += scrollWidth ;
                        }
                        cursorTitleWidth[index] = ( tmpTdWidth + 'px' ) ;
                        sumWidth += tmpTdWidth ;
                     }
                     else if( typeof( titleWidth[index] ) == 'number' )
                     {
                        lastIndex = index ;
                     }
                  }
                  var lastSumWidth = width - sumWidth ;
                  for( var index = 0; index < titleNum; ++index )
                  {
                     if( typeof( titleWidth[index] ) == 'number' )
                     {
                        //标题
                        var tmpTdWidth = 0 ;
                        tmpTdWidth = parseInt( titleWidth[index] * lastSumWidth * 0.01 ) ;
                        var bodyTdWidth = tmpTdWidth ;
                        if( index == lastIndex )
                        {
                           tmpTdWidth = width - sumWidth - scrollWidth ;
                        }
                        if( tmpTdWidth < 0 ) tmpTdWidth = 0 ;
                        cursorTitleWidth[index] = ( tmpTdWidth + 'px' ) ;
                        sumWidth += tmpTdWidth ;
                     }
                  }
                  for( var c = 0; c < titleNum; ++c )
                  {
                     $( titleTdEle[c] ).width( cursorTitleWidth[c] ) ;
                     if( subTitleTdEle.length == titleNum )
                     {
                        $( subTitleTdEle[c] ).width( cursorTitleWidth[c] ) ;
                     }
                  }
               }
            }
            //设置列宽
            function setColumnWidth( scope, gridEle, isFirefox )
            {
               var bodyEle    = $( '> .GridBody:first', gridEle ) ;
               var titleTdEle = $( '> .GridHeader:first > .GridTr:first > .GridTd', gridEle ) ;
               var bodyTrEle  = $( '> .GridTr', bodyEle ) ;
               var bodyTdEle  = $( '> .GridTd', bodyTrEle ) ;
               var tdBorder   = numberCarry( $( titleTdEle[0] ).outerWidth() - $( titleTdEle[0] ).width() ) ;
               var titleNum   = titleTdEle.length ;
               var rowNum     = bodyTrEle.length ;
               var width      = parseInt( $( gridEle ).outerWidth() - numberCarry( $( gridEle ).outerWidth() - $( gridEle ).width() ) - titleNum * tdBorder ) ;
               var bodyWidth = parseInt( $( bodyEle ).outerWidth() - titleNum * tdBorder ) ;
               if( isFirefox == true  && scope.isFirst_firefox < 2 )
               {
                  ++scope.isFirst_firefox ;
                  bodyWidth -= 17 ;
               }
               var scrollWidth = width - bodyWidth ;
               var titleWidth = scope.data.titleWidth ;
               var sumWidth = 0 ;
               var sumBodyWidth = 0 ;
               var cursorBodyWidth  = [] ;
               if( typeof( titleWidth ) == 'undefined' || titleWidth == 'auto' )
               {
                  var aveWidth = parseInt( width / titleNum ) ;
                  for( var i = 0; i < titleNum; ++i )
                  {
                     //内容
                     tmpTdWidth = aveWidth ;
                     if( i + 1 == titleNum )
                     {
                        tmpTdWidth = bodyWidth - sumBodyWidth ;
                     }
                     cursorBodyWidth.push( tmpTdWidth + 'px' ) ;
                     sumBodyWidth += tmpTdWidth ;
                  }
                  for( var r = 0; r < rowNum; ++r )
                  {
                     for( var c = 0; c < titleNum; ++c )
                     {
                        $( bodyTdEle[ r * titleNum + c ] ).width( cursorBodyWidth[c] ) ;
                     }
                  }
               }
               else
               {
                  var titleWidthLen = titleWidth.length ;
                  for( var index = 0; index < titleWidthLen; ++index )
                  {
                     cursorBodyWidth.push( 0 ) ;
                  }
                  var lastIndex = 0 ;
                  for( var index = 0; index < titleWidthLen; ++index )
                  {
                     if( typeof( titleWidth[index] ) == 'string' )
                     {
                        tmpTdWidth = parseInt( titleWidth[index] ) ;
                        sumBodyWidth += tmpTdWidth ;
                        cursorBodyWidth[index] = tmpTdWidth ;
                        sumWidth += tmpTdWidth ;
                     }
                     else if( typeof( titleWidth[index] ) == 'number' )
                     {
                        lastIndex = index ;
                     }
                  }
                  var lastSumWidth = width - sumWidth ;
                  for( var index = 0; index < titleWidthLen; ++index )
                  {
                     if( typeof( titleWidth[index] ) == 'number' )
                     {
                        tmpTdWidth = parseInt( titleWidth[index] * lastSumWidth * 0.01 ) ;
                        if( index == lastIndex )
                        {
                           tmpTdWidth = bodyWidth - sumBodyWidth ;
                        }
                        if( tmpTdWidth < 0 ) tmpTdWidth = 0 ;
                        cursorBodyWidth[index] = tmpTdWidth ;
                        sumBodyWidth += tmpTdWidth ;
                     }
                  }
                  for( var r = 0; r < rowNum; ++r )
                  {
                     for( var c = 0; c < titleNum; ++c )
                     {
                        $( bodyTdEle[ r * titleNum + c ] ).width( cursorBodyWidth[c] ) ;
                     }
                  }
               }
            }
            //设置行高
            function setRowHeight( scope, gridEle )
            {
               var bodyEle = $( '> .GridBody:first', gridEle ) ;
               var model = scope.data.tdModel ;
               if( typeof( model ) == 'undefined' ) model = 'auto' ;
               scope.Setting.tdHeight = [] ;
               if( model == 'dynamic' )
               {
                  $.each( scope.data['body'], function( index, row ){
                     scope.Setting.grid.tdHeight.push( 'auto' ) ;
                  } ) ;
               }
               else if( model == 'fixed' && typeof( scope.data.tdHeight ) != 'undefined' )
               {
                  if( typeof( scope.data.options.grid.tdHeight ) == 'string' )
                  {
                     $.each( scope.data['body'], function( index, row ){
                        scope.Setting.grid.tdHeight.push( scope.data.options.grid.tdHeight ) ;
                     } ) ;
                  }
                  else if( typeof( scope.data.options.grid.tdHeight ) == 'number' )
                  {
                     $.each( scope.data['body'], function( index, row ){
                        scope.Setting.grid.tdHeight.push( scope.data.options.grid.tdHeight + 'px' ) ;
                     } ) ;
                  }
               }
               else if( model == 'auto' )
               {
                  var titleEle  = $( '> .GridHeader:first > .GridTr:first > .GridTd', gridEle ) ;
                  var bodyTrEle = $( '> .GridTr', bodyEle ) ;
                  var bodyTdEle = $( '> .GridTd', bodyTrEle ) ;
                  var rowNum    = bodyTrEle.length ;
                  var columnNum = titleEle.length ;
                  for( var index = 0; index < rowNum; ++index )
                  {
                     var maxHeight = 0 ;
                     for( var index2 = 0; index2 < columnNum; ++index2 )
                     {
                        $( bodyTdEle[ index * columnNum + index2 ] ).css( 'height', 'auto' ) ;
                        var tdHeight = $( bodyTdEle[ index * columnNum + index2 ] ).height() ;
                        if( tdHeight > maxHeight )
                        {
                           maxHeight = tdHeight ;
                        }
                     }
                     for( var index2 = 0; index2 < columnNum; ++index2 )
                     {
                        $( bodyTdEle[ index * columnNum + index2 ] ).height( maxHeight ) ;
                     }
                  }
               }
            }
            return {
               pre: function preLink( scope, element, attributes ){
                  $( element ).parent().css( 'position', 'relative' ) ;
                  var gridOnResize = function ( width, height ) {
                     if( typeof( height ) == 'number' )
                     {
                        $( element ).css( 'height', height ) ;
                     }
                     //设置表格头
                     setHeader( scope, element, ( browserInfo[0] == 'firefox' ) ) ;
                     //设置列宽
                     setColumnWidth( scope, element, ( browserInfo[0] == 'firefox' ) ) ;
                     //设置行高
                     setRowHeight( scope, element ) ;
                  } ;
                  if( scope.data )
                  {
                     scope.data.onResize = gridOnResize ;
                  }
                  setHeader( scope, element ) ;
                  angular.element( $window ).bind( 'resize', gridOnResize ) ;
                  var listener1 = $rootScope.$watch( 'onResize', function(){
                     setTimeout( gridOnResize ) ;
                  } ) ;
                  var listener2 = scope.$watch( 'data', function(){
                     //清除网格内容
                     if( typeof( scope.data ) == 'object' )
                     {
                        setTimeout( function(){
                           //设置表格头
                           setHeader( scope, element ) ;
                           //设置列宽
                           setColumnWidth( scope, element, ( browserInfo[0] == 'firefox' ) ) ;
                           //设置行高
                           setRowHeight( scope, element ) ;
                           if( scope.data.isShow )
                           {
                              $( '> .GridBody:first', element ).css( 'visibility', 'visible' ) ;
                           }
                        } ) ;
                     }
                  } ) ;
                  var listener3 = scope.$watch( 'data.titleWidth', function(){
                     //清除网格内容
                     if( typeof( scope.data ) == 'object' )
                     {
                        $( element ).css( 'visibility', 'hidden' ) ;
                        setTimeout( function(){
                           //设置表格头
                           setHeader( scope, element ) ;
                           //设置列宽
                           setColumnWidth( scope, element, ( browserInfo[0] == 'firefox' ) ) ;
                           //设置行高
                           setRowHeight( scope, element ) ;
                           if( scope.data.isShow )
                           {
                              $( '> .GridBody:first', element ).css( 'visibility', 'visible' ) ;
                           }
                           $( element ).css( 'visibility', 'visible' ) ;
                        } ) ;
                     }
                  } ) ;
                  scope.$on( '$destroy', function(){
                     angular.element( $window ).unbind( 'resize', gridOnResize ) ;
                     listener1() ;
                     listener2() ;
                     listener3() ;
                  } ) ;
               },
               post: function postLink( scope, element, attributes ){}
            } ;
         }
      } ;
      return dire ;
   });

   /*
   <div class="Grid2" ng-grid2="xxx" ng-container="{}">
      <div class="Grid2Header">
         <table>
            <colgroup>
               <col />
            </colgroup>
            <tbody>
               <tr>
                  <td>
                     <div class="Ellipsis">xxxxx</div>
                  </td>
               </tr>
            </tbody>
         </table>
      </div>
      <div class="Grid2Body">
         <table>
            <colgroup>
               <col />
            </colgroup>
            <tbody>
               <tr>
                  <td>
                     <div class="Ellipsis">
                        xxxx
                     </div>
                  </td>
               </tr>
            </tbody>
         </table>
      </div>
   </div>
   <div class="GridTool">
      <div class="ToolLeft">xxxxx</div>
   </div>
   */
   //创建网格的指令(需要html构造)(准备废弃)
   sacApp.directive( 'ngGrid2', function( $window, $rootScope, SdbFunction ){
      var browserInfo = SdbFunction.getBrowserInfo() ;
      var dire = {
         restrict: 'A',
         scope: {
            data: '=ngGrid2'
         },
         replace: false,
         // 专用控制器
         controller: function( $scope, $element ){
         },
         // 编译
         compile: function( element, attributes ){
            //设置表格列宽
            function setColumn( scope, gridEle, isFirefox )
            {
               var bodyDiv        = $( '> .Grid2Body', gridEle ) ;
               var headerTable    = $( '> .Grid2Header > table', gridEle ).css( 'width', 'auto' ) ;
               var headerTd       = $( '> tbody:first > tr:first > td:first', headerTable ) ;
               var bodyTable      = $( '> table', bodyDiv ).hide() ;
               var headerTableCol = $( '> colgroup > col', headerTable ) ;
               var bodyTableCol   = $( '> colgroup > col', bodyTable ) ;
               var width          = bodyDiv.width() ;
               var titleWidth     = scope.data ? scope.data.titleWidth : undefined ;
               var titleNum       = headerTableCol.length ;
               var tdBorder       = numberCarry( $( headerTd ).outerWidth() - $( headerTd ).width() ) ;
               var borderWidth    = parseInt( $( bodyDiv ).outerWidth() - titleNum * tdBorder ) ;
               //设置header表格的宽度
               headerTable.width( width ) ;
               //设置body表格的宽度
               bodyTable.width( width ).show() ;
               width = borderWidth ;
               if( typeof( titleWidth ) == 'undefined' || titleWidth == 'auto' )
               {
                  var aveWidth = parseInt( width / titleNum ) ;
                  $( headerTableCol ).width( aveWidth ) ;
                  $( bodyTableCol ).width( aveWidth ) ;
               }
               else
               {
                  var sumWidth = 0 ;
                  var cursorTitleWidth = [] ;
                  for( var index = 0; index < titleNum; ++index )
                  {
                     cursorTitleWidth.push( 0 ) ;
                  }
                  var lastIndex = 0 ;
                  for( var index = 0; index < titleNum; ++index )
                  {
                     if( typeof( titleWidth[index] ) == 'string' )
                     {
                        //标题
                        var tmpTdWidth = parseInt( titleWidth[index] ) ;
                        var bodyTdWidth = tmpTdWidth ;
                        cursorTitleWidth[index] = ( tmpTdWidth + 'px' ) ;
                        sumWidth += tmpTdWidth ;
                     }
                     else if( typeof( titleWidth[index] ) == 'number' )
                     {
                        lastIndex = index ;
                     }
                  }
                  var lastSumWidth = width - sumWidth ;
                  for( var index = 0; index < titleNum; ++index )
                  {
                     if( typeof( titleWidth[index] ) == 'number' )
                     {
                        //标题
                        var tmpTdWidth = 0 ;
                        tmpTdWidth = parseInt( titleWidth[index] * lastSumWidth * 0.01 ) ;
                        var bodyTdWidth = tmpTdWidth ;
                        if( index == lastIndex )
                        {
                           tmpTdWidth = width - sumWidth ;
                        }
                        if( tmpTdWidth < 0 ) tmpTdWidth = 0 ;
                        cursorTitleWidth[index] = ( tmpTdWidth + 'px' ) ;
                        sumWidth += tmpTdWidth ;
                     }
                  }
                  for( var c = 0; c < titleNum; ++c )
                  {
                     $( headerTableCol[c] ).outerWidth( cursorTitleWidth[c] ) ;
                     $( bodyTableCol[c] ).outerWidth( cursorTitleWidth[c] ) ;
                  }
               }
            }
            return {
               pre: function preLink( scope, element, attributes ){
                  $( element ).parent().css( 'position', 'relative' ) ;
               },
               post: function postLink( scope, element, attributes ){
                  //绑定事件等
                  var gridOnResize = function ( width, height ) {
                     if( typeof( height ) == 'number' )
                     {
                        $( element ).css( 'height', height ) ;
                     }
                     setColumn( scope, element, ( browserInfo[0] == 'firefox' ) ) ;
                  } ;
                  if( scope.data )
                  {
                     scope.data.onResize = gridOnResize ;
                  }
                  setColumn( scope, element ) ;
                  angular.element( $window ).bind( 'resize', gridOnResize ) ;
                  var listener1 = $rootScope.$watch( 'onResize', function(){
                     setTimeout( gridOnResize ) ;
                  } ) ;
                  var listener2 = scope.$watch( 'data', function(){
                     //清除网格内容
                     if( typeof( scope.data ) == 'object' )
                     {
                        setTimeout( function(){
                           setColumn( scope, element ) ;
                        } ) ;
                     }
                  } ) ;
                  var listener3 = scope.$watch( 'data.titleWidth', function(){
                     //清除网格内容
                     if( typeof( scope.data ) == 'object' )
                     {
                        setTimeout( function(){
                           setColumn( scope, element ) ;
                        } ) ;
                     }
                  } ) ;
                  scope.$on( '$destroy', function(){
                     angular.element( $window ).unbind( 'resize', gridOnResize ) ;
                     listener1() ;
                     listener2() ;
                     listener3() ;
                  } ) ;
               }
            } ;
         }
      } ;
      return dire ;
   } ) ;

   //弹窗(准备废弃)
   sacApp.directive( 'createModal', function( $compile, $window, $rootScope, Tip ){
      var dire = {
         restrict: 'A',
         scope: {
            data: '=para'
         },
         templateUrl: './app/template/Component/Modal.html',
         replace: false,
         controller: function( $scope, $element ){
            $scope.Setting = {
               //弹窗状态，1是普通，2是最大化
               Status: 1,
               //弹窗样式
               Style: {
                  width: '0px',
                  height: '0px',
                  top: '0px',
                  left: '0px'
               },
               BodyStyle: {
                  width: '0px',
                  height: '0px'
               },
               //文本
               Text: {
                  OK: $rootScope.autoLanguage( '确定' ),
                  Close: $rootScope.autoLanguage( '取消' )
               },
               //遮罩
               Mask: $( '<div></div>' ).attr( 'ng-mousedown', 'prompt()').addClass( 'mask-screen unalpha' ),
               //临时数据
               Temp: {
                  left: '0px',
                  top: '0px',
                  width: '0px',
                  height: '0px',
                  x: 0,
                  y: 0
               }
            } ;
         },
         compile: function( element, attributes ){
            function modalResize( scope )
            {
               if( typeof( scope.data.Style ) == 'function' )
               {
                  function setResize()
                  {
                     scope.Setting.Style = scope.data.Style() ;
                     scope.Setting.BodyStyle.width = ( parseInt( scope.Setting.Style.width ) - 42 ) + 'px' ;
                     scope.Setting.BodyStyle.height = ( parseInt( scope.Setting.Style.height ) - 126 ) + 'px' ;
                     if( typeof( scope.data.onResize ) == 'function' )
                     {
                        scope.data.onResize( parseInt( scope.Setting.BodyStyle.width ), parseInt( scope.Setting.BodyStyle.height ) ) ;
                     }
                  }
                  setResize() ;
                  angular.element( $window ).bind( 'resize', function () {
                     setResize() ;
                  } ) ;
               }
               else
               {
                  function autoResize()
                  {
                     var bodyWidth = $( window ).width() ;
                     var bodyHeight = $( window ).height() ;
                     var width = bodyWidth * 0.5 ;
                     var height = bodyHeight * 0.5 ;
                     if( width < 600 ) width = 600 ;
                     if( height < 450 ) height = 450 ;
                     var left = ( bodyWidth - width ) * 0.5 ;
                     var top = ( bodyHeight - height ) * 0.5 ;
                     scope.Setting.Temp.left = left + 'px' ;
                     scope.Setting.Temp.top = top + 'px' ;
                     scope.Setting.Temp.width = width + 'px' ;
                     scope.Setting.Temp.height = height + 'px' ;
                     scope.Setting.Style.width = width + 'px' ;
                     scope.Setting.Style.height = height + 'px' ;
                     scope.Setting.Style.left = left + 'px' ;
                     scope.Setting.Style.top = top + 'px' ;
                     width -= 42 ;
                     height -= 126 ;
                     scope.Setting.BodyStyle.width = width + 'px' ;
                     scope.Setting.BodyStyle.height = height + 'px' ;
                     if( typeof( scope.data.onResize ) == 'function' )
                     {
                        scope.data.onResize( parseInt( scope.Setting.BodyStyle.width ), parseInt( scope.Setting.BodyStyle.height ) ) ;
                     }
                  }
                  autoResize() ;
               }
            }
            return {
               pre: function preLink( scope, element, attributes ){
                  var onResize = function () {
                     modalResize( scope ) ;
                     if( scope.Setting.Status == 2 )
                     {
                        scope.maximumModal() ;
                     }
                     else
                     {
                        scope.recoveryModal() ;
                     }
                     setTimeout( function(){
                        if( scope.Setting.Status == 2 )
                        {
                           scope.maximumModal() ;
                        }
                        else
                        {
                           scope.recoveryModal() ;
                        }
                     } ) ;
                  }
                  //更新弹窗宽度高度坐标
                  var listener1 = scope.$watch( 'data', function(){
                     if( typeof( scope.data ) == 'object' )
                     {
                        modalResize( scope ) ;
                     }
                  } ) ;
                  angular.element( $window ).bind( 'resize', onResize ) ;
                  var listener2 = $rootScope.$on( '$locationChangeStart', function(){
                     scope.data.isShow = false ;
                  } ) ;
                  scope.$on( '$destroy', function(){
                     angular.element( $window ).unbind( 'resize', onResize ) ;
                     listener1() ;
                     listener2() ;
                  } ) ;
               },
               post: function postLink( scope, element, attributes ){
                  //编译内容
                  scope.$watch( 'data.isShow', function(){
                     if( typeof( scope.data ) == 'object' )
                     {
                        if( scope.data.isShow == true )
                        {
                           scope.data.isClose = false ;
                           scope.Setting.BodyStyle['overflow-y'] = scope.data.isScroll == false ? 'hidden' : 'auto' ;
                           var timer = null ;
                           timer = setTimeout( function(){
                              $( document.body ).append( $compile( scope.Setting.Mask )( scope ) ) ;
                              var bodyEle = $( '> .modal2 > .body', element ) ;
                              var contextType = typeof( scope.data.Context ) ;
                              if( contextType == 'string' )
                              {
                                 bodyEle.html( $compile( scope.data.Context )( scope ) ) ;
                              }
                              else if( contextType == 'object' )
                              {
                                 bodyEle.html( scope.data.Context ) ;
                              }
                              else if( contextType == 'function' )
                              {
                                 $compile( scope.data.Context( bodyEle ) )( scope ) ;
                              }
                              if( typeof( scope.data.onResize ) == 'function' )
                              {
                                 scope.data.onResize( parseInt( scope.Setting.BodyStyle.width ), parseInt( scope.Setting.BodyStyle.height ) ) ;
                              }
                              modalResize( scope ) ;
                              scope.$apply() ;
                              clearTimeout( timer ) ;
                              timer = null ;
                           } ) ;
                        }
                        else
                        {
                           scope.recoveryModal() ;
                           scope.Setting.Mask.detach() ;
                           scope.data.noOK = false ;
                           scope.data.isClose = true ;
                        }
                     }
                  } ) ;

                  scope.$watch( 'data.isRepaint', function(){
                     if( typeof( scope.data ) == 'object' )
                     {
                        if( scope.data.isShow == true && typeof( scope.data.isRepaint ) == 'number' )
                        {
                           scope.data.isRepaint = null ;
                           scope.Setting.BodyStyle['overflow-y'] = scope.data.isScroll == false ? 'hidden' : 'auto' ;
                           $( document.body ).append( $compile( scope.Setting.Mask )( scope ) ) ;
                           var bodyEle = $( '> .modal2 > .body', element ) ;
                           var contextType = typeof( scope.data.Context ) ;
                           if( contextType == 'string' )
                           {
                              bodyEle.html( $compile( scope.data.Context )( scope ) ) ;
                           }
                           else if( contextType == 'object' )
                           {
                              bodyEle.html( scope.data.Context ) ;
                           }
                           else if( contextType == 'function' )
                           {
                              $compile( scope.data.Context( bodyEle ) )( scope ) ;
                           }
                           if( typeof( scope.data.onResize ) == 'function' )
                           {
                              scope.data.onResize( parseInt( scope.Setting.BodyStyle.width ), parseInt( scope.Setting.BodyStyle.height ) ) ;
                           }
                           modalResize( scope ) ;
                        }
                     }
                  } ) ;

                  //闪烁
                  scope.prompt = function(){
                     var counter = 0 ;
                     var bodyEle = $( '> .modal2 ', element ) ;
                     var timer = setInterval( function(){
                        ++counter ;
                        if( counter == 1 )
                        {
                           $( bodyEle ).css( 'box-shadow','none' );
                        }
                        else if( counter == 2 )
                        {
                           $( bodyEle ).css( 'box-shadow','0px 2px 8px rgba(0,0,0,0.5)' );
                        }
                        if( counter == 3 )
                        {
                           $( bodyEle ).css( 'box-shadow','none' );
                        }
                        else if( counter == 4 )
                        {
                           $( bodyEle ).css( 'box-shadow','0px 2px 8px rgba(0,0,0,0.5)' );
                        }
                        if( counter == 5 )
                        {
                           $( bodyEle ).css( 'box-shadow','none' );
                        }
                        else if( counter == 6 )
                        {
                           $( bodyEle ).css( 'box-shadow','0px 2px 8px rgba(0,0,0,0.5)' );
                           counter = 0 ;
                           clearInterval( timer );
                        }
                     }, 90 ) ;
                  }

                  //移动弹窗开始
                  scope.startMove = function( event ){
                     if( scope.Setting.Status == 1 )
                     {
                        var modal = $( '> .modal2', element ) ;
                        var pageX = event['pageX'] ;
                        var pageY = event['pageY'] ;
                        scope.Setting.Temp.x = pageX ;
                        scope.Setting.Temp.y = pageY ;
                        $( document.body ).addClass( 'unselect' ) ;
                        //监听鼠标移动
                        $( document ).on( 'mousemove', function( event2 ){
                           modal.addClass( 'alpha' ) ;
                           scope.moveModal( event2 ) ;
                        } ) ;
                        //监听鼠标松开
                        $( document ).on( 'mouseup', function(){
                           scope.endMove() ;
                        } ) ;
                     }
                  }

                  //移动
                  scope.moveModal = function( event ){
                     var modal = $( '> .modal2', element ) ;
                     if( modal.hasClass( 'alpha' ) )
                     {
                        var bodyWidth = $( window ).width() ;
                        var bodyHeight = $( window ).height() ;
                        var modalWidth = modal.width() ;
                        var modalHeight = modal.height() ;
                        var x = scope.Setting.Temp.x ;
                        var y = scope.Setting.Temp.y ;
                        var pageX = event['pageX'] ;
                        var pageY = event['pageY'] ;
                        scope.Setting.Temp.x = pageX ;
                        scope.Setting.Temp.y = pageY ;
                        var left = parseInt( modal.css( 'left' ) ) ;
                        var top = parseInt( modal.css( 'top' ) ) ;
                        var offsetLeft = pageX - x ;
                        var offsetTop = pageY - y ;
                        top  += offsetTop ;
                        left += offsetLeft ;
                        if( top <= 0 ) top = 0 ;
                        if( left <= 0 ) left = 0 ;
                        if( top + modalHeight + 5 >= bodyHeight ) top = bodyHeight - modalHeight - 5 ;
                        if( left + modalWidth + 5 >= bodyWidth ) left = bodyWidth - modalWidth - 5 ;
                        scope.Setting.Temp.left = left + 'px' ;
                        scope.Setting.Temp.top = top + 'px' ;
                        modal.css( { top: top, left: left } ) ;
                     }
                  }

                  //结束移动
                  scope.endMove = function(){
                     var modal = $( '> .modal2', element ) ;
                     modal.removeClass( 'alpha' ) ;
                     $( document.body ).removeClass( 'unselect' ) ;
                     $( document ).off( 'mousemove' ) ;
                     $( document ).off( 'mouseup' ) ;
                     Tip.auto() ;
                  }

                  //开始调整窗口大小
                  scope.startSetSize = function( event ){
                     var modal = $( '> .modal2', element ) ;
                     $( document.body ).addClass( 'unselect' ) ;
                     //监听鼠标移动
                     $( document ).on( 'mousemove', function( event2 ){
                        modal.addClass( 'alpha' ) ;
                        scope.setSize( event2 ) ;
                     } ) ;
                     //监听鼠标松开
                     $( document ).on( 'mouseup', function(){
                        scope.endSetSize() ;
                     } ) ;
                  }

                  //正在调整窗口大小
                  scope.setSize = function( event ){
                     var bodyWidth = $( window ).width() ;
                     var bodyHeight = $( window ).height() ;
                     var modal = $( '> .modal2', element ) ;
                     var body = $( '> .modal2 > .body', element ) ;
                     var left = parseInt( scope.Setting.Temp.left ) ;
                     var top = parseInt( scope.Setting.Temp.top ) ;
                     var pageX = event['pageX'] + 3 ;
                     var pageY = event['pageY'] + 5 ;
                     var width = pageX - left ;
                     var height = pageY - top ;
                     if( width < 600 ) width = 600 ;
                     if( height < 450 ) height = 450 ;
                     if( top + height + 10 >= bodyHeight ) height = bodyHeight - top - 10 ;
                     if( left + width + 10 >= bodyWidth ) width = bodyWidth - left - 10 ;
                     scope.Setting.Temp.width = width + 'px' ;
                     scope.Setting.Temp.height = height + 'px' ;
                     modal.width( width ).height( height ) ;
                     width -= 42 ;
                     height -= 126 ;
                     body.width( width ).height( height ) ;
                     scope.Setting.BodyStyle.width = width + 'px' ;
                     scope.Setting.BodyStyle.height = height + 'px' ;
                  }

                  //结束调整窗口大小
                  scope.endSetSize = function( id ){
                     var modal = $( '> .modal2', element ) ;
                     modal.removeClass( 'alpha' ) ;
                     $( document.body ).removeClass( 'unselect' ) ;
                     $( document ).off( 'mousemove' ) ;
                     $( document ).off( 'mouseup' ) ;
                     if( typeof( scope.data.onResize ) == 'function' )
                     {
                        scope.data.onResize( parseInt( scope.Setting.BodyStyle.width ), parseInt( scope.Setting.BodyStyle.height ) ) ;
                     }
                     Tip.auto() ;
                  }

                  //关闭弹窗
                  scope.closeModal = function(){
                     scope.recoveryModal() ;
                     scope.data.isShow = false ;
                     //scope.Setting.Mask.detach() ;
                     scope.data.onResize = null ;
                  }

                  //最大化或恢复弹窗
                  scope.switchModalSize = function(){
                     if( scope.Setting.Status == 2 )
                     {
                        scope.recoveryModal() ;
                     }
                     else
                     {
                        scope.maximumModal() ;
                     }
                  }

                  //最大化弹窗
                  scope.maximumModal = function(){
                     scope.Setting.Status = 2 ;
                     var modal = $( '> .modal2', element ) ;
                     var bodyWidth = $( window ).width() ;
                     var bodyHeight = $( window ).height() ;
                     var width = bodyWidth - 12 ;
                     var height = bodyHeight - 16 ;
                     if( width < 600 ) width = 600 ;
                     if( height < 450 ) height = 450 ;
                     var left = 6 ;
                     var top = 6 ;
                     scope.Setting.Style.width = width + 'px' ;
                     scope.Setting.Style.height = height + 'px' ;
                     scope.Setting.Style.left = left + 'px' ;
                     scope.Setting.Style.top = top + 'px' ;
                     width -= 42 ;
                     height -= 126 ;
                     $( '> .modal2 > .body', element ).width( width ).height( height ) ;
                     if( typeof( scope.data.onResize ) == 'function' )
                     {
                        scope.data.onResize( width, height ) ;
                     }
                  }

                  //恢复弹窗
                  scope.recoveryModal = function(){
                     scope.Setting.Status = 1 ;
                     var modal = $( '> .modal2', element ) ;
                     var width = parseInt( scope.Setting.Temp.width ) ;
                     scope.Setting.Style.width = width + 'px' ;
                     scope.Setting.Style.height = scope.Setting.Temp.height  ;
                     scope.Setting.Style.left = scope.Setting.Temp.left  ;
                     scope.Setting.Style.top = scope.Setting.Temp.top  ;
                     width = parseInt( scope.Setting.Style.width ) - 42 ;
                     var height = parseInt( scope.Setting.Style.height ) - 126 ;
                     $( '> .modal2 > .body', element ).width( width ).height( height ) ;
                     if( typeof( scope.data.onResize ) == 'function' )
                     {
                        scope.data.onResize( width, height ) ;
                     }
                  }
                  //确定
                  scope.ok = function(){
                     if( typeof( scope.data.ok ) == 'function' )
                     {
                        if( scope.data.ok() )
                        {
                           scope.closeModal() ;
                        }
                     }
                     else
                     {
                        scope.closeModal() ;
                     }
                  }

                  scope.close = function(){
                     if( typeof( scope.data.close ) == 'function' )
                     {
                        if( scope.data.close() )
                        {
                           scope.closeModal() ;
                        }
                     }
                     else
                     {
                        scope.closeModal() ;
                     }
                  }
               }
            } ;
         }
      } ;
      return dire ;
   });

   /*
   新版弹窗
   支持命令： ng-windows         必填    *   给弹窗内容的数据，任意类型
                  格式： newDataName as xxx， xxx代表上层scope的变量
             windows-callback   可选    {}  弹窗接口
   */
   sacApp.directive( 'ngWindows', function( $animate, $compile, $timeout, SdbFunction, Tip ){
      var dire = {
         restrict: 'A',
         replace: false,
         transclude: true,
         terminal: true,
         scope: true,
         templateUrl: './app/template/Component/Windows.html',
         controller: function( $scope, $element, $attrs, $transclude ){
            $scope.lastScope = null ; //最后一次自己创建的scope
            $scope.Setting = {
               isShow: false,       //是否显示
               Modal: null,
               data: null,          //父scope传来的值
               Status: 1,           //弹窗状态，1是普通，2是最大化
               Title: '',           //窗口标题
               Icon: '',            //窗口图标
               Windows: {           //窗口容器属性
                  'top': 0,
                  'left': 0,
                  'width': 0,
                  'height': 0
               },
               Event: {
                  onResize: null
               },
               Body: {},            //body的样式
               Button: {            //按钮
                  OK: {
                     'Text': $scope.autoLanguage( '确定' ),
                     'Func': null
                  },
                  Close: {
                     'Text': $scope.autoLanguage( '取消' ),
                     'Func': function(){ return true; }
                  }
               },
               Mask: $compile( $( '<div></div>' ).attr( 'ng-mousedown', 'prompt()' ).addClass( 'mask-screen unalpha' ) )( $scope ),  //遮罩
               Temp: {              //临时数据
                  left:    100000,
                  top:     100000,
                  width:   100000,
                  height:  100000,
                  x: 0,
                  y: 0
               }
            } ;
         },
         compile: function( element, attributes, transclude ){
            return {
               pre: function preLink( scope, element, attributes ){},
               post: function postLink( scope, element, attributes ){

                  scope.$on( '$destroy', function(){  //主scope释放，子的scope也要释放

                     if( scope.lastScope !== null )
                        scope.lastScope.$destroy() ;

                     $animate.leave( scope.Setting['Mask'] ) ;
                     if( scope.Setting.Modal !== null )
                     {
                        scope.Setting.Modal.remove() ;
                        $animate.leave( scope.Setting.Modal ) ;
                     }

                     scope.lastScope = null ;
                     scope.Setting['Modal'] = null ;
                     scope.Setting['data']  = null ;
                     scope.Setting['Windows'] = null ;
                     scope.Setting['Body'] = null ;
                     scope.Setting['Button']['OK']['Func'] = null ;
                     scope.Setting['Button']['OK'] = null ;
                     scope.Setting['Button']['Close']['Func'] = null ;
                     scope.Setting['Button']['Close'] = null ;
                     scope.Setting['Button'] = null ;
                     scope.Setting['Mask'] = null ;
                     scope.Setting['Temp'] = null ;
                     scope.Setting = null ;
                  } ) ;

                  //解析表达式
                  var expression = attributes.ngWindows;
                  var match = expression.match(/^\s*([\s\S]+?)\s+as\s+([\s\S]+?)\s*$/) ;
                  if( !match )
                  {
                     throw "Expected expression in form of '_a_ as _b_: " + expression ;
                  }
                  var sub = match[1] ;
                  var rhs  = match[2] ;

                  //创建内容
                  var createBody = function(){
                     //初始化原始宽高和位置
                     scope.Setting.Temp.left   = 100000 ;
                     scope.Setting.Temp.top    = 100000 ;
                     scope.Setting.Temp.width  = 100000 ;
                     scope.Setting.Temp.height = 100000 ;
                     scope.Setting.Temp.x = 0 ;
                     scope.Setting.Temp.y = 0 ;
                     //先恢复窗口
                     scope.recoveryModal() ;
                     if( scope.lastScope !== null )
                     {
                        scope.lastScope.$destroy() ;
                        scope.lastScope = null ;
                     }
                     $timeout( function(){
                        var modal = $( '> .modal2', element ) ;
                        var bodyEle = $( '> .body', modal ) ;
                        if( bodyEle.length > 0 && scope.Setting.isShow == true )
                        {
                           var childScope = scope.$new();
                           scope.lastScope = childScope ;
                           childScope[ sub ] = scope.Setting.data ;
                           transclude( childScope, function( clone ){
                              $animate.enter( clone, bodyEle, null ) ;
                              bodyEle = null ;
                           } ) ;
                        }
                        $( modal ).detach() ;
                        $( document.body ).append( modal ) ;
                        scope.Setting.Modal = modal ;
                     } ) ;
                  }

                  //监控数据
                  scope.$watch( rhs, function ngTable( collection ){
                     $( element ).height( 0 ) ; //为了Ie7兼容
                     scope.Setting.data = collection ;
                  } ) ;

                  //最大化
                  var maximumModal = function(){
                     scope.Setting.Status = 2 ;
                     var bodyWidth = $( window ).width() ;
                     var bodyHeight = $( window ).height() ;
                     var width = bodyWidth - 12 ;
                     var height = bodyHeight - 16 ;
                     if( width < 600 ) width = 600 ;
                     if( height < 450 ) height = 450 ;
                     var left = 6 ;
                     var top = 6 ;
                     scope.Setting.Windows = { 'left': left, 'top': top, 'width': width, 'height': height } ;
                     width -= 42 ;
                     height -= 126 ;
                  }

                  //恢复原来
                  var recoveryModal = function(){
                     scope.Setting.Status = 1 ;
                     var modalWidth = scope.Setting.Temp.width ;
                     var modalHeight = scope.Setting.Temp.height ;
                     var left = scope.Setting.Temp.left ;
                     var top = scope.Setting.Temp.top ;
                     var bodyWidth = $( window ).width() ;
                     var bodyHeight = $( window ).height() ;
                     if( top + modalHeight + 5 >= bodyHeight || left + modalWidth + 5 >= bodyWidth )
                     {
                        var width = bodyWidth * 0.5 ;
                        var height = bodyHeight * 0.5 ;
                        if( width < 600 ) width = 600 ;
                        if( height < 450 ) height = 450 ;
                        left = ( bodyWidth - width ) * 0.5 ;
                        top = ( bodyHeight - height ) * 0.5 ;
                        scope.Setting.Temp.left = left ;
                        scope.Setting.Temp.top = top ;
                        scope.Setting.Temp.width = width ;
                        scope.Setting.Temp.height = height ;
                        scope.Setting.Windows = { 'left': left, 'top': top, 'width': width, 'height': height } ;
                     }
                     else
                     {
                        scope.Setting.Windows = { 'left': left, 'top': top, 'width': modalWidth, 'height': modalHeight } ;
                     }
                  }

                  //打开窗口
                  scope.openWindows = function(){
                     $( element ).height( 0 ) ; //为了Ie7兼容
                     if( scope.Setting.isShow == false )
                     {
                        $( document.body ).append( scope.Setting.Mask ) ;
                     }
                     scope.Setting.isShow = true ;
                     createBody() ;
                  }

                  //关闭窗口
                  scope.closeWindows = function(){
                     if( scope.lastScope !== null )
                        scope.lastScope.$destroy() ;
                     if( scope.Setting.Modal !== null )
                     {
                        scope.Setting.Modal.remove() ;
                        $animate.leave( scope.Setting.Modal ) ;
                        scope.Setting.Modal = null ;
                     }
                     scope.Setting.isShow = false ;
                     $( scope.Setting.Mask ).detach() ;
                  }

                  //添加重绘事件
                  scope.setResize = function( func ){
                     scope.Setting.Event.onResize = func ;
                  }

                  //设置确定按钮
                  scope.setOkButton = function( text, func ){
                     scope.Setting.Button.OK['Text'] = text ;
                     scope.Setting.Button.OK['Func'] = func ;
                  }

                  //设置取消和关闭按钮
                  scope.setCloseButton = function( text, func ){
                     scope.Setting.Button.Close['Text'] = text ;
                     scope.Setting.Button.Close['Func'] = func ;
                  }

                  //启用body的滚动条
                  scope.enableBodyScroll = function(){
                     scope.Setting.Body['overflow'] = 'auto' ;
                  }

                  //禁用body的滚动条
                  scope.disableBodyScroll = function(){
                     scope.Setting.Body['overflow'] = 'hidden' ;
                  }

                  //设置标题
                  scope.setTitle = function( title ){
                     scope.Setting.Title = title ;
                  }

                  //设置图标
                  scope.setIcon = function( icon ){
                     scope.Setting.Icon = icon ;
                  }

                  //回调函数
                  scope.$watch( attributes.windowsCallback, function ngTable( callback ){
                     if( typeof( callback ) == 'object' )
                     {
                        callback['Open']              = scope.openWindows ;
                        callback['Close']             = scope.closeWindows ;
                        callback['SetOkButton']       = scope.setOkButton ;
                        callback['SetCloseButton']    = scope.setCloseButton ;
                        callback['EnableBodyScroll']  = scope.enableBodyScroll ;
                        callback['DisableBodyScroll'] = scope.disableBodyScroll ;
                        callback['SetTitle']          = scope.setTitle ;
                        callback['SetIcon']           = scope.setIcon ;
                        callback['SetResize']         = scope.setResize ;
                     }
                  } ) ;

                  //重绘函数
                  function resizeFun()
                  {
                     if( scope.Setting.Status == 1 )
                     {
                        recoveryModal() ;
                     }
                     else
                     {
                        maximumModal() ;
                     }
                     if( typeof( scope.Setting.Event.onResize ) == 'function' )
                     {
                        scope.Setting.Event.onResize( scope.Setting.Windows['left'],
                                                      scope.Setting.Windows['top'],
                                                      scope.Setting.Windows['width'],
                                                      scope.Setting.Windows['height'] ) ;
                     }
                  }

                  resizeFun() ;

                  //重绘函数绑定到重绘队列
                  SdbFunction.defer( scope, resizeFun ) ;

                  //闪烁
                  scope.prompt = function(){
                     if( scope.Setting.Modal !== null )
                     {
                        var counter = 0 ;
                        var modal = scope.Setting.Modal ;
                        var timer = setInterval( function(){
                           ++counter ;
                           if( counter % 2 == 0 )
                              $( modal ).css( 'box-shadow', '0px 2px 8px rgba(0,0,0,0.5)' ) ;
                           else
                              $( modal ).css( 'box-shadow', 'none' ) ;
                           if( counter >= 6 )
                              clearInterval( timer );
                        }, 90 ) ;
                     }
                  }

                  //移动弹窗开始
                  scope.startMove = function( event ){
                     if( scope.Setting.Status == 1 && scope.Setting.Modal != null )
                     {
                        var modal = scope.Setting.Modal ;
                        var pageX = event['pageX'] ;
                        var pageY = event['pageY'] ;
                        scope.Setting.Temp.x = pageX ;
                        scope.Setting.Temp.y = pageY ;
                        $( document.body ).addClass( 'unselect' ) ;
                        //监听鼠标移动
                        $( document ).on( 'mousemove', function( event2 ){
                           modal.addClass( 'alpha' ) ;
                           scope.moveModal( event2 ) ;
                        } ) ;
                        //监听鼠标松开
                        $( document ).on( 'mouseup', function(){
                           scope.endMove() ;
                        } ) ;
                     }
                  }

                  //移动
                  scope.moveModal = function( event ){
                     if( scope.Setting.Modal != null )
                     {
                        var modal = scope.Setting.Modal ;
                        if( modal.hasClass( 'alpha' ) )
                        {
                           var bodyWidth = $( window ).width() ;
                           var bodyHeight = $( window ).height() ;
                           var modalWidth = scope.Setting.Windows['width'] ;
                           var modalHeight = scope.Setting.Windows['height'] ;
                           var x = scope.Setting.Temp.x ;
                           var y = scope.Setting.Temp.y ;
                           var pageX = event['pageX'] ;
                           var pageY = event['pageY'] ;
                           scope.Setting.Temp.x = pageX ;
                           scope.Setting.Temp.y = pageY ;
                           var left = scope.Setting.Windows['left'] ;
                           var top = scope.Setting.Windows['top'] ;
                           var offsetLeft = pageX - x ;
                           var offsetTop = pageY - y ;
                           top  += offsetTop ;
                           left += offsetLeft ;
                           if( top <= 0 ) top = 0 ;
                           if( left <= 0 ) left = 0 ;
                           if( top + modalHeight + 5 >= bodyHeight ) top = bodyHeight - modalHeight - 5 ;
                           if( left + modalWidth + 5 >= bodyWidth ) left = bodyWidth - modalWidth - 5 ;
                           scope.Setting.Temp.left = left ;
                           scope.Setting.Temp.top = top ;
                           scope.Setting.Windows = { 'left': left, 'top': top, 'width': modalWidth, 'height': modalHeight } ;
                           scope.$digest() ;
                        }
                     }
                  }

                  //结束移动
                  scope.endMove = function(){
                     if( scope.Setting.Modal !== null )
                     {
                        var modal = scope.Setting.Modal ;
                        modal.removeClass( 'alpha' ) ;
                        $( document.body ).removeClass( 'unselect' ) ;
                        $( document ).off( 'mousemove' ) ;
                        $( document ).off( 'mouseup' ) ;
                        Tip.auto() ;
                     }
                  }

                  //开始调整窗口大小
                  scope.startSetSize = function( event ){
                     if( scope.Setting.Modal !== null )
                     {
                        var modal = scope.Setting.Modal ;
                        $( document.body ).addClass( 'unselect' ) ;
                        //监听鼠标移动
                        $( document ).on( 'mousemove', function( event2 ){
                           modal.addClass( 'alpha' ) ;
                           scope.setSize( event2 ) ;
                        } ) ;
                        //监听鼠标松开
                        $( document ).on( 'mouseup', function(){
                           scope.endSetSize() ;
                        } ) ;
                     }
                  }

                  //正在调整窗口大小
                  scope.setSize = function( event ){
                     var bodyWidth = $( window ).width() ;
                     var bodyHeight = $( window ).height() ;
                     var left = parseInt( scope.Setting.Temp.left ) ;
                     var top = parseInt( scope.Setting.Temp.top ) ;
                     var pageX = event['pageX'] + 3 ;
                     var pageY = event['pageY'] + 5 ;
                     var width = pageX - left ;
                     var height = pageY - top ;
                     if( width < 600 ) width = 600 ;
                     if( height < 450 ) height = 450 ;
                     if( top + height + 10 >= bodyHeight ) height = bodyHeight - top - 10 ;
                     if( left + width + 10 >= bodyWidth ) width = bodyWidth - left - 10 ;
                     scope.Setting.Temp.width = width ;
                     scope.Setting.Temp.height = height ;
                     scope.Setting.Windows = { 'left': left, 'top': top, 'width': width, 'height': height } ;
                     scope.$digest() ;
                  }

                  //结束调整窗口大小
                  scope.endSetSize = function( id ){
                     if( scope.Setting.Modal !== null )
                     {
                        var modal = scope.Setting.Modal ;
                        modal.removeClass( 'alpha' ) ;
                        $( document.body ).removeClass( 'unselect' ) ;
                        $( document ).off( 'mousemove' ) ;
                        $( document ).off( 'mouseup' ) ;
                        Tip.auto() ;
                        if( typeof( scope.Setting.Event.onResize ) == 'function' )
                        {
                           scope.Setting.Event.onResize( scope.Setting.Windows['left'],
                                                         scope.Setting.Windows['top'],
                                                         scope.Setting.Windows['width'],
                                                         scope.Setting.Windows['height'] ) ;
                        }
                     }
                  }

                  //最大化或恢复弹窗
                  scope.switchModalSize = function(){
                     if( scope.Setting.Status == 2 )
                     {
                        scope.recoveryModal() ;
                     }
                     else
                     {
                        scope.maximumModal() ;
                     }
                  }

                  //最大化弹窗
                  scope.maximumModal = function(){
                     scope.Setting.Status = 2 ;
                     scope.bindResize() ;
                  }

                  //恢复弹窗
                  scope.recoveryModal = function(){
                     scope.Setting.Status = 1 ;
                     scope.bindResize() ;
                  }

                  //确定
                  scope.ok = function(){
                     if( typeof( scope.Setting.Button.OK.Func ) == 'function' )
                     {
                        if( scope.Setting.Button.OK.Func() )
                        {
                           scope.closeWindows() ;
                        }
                     }
                     else
                     {
                        scope.closeWindows() ;
                     }
                  }

                  //关闭弹窗
                  scope.closeModal = function(){
                     if( scope.Setting.Button.Close.Func() )
                     {
                        scope.closeWindows() ;
                     }
                  }
               }
            } ;
         }
      } ;
      return dire ;
   } ) ;

   //Json编辑器
   sacApp.directive( 'jsonEdit', function( $rootScope ){
      var dire = {
         restrict: 'A',
         scope: {
            data: '=para'
         },
         templateUrl: './app/template/Component/JsonEdit.html',
         replace: false,
         controller: function( $scope, $element ){
            $scope.autoLanguage = $rootScope.autoLanguage ;
            var json = {} ;
            if( $scope.data && typeof( $scope.data['Json'] ) == 'object' )
            {
               json = $scope.data['Json'] ;
            }
            $scope.Setting = {
               IsPaste: false,
               PasteEle: null,
               IsFullScreen: false,
               TempCSS: null,
               Height: 0,
               Type: 1,
               View: json2Array( json ),
               Json: JSON.stringify( json, null, 3 ),
               Search: ''
            } ;
            $scope.Setting.View[0]['isOpen'] = true ;
            var listener = $scope.$watch( 'data.Height', function(){
               var value = parseInt( $scope.data.Height ) ;
               if( !isNaN( value ) )
               {
                  if( value >= 39 )
                  {
                     $scope.Setting.Height = value - 39 ;
                  }
                  else
                  {
                     $scope.Setting.Height = 0 ;
                  }
               }
            } ) ;
            $scope.$on( '$destroy', function(){
               listener() ;
            } ) ;
            $scope.data['Callback'] = {} ;
            $scope.data['Callback']['getJson'] = function(){
               if( $scope.Setting.Type == 1 )
               {
                  return array2Json( $scope.Setting.View ) ;
               }
               else
               {
                  try
                  {
                     return JSON.parse( $scope.Setting.Json ) ;
                  }
                  catch(e)
                  {
                     alert( e.message ) ;
                     throw e.message ;
                  }
               }
            }
         },
         compile: function( element, attributes ){
            return {
               pre: function preLink( scope, element, attributes ){
                  //scope的一些初始化或者运算
               },
               post: function postLink( scope, element, attributes ){

                  //显示类型菜单 
                  scope.showMenu = function( event ){
                     var clickEle = $( event.target ) ;
                     var button = clickEle ;
                     if( clickEle.hasClass( 'fa' ) )
                     {
                        button = clickEle.parent() ;
                     }
                     var menu = $( '~ .treeMenu', button.parent() ) ;
                     var offsetX = button.offset().left - 5 ;
                     var offsetY = button.offset().top + button.height() ;
                     menu.css( { 'left': offsetX, 'top': offsetY } ) ;
                     menu.show( 150, function(){
                        $( document ).on( 'click', function( event2 ){
                           scope.hideMenu( menu ) ;
                        } ) ;
                     } ) ;
                  }

                  scope.checkHtml = function( event ){
                     scope.Setting.IsPaste = true ;
                     scope.Setting.PasteEle = event.target ;
                  }

                  scope.htmlEscape = function( field ){
                     if( scope.Setting.IsPaste )
                     {
                        var str = field.val ;
                        $( scope.Setting.PasteEle ).text('').focus().text( str ) ;
                        scope.Setting.IsPaste = false ;
                        try
                        {
                           set_focus( scope.Setting.PasteEle ) ;
                        }
                        catch( e )
                        {
                        }
                     }
                  }

                  //隐藏菜单
                  scope.hideMenu = function( menu ){
                     menu.hide() ;
                     $( document ).off( 'click' ) ;
                  }

                  //显示隐藏子字段
                  scope.toggle = function( field ){
                     field.isOpen = !field.isOpen ;
                  }

                  //添加子字段
                  scope.add = function( field ){
                     if( field.type == 'Array' )
                     {
                        var key = '0' ;
                        var i = 0 ;
                        while( true )
                        {
                           var hasKey = false ;
                           $.each( field['val'], function( index, val ){
                              if( val['key'] == key )
                              {
                                 hasKey = true ;
                                 return false ;
                              }
                           } ) ;
                           if( hasKey == false )
                           {
                              field['val'].push( { key: key, val: 'value', isOpen: true, type: 'Auto', level: field.level + 1 } ) ;
                              break ;
                           }
                           ++i ;
                           key = i + '' ;
                        }
                     }
                     else
                     {
                        var key = 'field' ;
                        var i = 0 ;
                        while( true )
                        {
                           var hasKey = false ;
                           $.each( field['val'], function( index, val ){
                              if( val['key'] == key )
                              {
                                 hasKey = true ;
                                 return false ;
                              }
                           } ) ;
                           if( hasKey == false )
                           {
                              field['val'].push( { key: key, val: 'value', isOpen: true, type: 'Auto', level: field.level + 1 } ) ;
                              break ;
                           }
                           ++i ;
                           key = 'field_' + i ;
                        }
                     }
                  }

                  //删除字段
                  scope.remove = function( fields, field ){
                     var index = fields.indexOf( field ) ;
                     fields.splice( index, 1 );     
                  }
                  //复制
                  scope.copy = function( fields, field ){
                     var index = fields.indexOf( field ) + 1 ;
                     var key = 'field' ;
                     var i = 0 ;
                     while( true )
                     {
                        var hasKey = false ;
                        $.each( fields, function( index, val ){
                           if( val['key'] == key )
                           {
                              hasKey = true ;
                              return false ;
                           }
                        } ) ;
                        if( hasKey == false )
                        {
                           var newField = $.extend( true, {}, field ) ;
                           newField['key'] = key ;
                           newField['isOpen'] = true ;
                           fields.splice( index, 0, newField );
                           break ;
                        }
                        ++i ;
                        key = 'field_' + i ;
                     }
                  }
                  //切换视图
                  scope.switchView = function(){
                     if( scope.Setting.Type == 1 )
                     {
                        //视图 -> Json
                        var json = array2Json( scope.Setting.View ) ;
                        scope.Setting.Json = JSON.stringify( json, null, 3 ) ;
                        scope.Setting.Type = 2 ;
                     }
                     else 
                     {
                        //Json -> 视图
                        try
                        {
                           var json = JSON.parse( scope.Setting.Json ) ;
                           scope.Setting.View = json2Array( json ) ;
                           scope.Setting.View[0]['isOpen'] = true ;
                           scope.Setting.Type = 1 ;
                        }
                        catch(e)
                        {
                           alert( e.message ) ;
                        }
                     }
                  }
                  //展开
                  scope.expand = function(){
                     if( scope.Setting.Type == 1 )
                     {
                        //视图模式
                        function viewExpand( fields )
                        {
                           $.each( fields, function( index, field ){
                              field.isOpen = true ;
                              if( field.type == 'Object' || field.type == 'Array' )
                              {
                                 viewExpand( field.val ) ;
                              }
                           } ) ;
                        }
                        viewExpand( scope.Setting.View ) ;
                     }
                     else
                     {
                        //var json = array2Json( scope.Setting.View ) ;
                        try
                        {
                           var json = JSON.parse( scope.Setting.Json ) ;
                           scope.Setting.Json = JSON.stringify( json, null, 3 ) ;
                        }
                        catch( e )
                        {
                           alert( e.message ) ;
                        }
                     }
                  }
                  //收起
                  scope.collapse = function(){
                     if( scope.Setting.Type == 1 )
                     {
                        //视图模式
                        function viewExpand( fields )
                        {
                           $.each( fields, function( index, field ){
                              field.isOpen = false ;
                              if( field.type == 'Object' || field.type == 'Array' )
                              {
                                 viewExpand( field.val ) ;
                              }
                           } ) ;
                        }
                        viewExpand( scope.Setting.View ) ;
                     }
                     else
                     {
                        //var json = array2Json( scope.Setting.View ) ;
                        try
                        {
                           var json = JSON.parse( scope.Setting.Json ) ;
                           scope.Setting.Json = JSON.stringify( json ) ;
                        }
                        catch( e )
                        {
                           alert( e.message ) ;
                        }
                     }
                  }

                  //全屏
                  scope.fullScreen = function(){
                     var bodyWidth = $( window ).width() ;
                     var bodyHeight = $( window ).height() ;
                     var width = bodyWidth - 12 ;
                     var height = bodyHeight - 16 ;
                     if( width < 600 ) width = 600 ;
                     if( height < 450 ) height = 450 ;
                     scope.Setting.TempCSS = $( element ).attr( 'style' ) ;
                     $( element ).css( { 'position': 'fixed', 'top': '6px', 'left': '6px', 'background': '#fff', 'width': width, 'height': height,
                                         'border': '#AAA solid 1px', 'box-shadow': '0px 2px 8px rgba(0,0,0,0.5)', 'margin': 0 } ) ;
                     scope.Setting.Height = height - 39 ;
                     scope.Setting.IsFullScreen = true ;
                  }

                  //取消全屏
                  scope.cancelFullScreen = function(){
                     $( element ).css( { 'position': 'static', 'top': 'auto', 'left': 'auto', 'background': '#fff', 'width': 'auto', 'height': 'auto',
                                         'border': 'none', 'box-shadow': 'none' } ) ;
                     $( element ).attr( 'style', scope.Setting.TempCSS ) ;
                     scope.Setting.Height = scope.data.Height - 39 ;
                     scope.Setting.IsFullScreen = false ;
                  }

                  //修改类型
                  scope.changeType = function( event, field, newType ){
                     var clickEle = $( event.target ) ;
                     var button = clickEle ;
                     if( clickEle.hasClass( 'fa' ) )
                     {
                        button = clickEle.parent() ;
                     }
                     var menu = $( button.parent().parent().parent() ) ;
                     scope.hideMenu( menu ) ;
                     if( newType == 'Auto' )
                     {
                        if( !( field.type == 'Bool' || field.type == 'Number' || field.type == 'Null' || field.type == 'String' ) )
                        {
                           field.val = '' ;
                        }
                     }
                     else if( newType == 'Object' )
                     {
                        if( field.type == 'Object' )
                        {
                           return ;
                        }
                        else if( field.type == 'Array' )
                        {
                           $.each( field.val, function( index, child ){
                              child.key = index + '' ;
                           } ) ;
                           field.isOpen = true ;
                        }
                        else
                        {
                           field.val = [] ;
                           field.isOpen = true ;
                        }
                     }
                     else if( newType == 'Array' )
                     {
                        if( field.type == 'Array' )
                        {
                           return ;
                        }
                        else if( field.type == 'Object' )
                        {
                           $.each( field.val, function( index, child ){
                              child.key = index + '' ;
                           } ) ;
                           field.isOpen = true ;
                        }
                        else
                        {
                           field.val = [] ;
                           field.isOpen = true ;
                        }
                     }
                     else if( newType == 'String' )
                     {
                        if( field.type == 'Object' || field.type == 'Array' )
                        {
                           field.val = '' ;
                        }
                     }
                     else if( newType == 'Timestamp' )
                     {
                        field.val = timeFormat( new Date(), 'yyyy-MM-dd-hh.mm.ss.000000' ) ;
                     }
                     else if( newType == 'Date' )
                     {
                        field.val = timeFormat( new Date(), 'yyyy-MM-dd' ) ;
                     }
                     field.type = newType ;
                  }
               }
            } ;
         }
      } ;
      return dire ;
   });

   //Json查看器
   sacApp.directive( 'jsonView', function( $rootScope, $timeout, SdbFunction ){
      var dire = {
         restrict: 'A',
         scope: true,
         replace: false,
         templateUrl: './app/template/Component/JsonView.html',
         // 专用控制器
         controller: function( $scope, $element, $attrs, $transclude ){
            $scope.autoLanguage = $rootScope.autoLanguage ;
            var json = {} ;
            $scope.Setting = {
               TempCSS: '',
               IsFullScreen: false,
               Type: 1,
               View: json2Array( json ),
               Json: JSON.stringify( json, null, 3 ),
               Search: ''
            } ;
         },
         // 编译
         compile: function( element, attributes ){
            return {
               pre: function preLink( scope, element, attributes ){
               },
               post: function postLink( scope, element, attributes ){

                  var box = $( element ) ;

                  function resizeFun()
                  {
                     $timeout( function(){
                        var height ;
                        if( scope.Setting.IsFullScreen )
                        {
                           var bodyWidth = $( window ).width() ;
                           var bodyHeight = $( window ).height() ;
                           var width = bodyWidth - 12 ;
                           height = bodyHeight - 16 ;
                           $( element ).css( { 'width': width, 'height': height } ) ;
                        }
                        else
                        {
                           height = box.parent().height() ;
                        }

                        $( '> .jsonEdit > .jsonModel > .editBox', box ).height( height - 40 ) ;
                        $( '> .jsonEdit > .viewModel > .editBox', box ).height( height - 40 ) ;
                     } ) ;
                  }

                  resizeFun() ;

                  //重绘函数绑定到重绘队列
                  SdbFunction.defer( scope, resizeFun ) ;

                  //监控配置
                  scope.$watch( attributes.jsonView, function jsonView( json ){
                     if( typeof( json ) == 'object' )
                     {
                        scope.Setting.View = json2Array( json ) ;
                        scope.Setting.View[0]['isOpen'] = true ;
                        scope.Setting.Json = JSON.stringify( json, null, 3 ) ;
                     }
                  }, true ) ;

                  //如果有json-callback，那么把回调的函数传给他
                  scope.$watch( attributes.jsonCallback, function ( callbackGetter ){
                     if( typeof( callbackGetter ) == 'object' )
                     {
                        callbackGetter['Resize'] = resizeFun ;
                     }
                  } ) ;

                  //显示隐藏子字段
                  scope.toggle = function( field ){
                     field.isOpen = !field.isOpen ;
                  }
                  
                  //切换视图
                  scope.switchView = function(){
                     if( scope.Setting.Type == 1 )
                     {
                        //视图 -> Json
                        var json = array2Json( scope.Setting.View ) ;
                        scope.Setting.Json = JSON.stringify( json, null, 3 ) ;
                        scope.Setting.Type = 2 ;
                     }
                     else 
                     {
                        //Json -> 视图
                        try
                        {
                           var json = JSON.parse( scope.Setting.Json ) ;
                           scope.Setting.View = json2Array( json ) ;
                           scope.Setting.View[0]['isOpen'] = true ;
                           scope.Setting.Type = 1 ;
                        }
                        catch(e)
                        {
                           alert( e.message ) ;
                        }
                     }
                  }

                  //展开
                  scope.expand = function(){
                     if( scope.Setting.Type == 1 )
                     {
                        //视图模式
                        function viewExpand( fields )
                        {
                           $.each( fields, function( index, field ){
                              field.isOpen = true ;
                              if( field.type == 'Object' || field.type == 'Array' )
                              {
                                 viewExpand( field.val ) ;
                              }
                           } ) ;
                        }
                        viewExpand( scope.Setting.View ) ;
                     }
                     else
                     {
                        try
                        {
                           var json = JSON.parse( scope.Setting.Json ) ;
                           scope.Setting.Json = JSON.stringify( json, null, 3 ) ;
                        }
                        catch( e )
                        {
                           alert( e.message ) ;
                        }
                     }
                  }

                  //收起
                  scope.collapse = function(){
                     if( scope.Setting.Type == 1 )
                     {
                        //视图模式
                        function viewExpand( fields )
                        {
                           $.each( fields, function( index, field ){
                              field.isOpen = false ;
                              if( field.type == 'Object' || field.type == 'Array' )
                              {
                                 viewExpand( field.val ) ;
                              }
                           } ) ;
                        }
                        viewExpand( scope.Setting.View ) ;
                     }
                     else
                     {
                        try
                        {
                           var json = JSON.parse( scope.Setting.Json ) ;
                           scope.Setting.Json = JSON.stringify( json ) ;
                        }
                        catch( e )
                        {
                           alert( e.message ) ;
                        }
                     }
                  }

                  //全屏
                  scope.fullScreen = function(){
                     var bodyWidth = $( window ).width() ;
                     var bodyHeight = $( window ).height() ;
                     var width = bodyWidth - 12 ;
                     var height = bodyHeight - 16 ;
                     if( width < 600 ) width = 600 ;
                     if( height < 450 ) height = 450 ;
                     scope.Setting.TempCSS = $( element ).attr( 'style' ) ;
                     $( element ).css( { 'position': 'fixed', 'top': '6px', 'left': '6px', 'background': '#fff', 'width': width, 'height': height,
                                         'border': '#AAA solid 1px', 'box-shadow': '0px 2px 8px rgba(0,0,0,0.5)', 'margin': 0 } ) ;
                     $( '.jsonEdit > .jsonModel > .editBox', box ).height( height - 40 ) ;
                     $( '.jsonEdit > .viewModel > .editBox', box ).height( height - 40 ) ;
                     scope.Setting.IsFullScreen = true ;
                  }

                  //取消全屏
                  scope.cancelFullScreen = function(){
                     $( element ).css( { 'position': 'static', 'top': 'auto', 'left': 'auto', 'background': '#fff', 'width': 'auto', 'height': 'auto',
                                         'border': 'none', 'box-shadow': 'none' } ) ;
                     $( element ).attr( 'style', scope.Setting.TempCSS ) ;
                     var height = box.parent().height() ;
                     $( '.jsonEdit > .jsonModel > .editBox', box ).height( height - 40 ) ;
                     $( '.jsonEdit > .viewModel > .editBox', box ).height( height - 40 ) ;
                     scope.Setting.IsFullScreen = false ;
                  }

               }
            } ;
         }
      } ;
      return dire ;
   } ) ;

   //文字摘要组件，点击可以展开
   sacApp.directive( 'textAbstract', function( $rootScope, $timeout, SdbFunction ){
      var maxLen = 256 ;
      var dire = {
         restrict: 'A',
         scope: true,
         replace: false,
         template: '<span ng-click="switchShow()">{{text}}</span>',
         // 专用控制器
         controller: function( $scope, $element, $attrs, $transclude ){
            $scope.str = '' ;
            $scope.text = '' ;
            $scope.isFullShow = false ;
         },
         // 编译
         compile: function( element, attributes ){
            return {
               pre: function preLink( scope, element, attributes ){
               },
               post: function postLink( scope, element, attributes ){

                  var box = $( element ) ;

                  //监控配置
                  scope.$watch( attributes.textAbstract, function textAbstract( str ){
                     if( typeof( str ) == 'string' )
                     {
                        if( str.length > maxLen && scope.isFullShow == false )
                        {
                           scope.text = str.substring( 0, maxLen ) + '...' ;
                           $( 'span', box ).css( { 'cursor': 'pointer' } ) ;
                        }
                        else
                        {
                           scope.text = str ;
                           scope.isFullShow = true ;
                        }
                        scope.str = str ;
                     }
                  }, true ) ;

                  scope.switchShow = function()
                  {
                     if( scope.str.length > maxLen && scope.isFullShow == false )
                     {
                        scope.text = scope.str ;
                        scope.isFullShow = true ;
                     }
                     else if( scope.str.length > maxLen && scope.isFullShow == true )
                     {
                        scope.text = scope.str.substring( 0, maxLen ) + '...' ;
                        scope.isFullShow = false ;
                     }
                  }
               }
            } ;
         }
      } ;
      return dire ;
   } ) ;

   //表单
   sacApp.directive( 'formCreate', function( $rootScope, SdbFunction ){
      var dire = {
         restrict: 'A',
         scope: {
            data: '=para'
         },
         templateUrl: './app/template/Component/Form.html',
         replace: false,
         controller: function( $scope, $element ){
            var init = function(){
               if( typeof( $scope.data ) == 'object' )
               {
                  $.each( $scope.data.inputList, function( index ){
                     $scope.data.inputList[index]['isClick'] = false ;
                     if( $scope.data.inputList[index]['type'] == 'list' )
                     {
                        $.each( $scope.data.inputList[index]['child'], function( index2 ){
                           $.each( $scope.data.inputList[index]['child'][index2], function( index3 ){
                              if( typeof( $scope.data.inputList[index]['child'][index2][index3]['default'] ) == 'undefined' )
                              {
                                 $scope.data.inputList[index]['child'][index2][index3]['default'] = $scope.data.inputList[index]['child'][index2][index3]['value'] ;
                              }
                           } ) ;
                        } ) ;
                     }
                     else if( $scope.data.inputList[index]['type'] == 'multiple' )
                     {
                        if( typeof( $scope.data.inputList[index]['valid'] ) == 'object' )
                        {
                           if( isArray( $scope.data.inputList[index]['valid']['list'] ) == true )
                           {
                              $scope.data.inputList[index]['value'] = [] ;
                              $.each( $scope.data.inputList[index]['valid']['list'], function( listIndex, validInfo ){
                                 if( validInfo['checked'] == true )
                                 {
                                    $scope.data.inputList[index]['value'].push( validInfo['value'] ) ;
                                 }
                              } ) ;
                           }
                        }
                     }
                  } ) ;
                  $scope.browserInfo = SdbFunction.getBrowserInfo() ;
                  $scope.Setting = {
                     Type: $scope.data.type,
                     GridTitle: $scope.data.gridTitle,
                     Grid: $scope.data.grid,
                     KeyWidth: $scope.data.keyWidth ? $scope.data.keyWidth : '130px',
                     Text: {
                        'string': {
                           min: $rootScope.autoLanguage( '?长度不能小于?。' ),
                           max: $rootScope.autoLanguage( '?长度不能大于?。' ),
                           regex: $rootScope.autoLanguage( '?格式错误。' ),
                           ban: $rootScope.autoLanguage( '?不能有?字符。' )
                        },
                        'int': {
                           min: $rootScope.autoLanguage( '?的值不能小于?。' ),
                           max: $rootScope.autoLanguage( '?的值不能大于?。' ),
                           ban: $rootScope.autoLanguage( '?的值不能取?。' ),
                           step: $rootScope.autoLanguage( '?的值必须是?的倍数。' ),
                           format: $rootScope.autoLanguage( '?的值必须是整数。' )
                        },
                        'double': {
                           min: $rootScope.autoLanguage( '?的值不能小于?。' ),
                           max: $rootScope.autoLanguage( '?的值不能大于?。' ),
                           ban: $rootScope.autoLanguage( '?的值不能取?。' ),
                           step: $rootScope.autoLanguage( '?的值必须是?的倍数。' ),
                           format: $rootScope.autoLanguage( '?的值必须是数字。' )
                        },
                        'port':{
                           min: $rootScope.autoLanguage( '?不能小于?。' ),
                           max: $rootScope.autoLanguage( '?不能大于?。' ),
                           empty: $rootScope.autoLanguage( '?不能为空。' ),
                           format: $rootScope.autoLanguage( '?格式错误。' )
                        },
                        'multiple': {
                           min: $rootScope.autoLanguage( '?至少选择?个。' ),
                           max: $rootScope.autoLanguage( '?不能多于?个。' ),
                           empty: $rootScope.autoLanguage( '?不能为空。' ),
                           candidate: $rootScope.autoLanguage( '待选列表' ),
                           select: $rootScope.autoLanguage( '选中列表' )
                        },
                        'list': $rootScope.autoLanguage( '?参数错误。' )
                     },
                     inputList: $scope.data.inputList,
                     checkString: function( name, value, valid ){
                        var rc = true ;
                        var error = '' ;
                        if( typeof( valid ) == 'object' )
                        {
                           var min = valid.min ;
                           var max = valid.max ;
                           var reg = valid.regex ;
                           var rer = valid.regexError ;
                           var ban = valid.ban ;
                           var len = typeof( value ) == 'string' ? value.length : 0 ;
                           if( typeof( min ) == 'number' && len < min )
                           {
                              error = sprintf( $scope.Setting.Text.string.min, name, min ) ;
                              rc = false ;
                           }
                           else if( typeof( max ) == 'number' && len > max )
                           {
                              error = sprintf( $scope.Setting.Text.string.max, name, max ) ;
                              rc = false ;
                           }
                           else if( typeof( ban ) == 'string' && value.indexOf( ban ) >= 0 )
                           {
                              error = sprintf( $scope.Setting.Text.string.ban, name, ban ) ;
                              rc = false ;
                           }
                           else if( isArray( ban ) )
                           {
                              $.each( ban, function( index, banChar ){
                                 if( value.indexOf( banChar ) >= 0 )
                                 {
                                    error = sprintf( $scope.Setting.Text.string.ban, name, banChar ) ;
                                    rc = false ;
                                    return false ;
                                 }
                              } ) ;
                           }
                           else if( typeof( reg ) == 'string' )
                           {
                              var patt = new RegExp( reg, 'g' ) ;
                              if( patt.test( value ) == false )
                              {
                                 if( typeof( rer ) == 'string' )
                                 {
                                    error = rer ;
                                 }
                                 else
                                 {
                                    error = sprintf( $scope.Setting.Text.string.regex, name ) ;
                                 }
                                 rc = false ;
                              }
                           }
                        }
                        return { rc: rc, error: error } ;
                     },
                     checkInt: function ( name, value, valid ){
                        var rc = true ;
                        var error = '' ;
                        if( value.length == 0 && typeof( valid ) == 'object' && valid.empty == true )
                        {
                           return { rc: rc, error: error } ;
                        }
                        if( isNaN( value ) || parseInt( value ) != value )
                        {
                           error = sprintf( $scope.Setting['Text']['int']['format'], name ) ;
                           rc = false ;
                        }
                        else if( typeof( valid ) == 'object' )
                        {
                           var num = parseInt( value ) ;
                           var min = valid.min ;
                           var max = valid.max ;
                           var ban = valid.ban ;
                           var step = valid.step ;
                           if( typeof( min ) == 'number' && num < min )
                           {
                              error = sprintf( $scope.Setting['Text']['int']['min'], name, min ) ;
                              rc = false ;
                           }
                           else if( typeof( max ) == 'number' && num > max )
                           {
                              error = sprintf( $scope.Setting['Text']['int']['max'], name, max ) ;
                              rc = false ;
                           }
                           else if( typeof( ban ) == 'number' && num == ban )
                           {
                              error = sprintf( $scope.Setting['Text']['int']['ban'], name, ban ) ;
                              rc = false ;
                           }
                           else if( isArray( ban ) )
                           {
                              $.each( ban, function( index, banInt ){
                                 if( num == banInt )
                                 {
                                    error = sprintf( $scope.Setting['Text']['int']['ban'], name, banInt ) ;
                                    rc = false ;
                                    return false ;
                                 }
                              } ) ;
                           }
                           else if( typeof( step ) == 'number' && num % step != 0 )
                           {
                              error = sprintf( $scope.Setting['Text']['int']['step'], name, step ) ;
                              rc = false ;
                           }
                        }
                        return { rc: rc, error: error } ;
                     },
                     checkDouble: function ( name, value, valid ){
                        var rc = true ;
                        var error = '' ;
                        if( isNaN( value ) )
                        {
                           error = sprintf( $scope.Setting['Text']['double']['format'], name ) ;
                           rc = false ;
                        }
                        else if( typeof( valid ) == 'object' )
                        {
                           var num = parseFloat( value ) ;
                           var min = valid.min ;
                           var max = valid.max ;
                           var ban = valid.ban ;
                           var step = valid.step ;
                           if( typeof( min ) == 'number' && num < min )
                           {
                              error = sprintf( $scope.Setting['Text']['double']['min'], name, min ) ;
                              rc = false ;
                           }
                           else if( typeof( max ) == 'number' && num > max )
                           {
                              error = sprintf( $scope.Setting['Text']['double']['max'], name, max ) ;
                              rc = false ;
                           }
                           else if( typeof( ban ) == 'number' && num == ban )
                           {
                              error = sprintf( $scope.Setting['Text']['double']['ban'], name, ban ) ;
                              rc = false ;
                           }
                           else if( isArray( ban ) )
                           {
                              $.each( ban, function( index, banFloat ){
                                 if( num == banFloat )
                                 {
                                    error = sprintf( $scope.Setting['Text']['double']['ban'], name, banFloat ) ;
                                    rc = false ;
                                    return false ;
                                 }
                              } ) ;
                           }
                           else if( typeof( step ) == 'number' && num % step != 0 )
                           {
                              error = sprintf( $scope.Setting['Text']['double']['step'], name, step ) ;
                              rc = false ;
                           }
                        }
                        return { rc: rc, error: error } ;
                     },
                     checkPort: function ( name, value, valid ){
                        var rc = true ;
                        var error = '' ;
                        if( value.length == 0 && typeof( valid ) == 'object' && valid.empty == true )
                        {
                           return { rc: rc, error: error } ;
                        }
                        if( value.length == 0 )
                        {
                           rc = false ;
                           error = sprintf( $scope.Setting.Text.port.empty, name ) ;
                        }
                        else if( checkPort( value ) == false )
                        {
                           rc = false ;
                           error = sprintf( $scope.Setting.Text.port.format, name ) ;
                        }
                        else if( typeof( valid ) == 'object' )
                        {
                           var min = valid.min ;
                           var max = valid.max ;
                           var ban = valid.ban ;
                           var step = valid.step ;
                           if( typeof( min ) == 'number' && value < min )
                           {
                              error = sprintf( $scope.Setting['Text']['port']['min'], name, min ) ;
                              rc = false ;
                           }
                           else if( typeof( max ) == 'number' && value > max )
                           {
                              error = sprintf( $scope.Setting['Text']['port']['max'], name, max ) ;
                              rc = false ;
                           }
                        }
                        return { rc: rc, error: error } ;
                     },
                     checkMultiple: function ( name, value, valid ){
                        var rc = true ;
                        var error = '' ;
                        if( typeof( valid ) == 'object' )
                        {
                           value = [] ;
                           $.each( valid['list'], function( listIndex, validInfo ){
                              if( validInfo['checked'] == true )
                              {
                                 value.push( validInfo['value'] ) ;
                              }
                           } ) ;
                           var min = valid.min ;
                           var max = valid.max ;
                           if( value.length == 0 && valid.empty == false )
                           {
                              rc = false ;
                              error = sprintf( $scope.Setting.Text.multiple.empty, name ) ;
                           }
                           else if( typeof( min ) == 'number' && value.length < min )
                           {
                              rc = false ;
                              error = sprintf( $scope.Setting.Text.multiple.min, name, min ) ;
                           }
                           else if( typeof( max ) == 'number' && value.length > max )
                           {
                              rc = false ;
                              error = sprintf( $scope.Setting.Text.multiple.max, name, max ) ;
                           }
                        }
                        return { rc: rc, error: error } ;
                     },
                     checkInput: function( inputList, customCheckFun ){
                        var isAllClear = true ;
                        $.each( inputList, function( index, inputInfo ){
                           inputInfo.error = '' ;
                           var rv = { rc: true, error: '' } ;
                           switch( inputInfo.type )
                           {
                           case 'string':
                              rv = $scope.Setting.checkString( inputInfo.webName, trim( inputInfo.value ), inputInfo.valid ) ;
                              break ;
                           case 'password':
                              rv = $scope.Setting.checkString( inputInfo.webName, inputInfo.value, inputInfo.valid ) ;
                              break ;
                           case 'text':
                              rv = $scope.Setting.checkString( inputInfo.webName, trim( inputInfo.value ), inputInfo.valid ) ;
                              break ;
                           case 'int':
                              rv = $scope.Setting.checkInt( inputInfo.webName, trim( inputInfo.value ), inputInfo.valid ) ;
                              break ;
                           case 'double':
                              rv = $scope.Setting.checkDouble( inputInfo.webName, trim( inputInfo.value ), inputInfo.valid ) ;
                              break ;
                           case 'port':
                              rv = $scope.Setting.checkPort( inputInfo.webName, trim( inputInfo.value ), inputInfo.valid ) ;
                              break ;
                           case 'multiple':
                              rv = $scope.Setting.checkMultiple( inputInfo.webName, trim( inputInfo.value ), inputInfo.valid ) ;
                              break ;
                           case 'group':
                              isAllClear = $scope.Setting.checkInput( inputInfo.child ) ;
                              break ;
                           case 'list':
                              if( inputInfo.valid && inputInfo.valid.min == 0 && inputInfo.child.length == 1 )
                              {
                              }
                              else
                              {
                                 var hasError = false ;
                                 $.each( inputInfo.child, function( index2 ){
                                    var rc = $scope.Setting.checkInput( inputInfo.child[index2] ) ;
                                    if( rc == false )
                                    {
                                       hasError = true ;
                                    }
                                 } ) ;
                                 if( hasError == true )
                                 {
                                    isAllClear = false ;
                                    inputInfo.error = sprintf( $scope.Setting.Text.list, inputInfo.webName ) ;
                                 }
                              }
                              break ;
                           }
                           if( rv.rc == false )
                           {
                              isAllClear = false ;
                              inputInfo.error = rv.error ;
                           }
                        } ) ;
                        if( typeof( customCheckFun ) == 'function' )
                        {
                           var rvs = customCheckFun( $scope.Setting.getValue( $scope.Setting.inputList ) ) ;
                           if( rvs.length > 0 )
                           {
                              $.each( rvs, function( index2, errInfo ){
                                 $.each( inputList, function( index3, inputInfo ){
                                    if( inputInfo.name == errInfo.name )
                                    {
                                       inputInfo.error = errInfo.error ;
                                       return false ;
                                    }
                                 } ) ;
                              } ) ;
                              isAllClear = false ;
                           }
                        }
                        return isAllClear ;
                     },
                     getValue: function( inputList ){
                        var returnValue = {} ;
                        $.each( inputList, function( index, inputInfo ){
                           switch( inputInfo.type )
                           {
                           case 'string':
                              returnValue[ inputInfo.name ] = trim( inputInfo.value ) ;
                              break ;
                           case 'password':
                              returnValue[ inputInfo.name ] = inputInfo.value ;
                              break ;
                           case 'text':
                              returnValue[ inputInfo.name ] = trim( inputInfo.value ) ;
                              break ;
                           case 'int':
                              returnValue[ inputInfo.name ] = parseInt( trim( inputInfo.value ) ) ;
                              break ;
                           case 'double':
                              returnValue[ inputInfo.name ] = parseFloat( trim( inputInfo.value ) ) ;
                              break ;
                           case 'port':
                              returnValue[ inputInfo.name ] = trim( inputInfo.value ) ;
                              break ;
                           case 'multiple':
                              inputInfo.value = [] ;
                              $.each( inputInfo.valid['list'], function( listIndex, validInfo ){
                                 if( validInfo['checked'] == true )
                                 {
                                    inputInfo.value.push( validInfo['value'] ) ;
                                 }
                              } ) ;
                              returnValue[ inputInfo.name ] = inputInfo.value ;
                              break ;
                           case 'select':
                              returnValue[ inputInfo.name ] = inputInfo.value ;
                              break ;
                           case 'checkbox':
                              returnValue[ inputInfo.name ] = inputInfo.value ;
                              break ;
                           case 'group':
                              returnValue[ inputInfo.name ] = $scope.Setting.getValue( inputInfo.child ) ;
                              break ;
                           case 'list':
                              var listValue = [] ;
                              $.each( inputInfo.child, function( index2, items ){
                                 listValue.push( $scope.Setting.getValue( items ) ) ;
                              } ) ;
                              returnValue[ inputInfo.name ] = listValue ;
                              break ;
                           case 'switch':
                              returnValue[ inputInfo.name ] = inputInfo.value ;
                              break ;
                           case 'normal':
                              returnValue[ inputInfo.name ] = inputInfo.value ;
                              break ;
                           }
                        } ) ;
                        return returnValue ;
                     },
                     getValue2: function( inputList ){
                        var returnValue = [] ;
                        $.each( inputList, function( index, inputLine ){
                           var returnLine = [] ;
                           $.each( inputLine, function( index2, inputInfo ){
                              switch( inputInfo.type )
                              {
                              case 'textual':
                              case 'string':
                              case 'select':
                              case 'checkbox':
                                 returnLine.push( inputInfo.value ) ;
                                 break ;
                              }
                           } ) ;
                           returnValue.push( returnLine ) ;
                        } ) ;
                        return returnValue ;
                     }
                  } ;
                  $scope.data.check = function( customCheckFun ){
                     return $scope.Setting.checkInput( $scope.Setting.inputList, customCheckFun ) ;
                  }
                  $scope.data.getValue = function(){
                     if( $scope.Setting.Type == 'grid' || $scope.Setting.Type == 'table' )
                     {
                        return $scope.Setting.getValue2( $scope.Setting.inputList ) ;
                     }
                     else
                     {
                        return $scope.Setting.getValue( $scope.Setting.inputList ) ;
                     }
                  }
               }
            }
            var listener = $scope.$watch( 'data', function(){
               //清除网格内容
               if( typeof( $scope.data ) == 'object' )
               {
                  init() ;
               }
            } ) ;
            var listener = $scope.$watchCollection( 'data.inputList', function(){
               //清除网格内容
               if( typeof( $scope.data ) == 'object' )
               {
                  init() ;
               }
            } ) ;
            $scope.$on( '$destroy', function(){
               listener() ;
            } ) ;
            init() ;
         },
         compile: function( element, attributes ){
            return {
               pre: function preLink( scope, element, attributes ){
               },
               post: function postLink( scope, element, attributes ){
                  scope.multipleCheck = function( item, list ){
                     list.checked = !list.checked ;
                     if( list.checked )
                     {
                        item.value.push( list.value ) ;
                     }
                     else
                     {
                        var index = item.value.indexOf( list.value ) ;
                        if( index >= 0 )
                        {
                           item.value.splice( index, 1 ) ;
                        }
                     }
                     if( typeof( item.onChange ) == 'function' )
                     {
                        item.onChange( item.name, item.value ) ;
                     }
                  }
                  scope.multipleCheckAll = function( item, status ){
                     if( status )
                     {
                        item.value = [] ;
                        $.each( item.valid.list, function( index ){
                           item.valid.list[index]['checked'] = true ;
                           item.value.push( item.valid.list[index]['value'] ) ;
                        } ) ;
                     }
                     else
                     {
                        $.each( item.valid.list, function( index ){
                           item.valid.list[index]['checked'] = false ;
                        } ) ;
                        item.value = [] ;
                     }
                     if( typeof( item.onChange ) == 'function' )
                     {
                        item.onChange( item.name, item.value ) ;
                     }
                  }
                  scope.listAppend = function( items, item ){
                     var index = items.indexOf( item ) ;
                     var newItem = [] ;
                     $.each( item, function( index2, inputInfo ){
                        var newInputInfo = $.extend( true, {}, inputInfo ) ;
                        newInputInfo['value'] = newInputInfo['default'] ;
                        newItem.push( newInputInfo ) ;
                     } ) ;
                     items.splice( index + 1, 0, newItem ) ;
                  }
                  scope.listRemove = function( items, item ){
                     if( items.length > 1 )
                     {
                        var index = items.indexOf( item ) ;
                        items.splice( index, 1 ) ;
                     }
                     else
                     {
                        $.each( item, function( index, inputInfo ){
                           if( typeof( inputInfo['default'] ) == 'undefined' )
                           {
                              inputInfo['value'] = '' ;
                           }
                           else
                           {
                              inputInfo['value'] = inputInfo['default'] ;
                           }
                        } ) ;
                     }
                  }
                  scope.onChange = function( inputInfo ){
                     if( typeof( inputInfo.onChange ) == 'function' )
                     {
                        if( inputInfo['type'] == 'select' )
                        {
                           var inputKey = '' ;
                           $.each( inputInfo.valid, function( index, item ){
                              if( inputInfo.value == item.value )
                              {
                                 inputKey = item.key ;
                                 return false ;
                              }
                           } ) ;
                           inputInfo.onChange( inputInfo.name, inputKey, inputInfo.value ) ;
                        }
                        else
                        {
                           inputInfo.onChange( inputInfo.name, inputInfo.value ) ;
                        }
                     }
                  }
                  scope.showMenu = function( inputInfo ){
                     if( inputInfo.showMenu != true )
                     {
                        var browser = SdbFunction.getBrowserInfo() ;
                        if( browser[0] == 'ie' && browser[1] == '7' )
                        {
                           return ;
                        }
                        setTimeout( function(){
                           $( document ).on( 'click', function(){
                              inputInfo.showMenu = false ;
                              $( document ).off( 'click' ) ;
                              scope.$digest();
                           } ) ;
                        } ) ;
                        inputInfo.showMenu = true ;
                     }
                  }
                  scope.placeholderClick = function( inputInfo ){
                     inputInfo.isClick = true ;
                  }
               }
            } ;
         }
      } ;
      return dire ;
   });

   //json树 key部分
   sacApp.directive( 'treeKey', function( $rootScope, SdbFunction ){
      //var img = $( '<img>' ).attr( 'src', './images/tree/object.png' ) ;
      var dire = {
         restrict: 'A',
         transclude: true,
         scope: true,
         templateUrl: './app/template/Component/TreeKey.html',
         replace: false,
         controller: function( $scope, $element ){
            $scope.Setting = {
               'items': [],
               'index': 0,
               'width': 0
            }
         },
         compile: function( element, attributes ){
            return {
               pre: function preLink( scope, element, attributes ){
                  $( element ).addClass( 'jsonTree' ) ;
               },
               post: function postLink( scope, element, attributes ){

                  var box = $( element ) ;

                  //监控配置
                  scope.$watch( attributes.treeKey, function treeKey( options ){
                     if( typeof( options ) == 'object' )
                     {
                        scope.Setting['items'] = options['json'] ;
                        scope.Setting['index'] = options['index'] ;
                     }
                  }, true ) ;

                  function resizeFun(){
                     var width = box.parent().width() ;
                     if( width == 0 )
                     {
                        width = -1 ;
                     }
                     scope.Setting['width'] = width ;
                  }

                  resizeFun() ; //为了兼容ie7

                  //重绘函数绑定到重绘队列
                  SdbFunction.defer( scope, resizeFun ) ;

                  //回收资源
                  scope.$on( '$destroy', function(){
                     scope.Setting['items'] = null ;
                     scope.Setting = null ;
                  } ) ;

                  scope.toggle = function( field ){
                     if( field.type == 'Object' || field.type == 'Array' )
                     {
                        field.isOpen = !field.isOpen ;
                     }
                  }
               }
            } ;
         }
      } ;
      return dire ;
   });

   //json树 value部分
   sacApp.directive( 'treeValue', function( $rootScope, SdbFunction ){
      var dire = {
         restrict: 'A',
         transclude: true,
         scope: true,
         templateUrl: './app/template/Component/TreeValue.html',
         replace: false,
         controller: function( $scope, $element ){
            $scope.Setting = {
               'items': [],
               'index': 0,
               'width': 0
            }
         },
         compile: function( element, attributes ){
            return {
               pre: function preLink( scope, element, attributes ){
                  $( element ).addClass( 'jsonTreeValue' ) ;
               },
               post: function postLink( scope, element, attributes ){

                  var box = $( element ) ;

                  //监控配置
                  scope.$watch( attributes.treeValue, function treeKey( options ){
                     if( typeof( options ) == 'object' )
                     {
                        scope.Setting['items'] = options['json'] ;
                        scope.Setting['index'] = options['index'] ;
                     }
                  }, true ) ;

                  function resizeFun(){
                     var width = box.parent().width() ;
                     if( width == 0 )
                     {
                        width = -1 ;
                     }
                     scope.Setting['width'] = width ;
                  }

                  resizeFun() ; //为了兼容ie7

                  //重绘函数绑定到重绘队列
                  SdbFunction.defer( scope, resizeFun ) ;

                  //回收资源
                  scope.$on( '$destroy', function(){
                     scope.Setting['items'] = null ;
                     scope.Setting = null ;
                  } ) ;

               }
            } ;
         }
      } ;
      return dire ;
   });

   //json树 type部分
   sacApp.directive( 'treeType', function( $rootScope, SdbFunction ){
      var dire = {
         restrict: 'A',
         transclude: true,
         scope: true,
         templateUrl: './app/template/Component/TreeType.html',
         replace: false,
         controller: function( $scope, $element ){
            $scope.Setting = {
               'items': [],
               'index': 0,
               'width': 0
            }
         },
         compile: function( element, attributes ){
            return {
               pre: function preLink( scope, element, attributes ){
                  $( element ).addClass( 'jsonTreeType' ) ;
               },
               post: function postLink( scope, element, attributes ){

                  var box = $( element ) ;

                  //监控配置
                  scope.$watch( attributes.treeType, function treeKey( options ){
                     if( typeof( options ) == 'object' )
                     {
                        scope.Setting['items'] = options['json'] ;
                        scope.Setting['index'] = options['index'] ;
                     }
                  }, true ) ;

                  function resizeFun(){
                     var width = box.parent().width() ;
                     scope.Setting['width'] = width ;
                  }

                  resizeFun() ; //为了兼容ie7

                  //重绘函数绑定到重绘队列
                  SdbFunction.defer( scope, resizeFun ) ;

                  //回收资源
                  scope.$on( '$destroy', function(){
                     scope.Setting['items'] = null ;
                     scope.Setting = null ;
                  } ) ;

               }
            } ;
         }
      } ;
      return dire ;
   });

   //让ng-model支持开启contenteditable的div
   sacApp.directive( 'contenteditable', function(){
      var dire = {
         restrict: 'A',
         require: '?ngModel',
         link: function( scope, element, attr, ngModel ){
            var read ;
            if( !ngModel )
            {
               return ;
            }
            ngModel.$render = function(){
               return element.text( ngModel.$viewValue ) ;
            } ;
            var keyupEvent = function(){
               if( ngModel.$viewValue !== element.text() )
               {
                  return scope.$apply( read ) ;
               }
            }
            element.bind( 'keyup', keyupEvent ) ;
            scope.$on( '$destroy', function(){
               $( element ).unbind( 'keyup', keyupEvent ) ;
            } ) ;
            return read = function() {
               return ngModel.$setViewValue( element.text() ) ;
            } ;
         }
      } ;
      return dire ;
   });

   //执行命令，类似ng-init，区别是ng-init在ng-repeat中只执行一次，但ng-eval执行多次
   sacApp.directive( 'ngEval', function(){
      var dire = {
         restrict: 'A',
         priority: 440,
         compile: function(){
            return {
               pre: function( scope, element, attrs ){
                  scope.$target = element ;
                  var lintener = scope.$watch( attrs.ngEval, function(){
                     scope.$eval( attrs.ngEval ) ;
                  } ) ;
                  scope.$on( '$destroy', function(){
                     lintener() ;
                  } ) ;
               }
            } ;
         }
      } ;
      return dire ;
   });

   //添加placeholder
   sacApp.directive( 'ngPlaceholder', function(){
      var dire = {
         restrict: 'A',
         priority: 1,
         scope: false,
         replace: false,
         controller: function( $scope, $element ){
         },
         compile: function( element, attributes ){
            return {
               pre: function preLink( scope, element, attributes ){
                  var lintener = scope.$watch( attributes.ngPlaceholder, function ngPlaceholderAction( placeholder ){
                     if( typeof( placeholder ) == 'string' )
                     {
                        $( element ).attr( 'placeholder', placeholder ) ;
                     }
                  } ) ;
                  scope.$on( '$destroy', function(){
                     lintener() ;
                  } ) ;
               },
               post: function postLink( scope, element, attributes ){
               }
            } ;
         }
      } ;
      return dire ;
   });

   //创建确认提示框
   sacApp.directive( 'createConfirm', function( $compile, $window, $rootScope ){
      var dire = {
         restrict: 'A',
         scope: {
            data: '=para'
         },
         templateUrl: './app/template/Component/Confirm.html',
         replace: false,
         // 专用控制器
      controller: function( $scope, $element ){
         $scope.Setting = {
            //弹窗样式
            Style: {
               top:'0px' ,
               left:'0px'
            },
            Mask: $( '<div></div>' ).attr( 'ng-mousedown', 'prompt()').addClass( 'mask-screen2 unalpha' ).css( 'opacity', '0.1' )
         } ;
      },

      // 编译
      compile: function( element, attributes ){
         function confirmResize( scope )
            {
               function autoResize()
               {
                  var bodyWidth = $( window ).width() ;
                  var bodyHeight = $( window ).height() ;
                  var left = ( bodyWidth - 450 ) * 0.5 ;
                  var top = ( bodyHeight - 160 ) * 0.5 ;
                  scope.Setting.Style.left = left + 'px' ;
                  scope.Setting.Style.top = top + 'px' ;
               }
               autoResize() ;
            }
         return {
            pre: function preLink( scope, element, attributes ){
               scope.data.okText = $rootScope.autoLanguage( '确定' ) ;
               scope.data.closeText = $rootScope.autoLanguage( '取消' ) ;
               //更新弹窗宽度高度坐标
               var listener1 = scope.$watch( 'data', function(){
                  if( typeof( scope.data ) == 'object' )
                  {
                     confirmResize( scope ) ;
                  }
               } ) ;
               var onResize = function () {
                  confirmResize( scope ) ;
               }
               angular.element( $window ).bind( 'resize', onResize ) ;
               var listener2 = $rootScope.$on( '$locationChangeStart', function(){
                  scope.data.isShow = false ;
               } ) ;
               scope.$on( '$destroy', function(){
                  angular.element( $window ).bind( 'resize', onResize ) ;
                  listener1() ;
                  listener2() ;
               } ) ;
            },
            post: function postLink( scope, element, attributes ){
               //编译内容
               var listener = scope.$watch( 'data.isShow', function(){
                  if( typeof( scope.data ) == 'object' )
                  {
                     if( scope.data.isShow == true )
                     {
                        setTimeout( function(){
                           $( document.body ).append( $compile( scope.Setting.Mask )( scope ) ) ;
                           confirmResize( scope ) ;
                        } ) ;
                     }
                     else
                     {
                        scope.Setting.Mask.detach() ;
                        scope.data.okText = $rootScope.autoLanguage( '确定' ) ;
                        scope.data.closeText = $rootScope.autoLanguage( '取消' ) ;
                        scope.data.ok = null ;
                        scope.data.context = '' ;
                     }
                  }
               } ) ;
               scope.$on( '$destroy', function(){
                  listener() ;
               } ) ;
               //提示框阴影效果
               scope.prompt = function(){
                  var counter = 0 ;
                  var bodyEle = $( '> .confirm ', element ) ;
                  var timer = setInterval( function(){
                     ++counter ;
                     if( counter == 1 )
                     {
                        $( bodyEle ).css( 'box-shadow','none' );
                     }
                     else if( counter == 2 )
                     {
                        $( bodyEle ).css( 'box-shadow',' 0px 2px 8px rgba(0,0,0,0.5)' );
                     }
                     if( counter == 3 )
                     {
                        $( bodyEle ).css( 'box-shadow','none' );
                     }
                     else if( counter == 4 )
                     {
                        $( bodyEle ).css( 'box-shadow',' 0px 2px 8px rgba(0,0,0,0.5)' );
                     }
                     if( counter == 5 )
                     {
                        $( bodyEle ).css( 'box-shadow','none' );
                     }
                     else if( counter == 6 )
                     {
                        $( bodyEle ).css( 'box-shadow',' 0px 2px 8px rgba(0,0,0,0.5)' );
                        counter = 0 ;
                        clearInterval( timer );
                     }
                  }, 90 ) ;
               }

               //确定
               scope.ok = function(){
                  if( typeof( scope.data.ok ) == 'function' )
                  {  
                     if( scope.data.ok() )
                     {
                        scope.close() ;
                     }
                  }
                  else
                  {
                     scope.close() ;
                  }
               } ;
               //关闭
               scope.close = function(){
                  scope.data.isShow = false ;
               } ;
            }
         } ;
      }
   } ;
      return dire ;
   } ) ;

   //取得焦点
   sacApp.directive( 'getFocus', function(){
      var dire = {
         restrict: 'A',
         scope: {
            data: '=getFocus'
         },
         replace: false,
         priority: 3,
         // 专用控制器
         controller: function( $scope, $element ){},
         // 编译
         compile: function( element, attributes ){
            return {
               pre: function preLink( scope, element, attributes ){},
               post: function postLink( scope, element, attributes ){
                  var lintener = scope.$watch( 'data', function(){
                     if( scope.data == true )
                     {
                        $( element ).get(0).focus() ;
                     }
                  } ) ;
                  scope.$on( '$destroy', function(){
                     lintener() ;
                  } ) ;
               }
            } ;
         }
      } ;
      return dire ;
   });

   //容器(自动设置宽高)
   sacApp.directive( 'ngContainer', function( $window, $rootScope ){
      var list = [] ;
      var listSize = 0 ;
      var maxLevel = 0 ;
      function _arratInsert( eleInfo )
      {
         var level = eleInfo[0] ;
         if( level >= maxLevel )
         {
            list.push( eleInfo ) ;
            ++listSize ;
            maxLevel = level ;
         }
         else
         {
            for( var i = 0; i < listSize; ++i )
            {
               if( list[i][0] > level )
               {
                  list.splice( i, 0, eleInfo ) ;
                  ++listSize ;
                  break ;
               }
            }
         }
      }
      function _renderWidth( scope, ele, width )
      {
         var marginLeft  = scope.data.marginLeft ;
         var marginRight = scope.data.marginRight ;
         var offsetX     = scope.data.offsetX ;
         marginLeft  = ( typeof( marginLeft ) != 'number' ? 0 : marginLeft ) ;
         marginRight  = ( typeof( marginRight ) != 'number' ? 0 : marginRight ) ;
         offsetX  = ( typeof( offsetX ) != 'number' ? 0 : offsetX ) ;
         if( typeof( width ) == 'number' )
         {
            width += offsetX ;
            ele.outerWidth( width ).css( { marginLeft: marginLeft, marginRight: marginRight } ) ;
         }
         else
         {
             ele.css( { marginLeft: marginLeft, marginRight: marginRight } ) ;
         }
      }
      function _renderHeight( scope, ele, height )
      {
         var marginTop    = scope.data.marginTop ;
         var marginBottom = scope.data.marginBottom ;
         var offsetY      = scope.data.offsetY ;
         marginTop  = ( typeof( marginTop ) != 'number' ? 0 : marginTop ) ;
         marginBottom  = ( typeof( marginBottom ) != 'number' ? 0 : marginBottom ) ;
         offsetY  = ( typeof( offsetY ) != 'number' ? 0 : offsetY ) ;
         if( height === 'auto' )
         {
            ele.css( { marginTop: marginTop, marginBottom: marginBottom, height: 'auto' } ) ;
         }
         else
         {
            height += offsetY ;
            ele.outerHeight( height ).css( { marginTop: marginTop, marginBottom: marginBottom } ) ;
         }
      }
      function _renderMaxHeight( scope, ele, maxheight )
      {
         var marginTop    = scope.data.marginTop ;
         var marginBottom = scope.data.marginBottom ;
         var offsetY      = scope.data.offsetY ;
         marginTop  = ( typeof( marginTop ) != 'number' ? 0 : marginTop ) ;
         marginBottom  = ( typeof( marginBottom ) != 'number' ? 0 : marginBottom ) ;
         offsetY  = ( typeof( offsetY ) != 'number' ? 0 : offsetY ) ;
         maxheight += offsetY ;
         ele.css( { 'marginTop': marginTop, 'marginBottom': marginBottom, 'maxHeight': maxheight } ) ;
      }
      function _renderPosition( scope, ele )
      {
         var top = scope.data.top ;
         var left = scope.data.left ;
         var right = scope.data.right ;
         var bottom = scope.data.bottom ;
         var position = {} ;
         if( typeof( top ) == 'number' )
            position['top'] = top ;
         if( typeof( left ) == 'number' )
            position['left'] = left ;
         if( typeof( right ) == 'number' )
            position['right'] = right ;
         if( typeof( bottom ) == 'number' )
            position['bottom'] = bottom ;
         ele.css( position ) ;
      }
      function _render( scope, ele, parent )
      {
         $( document.body ).css( 'overflow', 'hidden' ) ;
         var width  = scope.data.width ;
         var height = scope.data.height ;
         var maxHeight = scope.data.maxHeight ;
         var widthType = typeof( width ) ;
         var heightType = typeof( height ) ;
         var maxHeightType = typeof( maxHeight ) ;
         if( widthType == 'string' )
         {
            var length = width.length ;
            if( width.charAt( length - 1 ) == '%' )
            {
               width = parseInt( parent.width() * parseInt( width ) * 0.01 ) ;
            }
            else if( width == 'auto' )
            {
            }
            else
            {
               width  = parent.width() ;
            }
         }
         else if( widthType != 'number' )
         {
            width  = parent.width() ;
         }
         _renderWidth( scope, ele, width ) ;
         if( maxHeightType == 'string' )
         {
            var length = maxHeight.length ;
            if( maxHeight.charAt( length - 1 ) == '%' )
            {  
               maxHeight = parseInt( parent.height() * parseInt( maxHeight ) * 0.01 ) ;
            }
            else
            {
               maxHeight = parseInt( maxHeight ) ;
            }
            _renderMaxHeight( scope, ele, maxHeight ) ;
         }
         else
         {
            if( heightType == 'string' )
            {
               var length = height.length ;
               if( height.charAt( length - 1 ) == '%' )
               {  
                  height = parseInt( parent.height() * parseInt( height ) * 0.01 ) ;
               }
               else if( height.charAt( length - 1 ) == 'w' )
               {
                  height = parseInt( width * parseInt( height ) * 0.01 ) ;
               }
               else if( height === 'auto' )
               {
               }
               else
               {
                  height  = parent.height() ;
               }
            }
            else if( heightType != 'number' )
            {
               height  = parent.height() ;
            }
            _renderHeight( scope, ele, height ) ;
         }
         _renderPosition( scope, ele ) ;
         $( document.body ).css( 'overflow', 'auto' ) ;
      }
      function _renderAll( rootLevel )
      {
         for( var index = ( rootLevel == -1 ? 0 : rootLevel ); index < listSize; ++index ){
            _render( list[index][1], list[index][2], list[index][3] ) ;
         }
      }
      $rootScope.$watch( 'onResize', function(){
         setTimeout( function(){
            _renderAll( -1 ) ;
         } ) ;
      } ) ;
      angular.element( $window ).bind( 'resize', function () {
         _renderAll( -1 ) ;
      } ) ;
      $rootScope.$on( '$locationChangeStart', function( event, newUrl, oldUrl ){
         var newList = [] ;
         var length = listSize ;
         listSize = 0 ;
         for(var i = 0; i < length; ++i )
         {
            var element = list[i][2] ;
            if( $( element ).is( ':hidden' ) == false )
            {
               newList.push( list[i] ) ;
               ++listSize ;
               maxLevel = list[i][0] ;
            }
         }
         list = newList ;
      } ) ;
      function _append( scope, element, parent )
      {
         var level = 0 ;
         var root = element ;
         for( ; ; ++level )
         {
            if( root.get(0) == document.body || root.get(0) == document )
            {
               break ;
            }
            root = root.parent() ;
         }
         scope.level = level ;
         _arratInsert( [ level, scope, element, parent ] ) ;
      }
      var dire = {
         restrict: 'A',
         scope: true,
         replace: false,
         priority: 1,
         // 专用控制器
         controller: function( $scope, $element ){
            $scope.level = -1 ;
         },
         //编译
         compile: function( element, attributes ){
            return {
               pre: function preLink( scope, element, attributes ){},
               post: function postLink( scope, element, attributes ){
                  scope.data = scope.$eval( attributes.ngContainer ) ;
                  var current = $( element ) ;
                  var parent = $( element ).parent() ;
                  _render( scope, current, parent ) ;
                  _append( scope, current, parent ) ;
                  scope.$watch( attributes.ngContainer, function ngContainerAction( data ){
                     scope.data = data ;
                     _renderAll( scope.level ) ;
                  }, true ) ;
               }
            } ;
         }
      } ;
      return dire ;
   });

   //添加自定义属性
   sacApp.directive( 'ngAttr', function(){
      var dire = {
         restrict: 'A',
         priority: 2,
         scope: false,
         replace: false,
         // 专用控制器
         controller: function( $scope, $element ){},
         // 编译
         compile: function( element, attributes ){
            return {
               pre: function preLink( scope, element, attributes ){
                  var lintener = scope.$watch( attributes.ngAttr, function ngAttrAction( attr ){
                     if( typeof( attr ) != 'undefined' )
                     {
                        $( element ).attr( attr ) ;
                     }
                  }, true ) ;
                  scope.$on( '$destroy', function(){
                     lintener() ;
                  } ) ;
               },
               post: function postLink( scope, element, attributes ){}
            } ;
         }
      } ;
      return dire ;
   });

   //创建圆柱体
   sacApp.directive( 'createCylinder', function(){
      function getAllCoord( $scope )
      {
         function getCoord( $scope, index, height, offsetY ){
            var round   = $scope.Setting.round ;
            var width   = $scope.Setting.width - 100 ;
            var height  = height ;
            var percent = $scope.Setting.data[index]['percent'] ;
            //圆柱主干
            $scope.Setting.data[index]['zg'] = {} ;
            $scope.Setting.data[index]['zg']['x'] = 0 ;
            $scope.Setting.data[index]['zg']['y'] = round + offsetY ;
            $scope.Setting.data[index]['zg']['width'] = width - 1 ;
            $scope.Setting.data[index]['zg']['height'] = height - round * 2 - offsetY ;
            if( $scope.Setting.data[index]['zg']['height'] < 0 ) $scope.Setting.data[index]['zg']['height'] = 0 ;

            //圆柱底部
            $scope.Setting.data[index]['db'] = {} ;
            $scope.Setting.data[index]['db']['cx'] = width / 2 - 1 ;
            $scope.Setting.data[index]['db']['cy'] = height - round - 1 ;
            $scope.Setting.data[index]['db']['rx'] = width / 2 - 1 ;
            $scope.Setting.data[index]['db']['ry'] = round ;

            //圆柱主干2
            $scope.Setting.data[index]['zg2'] = {} ;
            $scope.Setting.data[index]['zg2']['x'] = 1 ;
            $scope.Setting.data[index]['zg2']['y'] = round + offsetY ;
            $scope.Setting.data[index]['zg2']['width'] = width - 3 ;
            $scope.Setting.data[index]['zg2']['height'] = height - round * 2 - offsetY - 2 ;
            if( $scope.Setting.data[index]['zg2']['height'] < 0 ) $scope.Setting.data[index]['zg2']['height'] = 0 ;

            //圆柱头部
            $scope.Setting.data[index]['tb'] = {} ;
            $scope.Setting.data[index]['tb']['cx'] = width / 2 - 1 ;
            $scope.Setting.data[index]['tb']['cy'] = round + offsetY ;
            $scope.Setting.data[index]['tb']['rx'] = width / 2 - 1 ;
            $scope.Setting.data[index]['tb']['ry'] = round ;

            //箭头
            var pointX = width - 2 ;
            var pointY = height / 2 + offsetY / 2 + 5 ;
            $scope.Setting.data[index]['jt'] = {} ;
            $scope.Setting.data[index]['jt']['d'] = 'M' + pointX + ' ' + pointY + 'L' + ( pointX + 25 ) + ' ' + ( pointY - 6 ) ;

            //字体
            $scope.Setting.data[index]['zt'] = {} ;
            $scope.Setting.data[index]['zt']['x'] = width + 29 ;
            $scope.Setting.data[index]['zt']['y'] = height / 2  + offsetY / 2 - 2 ;
         }
         $scope.Setting.data = $scope.data ;
         //圆柱真正的高度
         var height2 = $scope.Setting.height - ( $scope.Setting.round * 2 ) ;
         var height3 = 0 ;
         var len = $scope.Setting.data.length ;
         $.each( $scope.Setting.data, function( index ){
            var tmpHeight = 0 ;
            var thisHeight = height2 * ( 1 - $scope.Setting.data[index]['percent'] ) - height3 ;
            var cyOffsetY = ( index + 1 < len ) ? thisHeight : 0 ;
            if( index > 0 ) tmpHeight = 1 ;
            getCoord( $scope, index, ( $scope.Setting.height - height3 + tmpHeight ), cyOffsetY ) ;
            height3 += height2 * $scope.Setting.data[index]['percent'] ;
            $scope.Setting.data[index]['percentStr'] = parseInt( $scope.Setting.data[index]['percent'] * 100 ) + '%' ;
         } ) ;
      }
      var dire = {
         restrict: 'A',
         scope: {
            data: '=para'
         },
         replace: false,
         templateUrl: './app/template/Component/Cylinder.html',
         controller: function( $scope, $element ){
            $scope.Setting = {
               width: 200,
               height: 200,
               round: 20,
               borderColor: '#F0F0F0',
               data: []
            } ;
            var lintener = $scope.$watch( 'data', function(){
               if( isArray( $scope.data ) && $scope.data.length > 0 )
               {
                  getAllCoord( $scope ) ;
               }
            } ) ;
            scope.$on( '$destroy', function(){
               lintener() ;
            } ) ;
         },
         compile: function( element, attributes ){
            return {
               pre: function preLink( scope, element, attributes ){
                  //scope的一些初始化或者运算
               },
               post: function postLink( scope, element, attributes ){
                  setTimeout( function(){
                     scope.Setting.width = $( element ).width()  - 5 ;
                     scope.Setting.height = $( element ).height() - 5 ;
                     $( '> svg', element ).attr( { width: scope.Setting.width, height: scope.Setting.height } ) ;
                     getAllCoord( scope ) ;
                  } ) ;
               }
            } ;
         }
      } ;
      return dire ;
   } );

   //创建echart图
   sacApp.directive( 'createChart', function( $rootScope, $window ){
      var dire = {
         restrict: 'A',
         scope: {
            data: '=createChart'
         },
         replace: false,
         //专用控制器
         controller: function( $scope, $element ){
            $scope.Setting = {
               'element': null
            } ;
            var lintener1 = $scope.$watch( 'data', function(){
               if( typeof( $scope.data ) == 'object' && typeof( $scope.data.options ) == 'object' )
               {
                  if( $scope.Setting.element == null )
                  {
                     $scope.Setting.element = echarts.init( $( $element ).get(0) ).setOption( $scope.data.options ) ;
                  }
                  else
                  {
                     $scope.Setting.element.setOption( $scope.data.options ) ;
                  }
               }
            } ) ;
            var lintener2 = $scope.$watch( 'data.value', function(){
               if( typeof( $scope.data ) == 'object' && typeof( $scope.data.options ) == 'object' )
               {
                  if( $scope.Setting.element != null && isArray( $scope.data.value ) )
                  {
                     $scope.Setting.element.addData( $scope.data.value ) ;
                  }
               }
            } ) ;
            $scope.$on( '$destroy', function(){
               lintener1() ;
               lintener2() ;
            } ) ;
         },
         //编译
         compile: function( element, attributes ){
            return {
               pre: function preLink( scope, element, attributes ){
                  function resize()
                  {
                     if( scope.Setting.element != null )
                     {
                        scope.Setting.element.resize() ;
                     }
                  }
                  angular.element( $window ).bind( 'resize', resize ) ;
                  var lintener = $rootScope.$watch( 'onResize', function(){
                     setTimeout( function(){
                        resize() ;
                     } ) ;
                  } ) ;
                  scope.$on( '$destroy', function(){
                     angular.element( $window ).unbind( 'resize', resize ) ;
                     lintener() ;
                  } ) ;
               },
               post: function postLink( scope, element, attributes ){}
            } ;
         }
      } ;
      return dire ;
   });

   //创建动态框架
   sacApp.directive( 'createResponse', function( $window, $rootScope ){
      //计算一个的宽度
      function getLineWidth( parentWidth, len, column, min, max )
      {
         //步进
         var step = 0 ;
         //希望得到的列宽
         var dWidth = parseInt( parentWidth / column ) ;
         if( dWidth < min )
         {
            step = -1 ;
         }
         else if( dWidth > max )
         {
            step = 1 ;
         }
         else
         {
            return dWidth ;
         }
         while( true )
         {
            column += step ;
            dWidth = parseInt( parentWidth / column ) ;
            if( step == -1 && dWidth > min )
            {
               break ;
            }
            else if( step == 1 && dWidth < max )
            {
               break ;
            }
            else if( column == 0 )
            {
               dWidth = parentWidth ;
               break ;
            }
            else if( column == len )
            {
               dWidth = max ;
               break ;
            }
         }
         return dWidth ;
      }
      var dire = {
         restrict: 'A',
         scope: {
            data: '=createResponse'
         },
         replace: false,
         priority: 2,
         controller: function( $scope, $element ){
         },
         compile: function( element, attributes ){
            return {
               pre: function preLink( scope, element, attributes ){
                  function onResize()
                  {
                     if( typeof( scope.data.max ) == 'undefined' ) scope.data.max = 0 ;
                     var parent = $( element ).parent() ;
                     if( parent.width() == 0 )
                     {
                        return ;
                     }
                     var newWidth = getLineWidth( parent.width() - parseInt( parent.css( 'padding-left' ) ) - parseInt( parent.css( 'padding-right' ) ),
                                                  scope.data.len,
                                                  scope.data.column,
                                                  scope.data.min,
                                                  scope.data.max ) ;
                     newWidth = newWidth - parseInt( $( element ).css( 'margin-left' ) ) - parseInt( $( element ).css( 'margin-right' ) ) ;
                     $( element ).css( 'float', 'left' ).outerWidth( newWidth ) ;
                  }
                  angular.element( $window ).bind( 'resize', onResize ) ;
                  var listener2 = $rootScope.$watch( 'onResize', function(){
                     setTimeout( onResize ) ;
                  } ) ;
                  var listener3 = scope.$watch( 'data', function(){
                     onResize() ;
                  } ) ;
                  scope.$on( '$destroy', function(){
                     angular.element( $window ).unbind( 'resize', onResize ) ;
                     listener2() ;
                     listener3() ;
                  } ) ;
               },
               post: function postLink( scope, element, attributes ){
               }
            } ;
         }
      } ;
      return dire ;
   });

   //滚动条自动到底部
   sacApp.directive( 'scrollBottom', function( $parse, $window, $timeout, SdbFunction ){
      function createActivationState($parse, attr, scope){
         function unboundState(initValue){
            var activated = initValue;
            return {
                  getValue: function(){
                     return activated;
                  },
                  setValue: function(value){
                     activated = value;
                  }
            };
         }

         function oneWayBindingState(getter, scope){
            return {
                  getValue: function(){
                     return getter(scope);
                  },
                  setValue: function(){}
            }
         }

         function twoWayBindingState(getter, setter, scope){
            return {
                  getValue: function(){
                     return getter(scope);
                  },
                  setValue: function(value){
                     if(value !== getter(scope)){
                        scope.$apply(function(){
                              setter(scope, value);
                        });
                     }
                  }
            };
         }

         if(attr !== ""){
            var getter = $parse(attr);
            if(getter.assign !== undefined){
                  return twoWayBindingState(getter, getter.assign, scope);
            } else {
                  return oneWayBindingState(getter, scope);
            }
         } else {
            return unboundState(true);
         }
      }
      var direction = {
         isAttached: function(el){
            // + 1 catches off by one errors in chrome
            return el.scrollTop + el.clientHeight + 1 >= el.scrollHeight;
         },
         scroll: function(el){
            el.scrollTop = el.scrollHeight;
         }
      } ;
      return {
         priority: 1,
         restrict: 'A',
         link: function(scope, $el, attrs){
            var el = $el[0],
               activationState = createActivationState($parse, attrs['scrollBottom'], scope);

            function scrollIfGlued() {
               if(activationState.getValue() && !direction.isAttached(el)){
                     direction.scroll(el);
               }
            }

            function onScroll() {
               activationState.setValue(direction.isAttached(el));
            }

            scope.$watch( scrollIfGlued ) ;

            $timeout(scrollIfGlued, 0, false);

            var browser = SdbFunction.getBrowserInfo() ;
            if( ( browser[0] == 'ie' && browser[1] >= 9 ) ||  browser[0] != 'ie' )
            {
               //ie7、8不支持addEventListener
               $window.addEventListener('resize', scrollIfGlued, false);
            }

            $el.bind('scroll', onScroll);


            // Remove listeners on directive destroy
            $el.on('$destroy', function() {
               $el.unbind('scroll', onScroll);
            });

            scope.$on('$destroy', function() {
               $window.removeEventListener('resize',scrollIfGlued, false);
            });
         }
      } ;
   } ) ;

   //进度条
   sacApp.directive( 'progressBar', function( $window, $rootScope ){
      var dire = {
         restrict: 'A',
         scope: {
            data: '=progressBar'
         },
         templateUrl: './app/template/Component/ProgressBar.html',
         replace: false,
         controller: function( $scope, $element ){
            $scope.Setting = {
               'style': {
                  'box':      { 'height': '20px', 'background': '#CDD7E1' },
                  'progress': { 'height': '20px', 'background': '#188FF1', 'width': '0%' },
                  'context':  { 'height': '16px', 'padding-top': '4px', 'color': '#fff', 'text-shadow': '#aaa 0 1px 2px' }
               },
               'context': '0%'
            } ;
         },
         compile: function( element, attributes ){
            return {
               pre: function preLink( scope, element, attributes ){
                  var lintener = scope.$watch( 'data', function(){
                     if( typeof( scope.data ) == 'object' )
                     {
                        if( typeof( scope.data.percent ) == 'number' )
                        {
                           scope.Setting.style.progress.width = scope.data.percent + '%' ;
                           if( typeof( scope.data.text ) == 'string' )
                           {
                              scope.Setting.context = scope.data.text ;
                           }
                           else
                           {
                              scope.Setting.context = scope.data.percent + '%' ;
                           }
                        }
                        if( typeof( scope.data.style ) == 'object' )
                        {
                           if( typeof( scope.data.style.box ) == 'object' )
                           {
                              $.each( scope.data.style.box, function( key, val ){
                                 scope.Setting.style.box[key] = val ;
                              } ) ;
                           }
                           if( typeof( scope.data.style.progress ) == 'object' )
                           {
                              $.each( scope.data.style.progress, function( key, val ){
                                 scope.Setting.style.progress[key] = val ;
                              } ) ;
                           }
                           if( typeof( scope.data.style.context ) == 'object' )
                           {
                              $.each( scope.data.style.context, function( key, val ){
                                 scope.Setting.style.context[key] = val ;
                              } ) ;
                           }
                        }
                     }
                  } ) ;
                  scope.$on( '$destroy', function(){
                     lintener() ;
                  } ) ;
               },
               post: function postLink( scope, element, attributes ){
               }
            } ;
         }
      } ;
      return dire ;
   });

   //步骤条
   sacApp.directive( 'stepChart', function( $window, $rootScope ){
      var dire = {
         restrict: 'A',
         scope: {
            data: '=stepChart'
         },
         templateUrl: './app/template/Component/StepChart.html',
         replace: false,
         controller: function( $scope, $element ){
            $scope.Setting = {
               'bar': [],
               'text': []
            } ;
            $scope.onclick = function( index ){
               if( typeof( $scope.data ) == 'object' && typeof( $scope.data['info'][index]['click'] ) == 'function' )
               {
                  $scope.data['info'][index]['click']( index + 1 ) ;
               }
            } ;
         },
         compile: function( element, attributes ){
            return {
               pre: function preLink( scope, element, attributes ){
                  function setChart()
                  {
                     scope.Setting.bar = [] ;
                     if( typeof( scope.data ) == 'object' )
                     {
                        var step = scope.data.step ;
                        var width = $( element ).width() - 170 ;
                        var txtNum = scope.Setting.text.length ;
                        var barNum = txtNum - 1 ;
                        var barWidth = parseInt( width / barNum ) ;
                        for( var i = 0; i < barNum; ++i )
                        {
                           scope.Setting.bar.push( { 'width': barWidth + 'px' } ) ;
                           if( i + 1 >= step )
                           {
                              scope.Setting.bar[i]['background'] = '#ddd' ;
                           }
                        }
                        for( var i = 0; i < txtNum; ++i )
                        {
                           var currentLeft = barWidth * i - 14 ;
                           scope.Setting.text[i]['style'] = { 'left': currentLeft + 'px' } ;
                           if( i + 1 > step )
                           {
                              scope.Setting.text[i]['style']['background'] = '#ddd' ;
                           }
                        }
                     }
                  }
                  var lintener1 = scope.$watch( 'data', function(){
                     if( typeof( scope.data ) == 'object' )
                     {
                        $.each( scope.data.info, function( index, info ){
                           scope.Setting.text.push( { 'text': info.text } ) ;
                        } ) ;
                        setChart() ;
                     }
                  } ) ;
                  angular.element( $window ).bind( 'resize', setChart ) ;
                  var lintener2 = $rootScope.$watch( 'onResize', function(){
                     setTimeout( setChart ) ;
                  } ) ;
                  scope.$on( '$destroy', function(){
                     angular.element( $window ).unbind( 'resize', setChart ) ;
                     lintener1() ;
                     lintener2() ;
                  } ) ;
               },
               post: function postLink( scope, element, attributes ){
               }
            } ;
         }
      } ;
      return dire ;
   });

   /*
   下拉菜单（新版）
   支持命令： ng-dropdown         必填   [item] in [array_value]  //列表数据
                                 array_value: [
                                    { 'xxx': xxxx },
                                    { 'xxx': xxxx },
                                    { 'divider': true }, //分割线
                                    { 'xxx': xxxx, ...., 'disabled': true } //禁用
                                    { 'xxx': xxxx }
                                 ]
             dropdown-event      可选   {}                       //下拉菜单的事件
                                                                 OnOpen( isOpen )
                                                                 OnClose( isClose )
             dropdown-callback   可选   {}                       //下拉菜单接口
                                                                 Open()
                                                                 Close()
   */
   sacApp.directive( 'ngDropdown', function( $compile, $animate, SdbFunction ){
      var dire = {
         restrict: 'A',
         replace: false,
         transclude: true,
         scope: true,
         controller: function( $scope, $element, $attrs, $transclude ){
            $scope.event = {} ;      //事件
            $scope.lastScope = [] ;  //最后一次创建的scope
            $scope.lastLi = [] ;
            $scope.mask = $compile( $( '<div></div>' ).attr( 'ng-mousedown', 'close()' ).addClass( 'mask-screen unalpha' ) )( $scope ) ;  //遮罩
            $scope.ulBox = $( '<ul class="dropdown-menu"></ul>' ).css( { 'position': 'relative' } ) ;  //下拉菜单外框
            $scope.divBox = $( '<div></div>' ).css( { 'position': 'absolute', 'left': 0, 'top': 0, 'z-index': 10000 } ) ; //下拉菜单移动框

            $scope.status = 0 ; //当前下拉菜单状态，1:开启  0:关闭
            $scope.btnEle = null ;

            $scope.divBox.append( $scope.ulBox ) ;//把下拉菜单放到制定的位置
         },
         compile: function( element, attributes, transclude ){
            return {
               pre: function preLink( scope, element, attributes ){},
               post: function postLink( scope, element, attributes ){

                  scope.$on( '$destroy', function(){
                     //主scope释放，子的scope也要释放
                     $.each( scope.lastScope, function( index, rowScope ){
                        rowScope.$destroy();
                        rowScope = null ;
                     } ) ;
                     //移除dom
                     $.each( scope.lastLi, function( index, liEle ){
                        $animate.leave( liEle ) ;
                        liEle = null ;
                     } ) ;
                  } ) ;

                  //解析表达式
                  var expression = attributes.ngDropdown ;
                  var match = expression.match(/^\s*([\s\S]+?)\s+in\s+([\s\S]+?)\s*$/) ;
                  if( !match )
                  {
                     throw "Expected expression in form of '_item_ in _collection_: " + expression ;
                  }
                  var item = match[1] ;
                  var rhs  = match[2] ;

                  //渲染下拉菜单
                  var createDropdown = function( dataList ){

                     //释放旧的scope
                     $.each( scope.lastScope, function( index, rowScope ){
                        rowScope.$destroy();
                        rowScope = null ;
                     } ) ;

                     //删除旧的元素
                     $.each( scope.lastLi, function( index, liEle ){
                        $animate.leave( liEle ) ;
                        liEle = null ;
                     } ) ;

                     scope.lastScope = [] ;

                     var ulBox = scope.ulBox ;

                     var length = dataList.length ;

                     $.each( dataList, function( index, dataInfo ){

                        var childScope = scope.$new();   //创建新的scope

                        scope.lastScope.push( childScope ) ;   //存储新的scope，用来释放

                        childScope['$index'] = index ;

                        childScope[item] = dataList[index] ;

                        transclude( childScope, function( clone ){

                           if( dataList[index]['divider'] === true )
                           {
                              var li = angular.element( '<li></li>' ) ;
                              scope.lastLi.push( li ) ;
                              $( li ).addClass( 'divider' ) ;
                              ulBox.append( li ) ;
                           }
                           else
                           {
                              $.each( clone, function( index2, col ){
                                 if( col.nodeType == 1 )
                                 {
                                    if( $( col ).attr( 'dropdown-config' ) == 'last' && index + 1 < length ) //如果有最后的属性，就只有最后才加入
                                    {
                                       return true ;
                                    }
                                    var li = angular.element( '<li></li>' ) ;
                                    if( dataList[index]['disabled'] === true )
                                    {
                                       li.addClass( 'drop-disabled' ) ;
                                    }
                                    else
                                    {
                                       li.addClass( 'event' ) ;
                                    }
                                    scope.lastLi.push( li ) ;
                                    $( li ).append( col ) ;
                                    ulBox.append( li ) ;
                                 }
                              } ) ;
                           }

                        } ) ;
                     } ) ;
                  }

                  //重绘
                  var resizeFun = function(){
                     if( scope.status == 0 )
                        return ;
                     var ele        = scope.btnEle ;
                     var menu       = scope.divBox ;
                     var ulBox      = scope.ulBox ;
                     var bodyWidth  = $( 'body' ).outerWidth() ;
                     var bodyHeight = $( 'body' ).outerHeight() ;
                     var eleWidth   = $( ele ).outerWidth() ;
                     var eleHeight  = $( ele ).outerHeight() ;
                     var menuWidth  = $( ulBox ).outerWidth() ;
                     var menuHeight = $( ulBox ).outerHeight() ;
                     var left = $( ele ).offset().left ;
                     var top  = $( ele ).offset().top ;
                     if( left + menuWidth > bodyWidth )
                     {
                        left = bodyWidth - menuWidth ;
                     }
                     left = left <= 0 ? 0 : left ;
                     if( top + eleHeight + menuHeight > bodyHeight )
                     {
                        //判断按钮在页面的上面还是下面
                        var isTop = top + parseInt( eleHeight * 0.5 ) <= ( bodyHeight * 0.5 ) ;
                        if( isTop == true )
                        {
                           top = top + eleHeight ;
                           $( ulBox ).css('overflow-y', 'auto').height( parseInt( ( bodyHeight - top - 20 ) * 0.8 ) ) ;
                        }
                        else
                        {
                           if( top - menuHeight - 3 > 0 )
                           {
                              top = top - menuHeight - 3 ;
                           }
                           else
                           {
                              menuHeight = parseInt( ( top - 20 ) * 0.8 ) ;
                              $( ulBox ).css('overflow-y', 'auto').height( menuHeight ) ;
                              top = top - menuHeight - 13 ;
                           }
                        }
                     }
                     else
                     {
                        top = top + eleHeight ;
                     }
                     $( menu ).css( { 'left': left, 'top': top } )  ;
                  }

                  //打开下拉菜单
                  var open = function( btnEle ){
                     if( typeof( scope.event['OnOpen'] ) == 'function' && scope.event['OnOpen']( scope.status == 1 ) === false )
                     {
                        return ;
                     }
                     $( element ).height( 0 ) ; //兼容IE7
                     if( scope.status == 0 )
                     {
                        var ulBox = scope.ulBox ;
                        scope.btnEle = $( btnEle ) ;
                        scope.status = 1 ;
                        $( document.body ).append( scope.divBox ) ;
                        $( document.body ).append( scope.mask ) ;
                        ulBox.show() ;
                        resizeFun() ;
                     }
                  }

                  //关闭下拉菜单
                  var close = function(){
                     if( typeof( scope.event['OnClose'] ) == 'function' && scope.event['OnClose']( scope.status == 0 ) === false )
                     {
                        return ;
                     }
                     if( scope.status == 1 )
                     {
                        var ulBox = scope.ulBox ;
                        scope.status = 0 ;
                        $( scope.mask ).detach() ;
                        $( scope.divBox ).detach() ;
                        ulBox.hide() ;
                     }
                  }                 

                  //监控回调函数
                  scope.$watch( attributes.dropdownCallback, function( callback ){
                     if( typeof( callback ) == 'object' )
                     {
                        callback['Open'] = open ;
                        callback['Close'] = close ;
                     }
                  } ) ;

                  //监控事件函数
                  scope.$watch( attributes.dropdownEvent, function( event ){
                     if( typeof( event ) == 'object' )
                     {
                        scope.event = event ;
                     }
                  } ) ;

                  //监控数据
                  scope.$watchCollection( rhs, function( collections ){
                     $( element ).height( 0 ) ; //兼容IE7
                     if( isArray( collections ) )  //必须是数组
                       createDropdown( collections ) ;
                  } ) ;

                  scope.close = function(){
                     close() ;
                  }

                  //重绘函数绑定到重绘队列
                  SdbFunction.defer( scope, resizeFun ) ;

               }
            } ;
         }
      } ;
      return dire ;
   } ) ;

   //下拉菜单(准备废弃)
   sacApp.directive( 'dropdownMenu', function( $window, $rootScope, $compile ){
      var menu = $( '<ul></ul>' ).addClass( 'dropdown-menu' ).appendTo( $( 'body' ) ) ;
      var mask = $( '<div></div>' ).addClass( 'mask-screen unalpha' ).appendTo( $( 'body' ) ).hide().on( 'click', function(){
         $( mask ).hide() ;
         $( menu ).hide().css( { 'overflow-y': 'visible', 'height': 'auto' } ) ;
      } ) ;
      var dire = {
         restrict: 'A',
         scope: {
            data: '=dropdownMenu'
         },
         replace: false,
         controller: function( $scope, $element ){
         },
         compile: function( element, attributes ){
            var menuHide = function(){
               $( menu ).hide().css( { 'overflow-y': 'visible', 'height': 'auto' } ) ;
            }
            var maskHide = function(){
               $( mask ).hide() ;
            }
            var onClickEvent = function( scope, index ){
               if( typeof( scope.data[index]['onClick'] ) == 'function' )
               {
                  scope.data[index]['onClick'](  menuHide, maskHide ) ;
               }
               else
               {
                  $( mask ).hide() ;
                  $( menu ).hide().css( { 'overflow-y': 'visible', 'height': 'auto' } ) ;
               }
            }
            var onMove = function( ele ){
               var bodyWidth  = $( 'body' ).outerWidth() ;
               var bodyHeight = $( 'body' ).outerHeight() ;
               var eleWidth   = $( ele ).outerWidth() ;
               var eleHeight  = $( ele ).outerHeight() ;
               var menuWidth  = $( menu ).outerWidth() ;
               var menuHeight = $( menu ).outerHeight() ;
               var left = $( ele ).offset().left ;
               var top  = $( ele ).offset().top ;
               if( left + menuWidth > bodyWidth )
               {
                  left = bodyWidth - menuWidth ;
               }
               left = left <= 0 ? 0 : left ;
               if( top + eleHeight + menuHeight > bodyHeight )
               {
                  //判断按钮在页面的上面还是下面
                  var isTop = top + parseInt( eleHeight * 0.5 ) <= ( bodyHeight * 0.5 ) ;
                  if( isTop == true )
                  {
                     top = top + eleHeight ;
                     $( menu ).css('overflow-y', 'auto').height( parseInt( ( bodyHeight - top - 20 ) * 0.8 ) ) ;
                  }
                  else
                  {
                     if( top - menuHeight - 3 > 0 )
                     {
                        top = top - menuHeight - 3 ;
                     }
                     else
                     {
                        menuHeight = parseInt( ( top - 20 ) * 0.8 ) ;
                        $( menu ).css('overflow-y', 'auto').height( menuHeight ) ;
                        top = top - menuHeight - 13 ;
                     }
                  }
               }
               else
               {
                  top = top + eleHeight ;
               }
               $( menu ).css( { 'left': left, 'top': top } )  ;
            }
            return {
               pre: function preLink( scope, element, attributes ){
                  $( element ).on( 'click', function(){
                     $( menu ).empty() ;
                     if( typeof( scope.data ) == 'object' )
                     {
                        var rowNum = scope.data.length ;
                        for( var index = 0; index < rowNum; ++index )
                        {
                           if( typeof( scope.data[index]['html'] ) == 'object' )
                           {
                              if( scope.data[index]['disabled'] == true )
                              {
                                 $( '<li></li>' ).addClass( 'drop-disabled' ).append( scope.data[index]['html'] ).appendTo( menu ) ;
                              }
                              else
                              {
                                 (function( index ){
                                    $( '<li></li>' ).addClass( 'event' ).append( scope.data[index]['html'] ).appendTo( menu ).on( 'click', function(){
                                       onClickEvent( scope, index ) ;
                                    } ) ;
                                 })( index ) ;
                              }
                           }
                           else
                           {
                              $( '<li></li>' ).addClass( 'divider' ).appendTo( menu ) ;
                           }
                        }
                        $( mask ).show() ;
                        $( menu ).show().css( { 'left': 0, 'top': 0 } )  ;
                        onMove( element ) ;
                     }
                  } ) ;
                  var onResize = function () {
                     if( $( menu ).is( ':hidden' ) == false )
                     {
                        onMove( element ) ;
                     }
                  }
                  angular.element( $window ).bind( 'resize', onResize ) ;
                  var lintener = $rootScope.$watch( 'onResize', onResize ) ;
                  scope.$on( '$destroy', function(){
                     angular.element( $window ).unbind( 'resize', onResize ) ;
                     lintener() ;
                  } ) ;
               },
               post: function postLink( scope, element, attributes ){
               }
            } ;
         }
      } ;
      return dire ;
   });

   //落地窗
   sacApp.directive( 'frenchWindow', function( $rootScope, $window, $compile ){
      var mask = $( '<div></div>' ).addClass( 'mask-screen unalpha' ) ;
      var boxOffset  = 94 ;
      var bodyOffset = boxOffset + 47 ;
      var dire = {
         restrict: 'A',
         scope: {
            data: '=frenchWindow'
         },
         templateUrl: './app/template/Component/FrenchWindow.html',
         replace: false,
         controller: function( $scope, $element ){
            $( $element ).addClass( 'frenchWindow' ) ;
         },
         compile: function( element, attributes ){
            var headEle = $( '> .header', element ) ;
            var bodyEle = $( '> .body', element ) ;
            var resize = function(){
               var bodyHeight = $( 'body' ).outerHeight() ;
               bodyEle.height( bodyHeight - bodyOffset ) ;
               $( element ).height( bodyHeight - boxOffset ) ;
            }
            return {
               pre: function preLink( scope, element, attributes ){
                  mask.on( 'click', function(){
                     scope.data.isShow = false ;
                     scope.$apply() ;
                  } ) ;
               },
               post: function postLink( scope, element, attributes ){
                  var onResize = function () {
                     if( scope.data.isShow == true )
                     {
                        resize() ;
                     }
                  }
                  var lintener1 = scope.$watch( 'data.isShow', function(){
                     if( typeof( scope.data ) != 'undefined' && scope.data.isShow == true )
                     {
                        headEle.text( scope.data.title ) ;
                        if( scope.data.Context == null )
                        {
                           var bodyHeight = $( 'body' ).outerHeight() ;
                           bodyEle.text( scope.data.empty ) ;
                           bodyEle.css( { 'line-height': ( bodyHeight - bodyOffset ) + 'px', 'text-align': 'center', 'color': '#666' } ) ;
                        }
                        else if( typeof( scope.data.Context ) == 'object' )
                        {
                           bodyEle.css( { 'line-height': 'normal', 'text-align': 'left', 'color': '#000' } ) ;
                           bodyEle.html( scope.data.Context ) ;
                        }
                        else if( typeof( scope.data.Context ) == 'string' )
                        {
                           bodyEle.css( { 'line-height': 'normal', 'text-align': 'left', 'color': '#000' } ) ;
                           bodyEle.html( $compile( scope.data.Context )( scope ) ) ;
                        }
                        resize() ;
                        $( element ).show() ;
                        $( mask ).appendTo( $( 'body' ) ) ;
                     }
                     else
                     {
                        $( element ).hide() ;
                        $( mask ).detach() ;
                     }
                  } ) ;
                  angular.element( $window ).bind( 'resize', onResize ) ;
                  var lintener2 = $rootScope.$watch( 'onResize', onResize ) ;
                  scope.$on( '$destroy', function(){
                     angular.element( $window ).unbind( 'resize', onResize ) ;
                     lintener1() ;
                     lintener2() ;
                  } ) ;
               }
            } ;
         }
      } ;
      return dire ;
   });

   /*
   定时器
   支持命令： create-timer     必填 {}    配置
                  {
                     'interval': 5       //每隔x秒执行回调
                  }
             timer-callback   可选 {}    //定时器接口
                                        Start( 定时执行的函数 )    启动定时器
                                        Complete()                继续开始下一次
                                        Stop()                    停止定时器
                                        GetStatus()               获取定时器状态, true: 运行中， false: 停止了
                                        SetInterval( 秒 )         设置定时器间隔
                                        GetInterval()             获取定时器间隔
   */
   sacApp.directive( 'createTimer', function( $timeout, $interval ){
      var rate = 20 ; //定时器速率
      var dire = {
         restrict: 'A',
         scope: true,
         replace: false,
         controller: function( $scope, $element ){
            $scope.Setting = {
               options: {
                  'width': '0%',
                  'height': '2px',
                  'backgroundColor': '#DDD'
               },
               status: 'stop',
               interval: 5000,
               func: null,
               timer: null
            }
         },
         compile: function( element, attributes ){
            return {
               pre: function preLink( scope, element, attributes ){
                  $( element ).css( scope.Setting['options'] ) ;
               },
               post: function postLink( scope, element, attributes ){

                  var timer = function(){
                     if( scope.Setting.status == 'start' )
                     {
                        scope.Setting.currentTime += rate ;
                        var percent = ( scope.Setting.currentTime / scope.Setting.interval * 100 ) ;
                        percent =　percent > 100 ? 100 + '%' : percent + '%' ;
                        $( element ).css( 'width', percent ) ;
                        if( scope.Setting.currentTime >= scope.Setting.interval )
                        {
                           scope.Setting.currentTime = 0 ;
                           scope.Setting.status = 'complete' ;
                           $timeout( scope.Setting.func, 10, false ) ;
                           $interval.cancel( scope.Setting.timer ) ;
                           scope.Setting.timer = null ;
                        }
                     }
                     else
                     {
                        $( element ).css( 'width', '0%' ) ;
                     }
                  }

                  //开始定时器
                  var start = function( func ){
                     if( scope.Setting.status == 'stop' || scope.Setting.status == 'complete' )
                     {
                        scope.Setting.func = func ;
                        scope.Setting.status = 'start' ;
                        scope.Setting.options.width = '0%' ;
                        scope.Setting.currentTime = 0 ;
                        scope.Setting.timer = $interval( timer, rate, 0, false ) ;
                     }
                  }

                  //继续开始下一次
                  var complete = function(){
                     if( scope.Setting.status == 'stop' || scope.Setting.status == 'complete' )
                     {
                        scope.Setting.status = 'start' ;
                        scope.Setting.options.width = '0%' ;
                        scope.Setting.currentTime = 0 ;
                        scope.Setting.timer = $interval( timer, rate, 0, false ) ;
                     }
                  }

                  //停止定时器
                  var stop = function(){
                     scope.Setting.status = 'stop' ;
                     $( element ).css( { 'width': '0%' } ) ;
                     if( scope.Setting.timer !== null )
                        $interval.cancel( scope.Setting.timer ) ;
                  }

                  //获取定时器状态, true: 运行中， false: 停止了
                  var getStatus = function(){
                     if( scope && scope.Setting )
                     {
                        return scope.Setting.status ;
                     }
                     return false ;
                  }

                  //设置定时器时间
                  var setTimerInterval = function( seconds ){
                     scope.Setting['interval'] = seconds * 1000 ;
                  }

                  //获取定时器时间
                  var getTimerInterval = function( seconds ){
                     return scope.Setting['interval'] / 1000 ;
                  }

                  //回收资源
                  scope.$on( '$destroy', function(){
                     stop() ;
                     scope.Setting['options'] = null ;
                     scope.Setting = null ;
                  } ) ;

                  scope.$watch( attributes.timerCallback, function( callback ){
                     if( typeof( callback ) == 'object' )
                     {
                        callback['Start'] = start ;
                        callback['Complete'] = complete ;
                        callback['Stop']  = stop ;
                        callback['GetStatus']  = getStatus ;
                        callback['SetInterval'] = setTimerInterval ;
                        callback['GetInterval'] = getTimerInterval ;
                     }
                  } ) ;

                  scope.$watch( attributes.createTimer, function( options ){
                     $.each( scope.Setting, function( key ){
                        if( typeof( options[key] ) != 'undefined' )
                        {
                           if( key == 'interval' )
                              scope.Setting[key] = options[key] * 1000 ;
                           else
                              scope.Setting[key] = options[key] ;
                        }
                     } ) ;
                     if( scope.Setting['beginRun'] === true &&
                         typeof( scope.Setting['RunFun'] ) == 'function' )
                     {
                        start( scope.Setting['RunFun'] ) ;
                     }

                     $( element ).css( scope.Setting['options'] ) ;
                  } ) ;
               }
            } ;
         }
      } ;
      return dire ;
   });

   /*
   ng-repeat的修改版，区别在于不加载全部，只有滚动条差不多到底部才继续加载
   支持命令： ng-repeats  必填 []     数据
             loadfirst   可选 正整数  第一次加载多少, 默认100
             loadnext    可选 正整数  后续一次加载多少, 默认100
   */
   sacApp.directive( 'ngRepeats', function( $animate ){
      var dire = {
         restrict: 'A',
         replace: false,
         transclude: true,
         terminal: true,
         controller: function( $scope, $element, $attrs, $transclude ){
            //最后一次创建的scope列表
            $scope.lastScope = [] ;
            //已经加载多少
            $scope.loadNum = 0 ;
            //总共多少
            $scope.length = 0 ;
            //数据
            $scope.collection = [] ;
            //默认渲染多少个
            $scope.loadfirst = isNaN( $attrs.loadfirst ) ? 100 : parseInt( $attrs.loadfirst ) ;
            //后续渲染多少个
            $scope.loadnext = isNaN( $attrs.loadnext ) ? 100 : parseInt( $attrs.loadnext ) ;
         },
         compile: function( element, attributes, transclude ){
            return {
               pre: function preLink( scope, element, attributes ){
               },
               post: function postLink( scope, element, attributes ){

                  scope.$on( '$destroy', function(){  //主scope释放，子的scope也要释放

                     $.each( scope.lastScope, function( index, rowScope ){
                        rowScope.$destroy();
                        rowScope = null ;
                     } ) ;

                     //移除旧的dom
                     angular.forEach( element.children(), function( ele ){
                        $animate.leave( ele );
                     } ) ;

                  } ) ;

                  //解析表达式
                  var expression = attributes.ngRepeats;
                  var match = expression.match(/^\s*([\s\S]+?)\s+in\s+([\s\S]+?)\s*$/) ;
                  if( !match )
                  {
                     throw "Expected expression in form of '_item_ in _collection_: " + expression ;
                  }
                  var item = match[1] ;
                  var rhs  = match[2] ;

                  //渲染内容
                  var renderFun = function( startIndex, endIndex ){

                     for( var index = startIndex; index < endIndex; ++index )
                     {
                        var childScope = scope.$new();

                        scope.lastScope.push( childScope ) ;

                        childScope['$index'] = index ;

                        childScope[ item ] = scope.collection[index] ;

                        transclude( childScope, function( clone ){

                           $animate.enter( clone, element, null ) ;

                        } ) ;

                     }
                  }

                  //监控滚动条
                  element.bind( 'scroll', function(){

                     //如果滚动条接近底部
                     if( element[0].scrollTop + element[0].offsetHeight >= element[0].scrollHeight - 150 )
                     {
                        //还没加载全部
                        if( scope.loadNum < scope.length )
                        {
                           var length = scope.length - scope.loadNum ;
                           var startIndex = scope.loadNum ;
                           var endIndex = 0 ;

                           length = length > scope.loadnext ? scope.loadnext : length ;

                           endIndex = length + startIndex ;

                           scope.loadNum += length ;

                           renderFun( startIndex, endIndex ) ;
                        }
                     }
                  } ) ;

                  //监控数组
                  scope.$watchCollection( rhs, function ngTable( collection ){
                     
                     scope.collection = collection ;

                     //移除旧的
                     angular.forEach( element.children(), function( ele ){
                        $animate.leave( ele );
                     } ) ;

                     //释放旧的scope
                     $.each( scope.lastScope, function( index, rowScope ){
                        rowScope.$destroy();
                        rowScope = null ;
                     } ) ;

                     scope.lastScope = [] ;

                     //创建新的
                     var length = collection.length ;

                     scope.length = length ;

                     length = length > scope.loadfirst ? scope.loadfirst : length ;

                     scope.loadNum = length ;

                     renderFun( 0, length ) ;

                  } ) ;

               }
            } ;
         }
      } ;
      return dire ;
   } ) ;

   /*
   表格
   支持命令： ng-table       必填 {}    表格的配置项
                  {
                     'width': [],      //控制表格宽度, 支持 '10px' 和 '10%', 'auto' 3种写法, 也可以混合写，默认就是auto
                     'tools': true,    //是否开启工具栏, 如果false, 就不能开启换页功能，默认 true
                     'max': 100        //一页最大显示多少行， 默认 100
                     'trim': true,     //是否允许调整表格宽度, 默认开启
                     'mode': 'normal', //模式， normal: 一次性加载全部数据，由内置方法控制表格换页； dynamic: 由外部提供方法控制换页，每一页都是通过外部获取。
                     'sort': [],       //是否开启排序，数组仅支持bool类型的元素，排序数组和标题数组一一对应
                     'autoSort': {}    //自动排序，当第一次数据写入到表格、表格数据改变时，将会自动排序
                                         格式:
                                          { 'key': '排序的字段', 'asc': true }  asc为true就是正序，从小到大
                     'filter': {}      //是否开启过滤功能，支持4种模式, key对应title的key
                                         1. 'indexof': 模糊匹配，只要在内容找到字符串子串就匹配成功。 输入框
                                         2. 任意字符串: 正则匹配，按照正则规则来匹配。 输入框
                                         3. 函数:      自定义函数来匹配，函数返回true或false, true就是匹配成功，参数由2个，第一个是表格的值，第二个是过滤的值。 输入框
                                         4. 数组:      根据数组的值匹配表格，建议数组第一个是空字符串，这样可以默认不匹配。 下拉菜单
                                         5. 'number':  根据数值匹配，有匹配符。下拉菜单 + 输入框
                     'default': {},    //如果开启过滤，是否要设置默认值，如果不填，默认是''
                     'text': {
                        'default': 'xxx' 默认显示在状态栏的文本，第一个?是当前表格行数，第二个?是总行数
                        'filterDefault': 'xxx' 设置过滤后显示在状态栏的文本，第一个?是当前表格行数，第二个?是总行数
                     }
                  }
             table-title    必填 {}    表格的标题， key是要对应该列的字段名，用于排序和过滤， 如果value是false，那么该列不显示
             table-content  必填 []    表格的内容
             table-key      必填 ""    列名，对应table-title的key
             table-callback 可选 {}    只要空对象就行，指令会把回调函数传回来
                                       GetPageData( 指定第几页 )
                                       GetAllData()
                                       GetFilterPageData( 指定第几页 )
                                       GetFilterAllData()
                                       GetFilterStatus()
                                       GetCurrentPageNum()
                                       GetSumPageNum()
                                       ResizeTableHeader()
                                       ResetBodyTop()
                                       ResetBodyTopAfterRender()
                                       SortData( 字段名, 是否正序(boolean) )
                                       AddToolButton( 图标名字, 选项, 点击事件函数 )
                                          选项： 'position':    'left|right',
                                                'style':       'xxx'
                                       SetToolButton( 图标名字, 选项 ) 修改按钮的选项
                                       SetTotalNum( 设置总记录数 )
                                       SetToolPageButton( name, func ) 自定义翻页按钮事件
                                          name: 'first'  第一页
                                                'last'   最后一页
                                                'previous'  上一页
                                                'next'   下一页
                                                'jump'   跳转到指定页
                                       Jump( 指定页数 ) 跳转到指定页
   */
   sacApp.directive( 'ngTable', function( $animate, $timeout, $compile, $filter, SdbFunction ){
      var brower = SdbFunction.getBrowserInfo() ;
      var setOptionsFun = function( src, defaultVal ){
         if( !src )
            return defaultVal ;
         return src ;
      }
      var getNextKey = function( obj, key ){
         var nextKey = null ;
         var isFind = false ;
         $.each( obj, function( cKey, val ){
            if( isFind == true && typeof( val ) == 'string' )
            {
               nextKey = cKey ;
               return false ;
            }
            if( key == cKey )
            {
               isFind = true ;
            }
         } ) ;
         return nextKey ;
      }
      var dire = {
         restrict: 'A',
         templateUrl: './app/template/Component/ngTable.html',
         replace: false,
         transclude: true,
         scope: true,
         controller: function( $scope, $element, $attrs, $transclude ){
            $scope.lastScope = [] ; //最后一次自己创建的scope
            $scope.lastTr    = [] ; //最后一次创建的表格tr
            $scope.tools = {
               'page': 0,           //总共多少页
               'text': '',          //工具栏右边的文字
               'isCustom': false,   //工具栏的文字是否自定义
               'height': 0,         //工具栏高度
               'custom': []         //自定义按钮， 格式 { 'position': 'left|right', 'icon': 'fa-xxx', 'onClick': [function] }
            } ;
            $scope.loadStatus = {
               'length': 0,      //数据总共多少
               'page': 1,        //当前在第几页, 必须 > 0
               'tableWidth': 0,  //表格当前的宽度
               'width': {},      //控制表格宽度, 支持 '10px' 和 '10%' 两种写法, 也可以混合写
               'onMove': {             //调整表格宽度
                  'isMove': false,     //鼠标是不是按下移动的状态
                  'mouseX': 0,         //鼠标按下记录的坐标
                  'prevWidth': null,   //上一个td的宽度
                  'nextWidth': null,   //下一个td的宽度
                  'key': '',           //上一个td的key
                  'nextKey': ''        //下一个td的key
               },
               'onSort': {             //表格排序
                  'status': {},        //列的状态 0:默认 1:正序 -1:反序
                  'iconEles': {},      //列的图标dom元素
                  'last': null         //记录最后一次排序的信息
               },
               'onFilter': {
                  'dataBackup': [],    //数据备份
                  'status': false,     //显示的内容是否过滤后的， true: 过滤的内容；false: 所有内容
                  'isInit': true,      //是否需要初始化过滤表达式
                  'expre': {},         //过滤的表达式
                  'condition': {}      //过滤的条件
               },
               'showNum': 0            //当前显示多少记录
            } ;
            $scope.table = {
               'title': {},
               'body': [],
               'options': {
                  'width': {},      //控制表格宽度, 支持 '10px' 和 '10%' 两种写法, 也可以混合写(这里是作为缓存使用)
                  'tools': true,    //是否开启工具栏, 如果false, 就不能开启换页功能
                  'max': 100,       //一页最大显示多少行
                  'trim': true,     //是否允许调整表格宽度
                  'mode': 'normal', //模式， normal: 一次性加载全部数据，由内置方法控制表格换页； dynamic: 由外部提供方法控制换页，每一页都是通过外部获取。
                  'sort': [],       //是否开启排序，数组仅支持bool类型的元素，排序数组和标题数组一一对应
                  'autoSort': '',   //自动排序，当第一次数据写入到表格、表格数据改变时，将会自动排序
                  'filter': {},     //是否开启过滤功能
                  'default': {},    //如果开启过滤，是否要设置默认值，如果不填，默认是''
                  'isRenderHide': true,   //是否渲染隐藏(ng-if和ng-show)的元素
                  'text': {
                     'default': $scope.autoLanguage( '显示 ? 条记录，一共 ? 条' ),
                     'filterDefault': $scope.autoLanguage( '显示 ? 条记录，符合条件的一共 ? 条' )
                  }
               }
            } ;
            $scope.customEvent = {
               'first': null,
               'last': null,
               'previous': null,
               'next': null,
               'jump': null
            } ;
         },
         compile: function( element, attributes, transclude ){
            return {
               pre: function preLink( scope, element, attributes ){},
               post: function postLink( scope, element, attributes ){

                  //解析表达式
                  var expression = attributes.tableContent ;
                  var match = expression.match(/^\s*([\s\S]+?)\s+in\s+([\s\S]+?)\s*$/) ;
                  if( !match )
                  {
                     throw "Expected expression in form of '_item_ in _collection_: " + expression ;
                  }
                  var item = match[1] ;
                  var rhs  = match[2] ;

                  var tableEle = $( '> .ng-table', element ) ;

                  var boxEle   = angular.element( $( '.ng-table-box', tableEle ) ) ;

                  var headerBox = angular.element( $( '> .ng-table-header', boxEle ) ) ;

                  var titleEle = angular.element( $( '.ng-table-titles', headerBox ) ) ;

                  var filterEle = angular.element( $( '.ng-table-filter', headerBox ) ) ;

                  var headerTable  = angular.element( $( '> table', headerBox ) ) ;

                  var bodyBox = angular.element( $( '> .ng-table-body', boxEle ) ) ;

                  var bodyTable  = angular.element( $( '> table', bodyBox ) ) ;

                  var bodyEle  = angular.element( $( '> tbody', bodyTable ) ) ;

                  var toolEle = angular.element( $( '.ng-table-tools', tableEle ) ) ;

                  //回收资源
                  scope.$on( '$destroy', function(){
                     //主scope释放，子的scope也要释放
                     $.each( scope.lastScope, function( index, rowScope ){
                        rowScope.$destroy();
                        rowScope = null ;
                     } ) ;
                     //移除dom
                     $.each( scope.lastTr, function( index, trEle ){
                        $animate.leave( trEle ) ;
                        trEle = null ;
                     } ) ;
                     tableEle = null ;
                     boxEle = null ;
                     headerBox = null ;
                     titleEle = null ;
                     filterEle = null ;
                     headerTable = null ;
                     bodyBox = null ;
                     bodyTable = null ;
                     bodyEle = null ;
                     toolEle = null ;
                  } ) ;

                  //设置过滤条件
                  var setFilter = function( key, value ){
                     if( typeof( scope.loadStatus['onFilter']['expre'][key] ) != 'undefined' )
                     {
                        scope.loadStatus['onFilter']['expre'][key] = value ;
                        scope.find() ;
                     }
                  }

                  //设置工具栏内容
                  var setToolText = function( text ){
                     if( text === null )
                     {
                        scope.tools['isCustom'] = false ;
                        return ;
                     }
                     var type = typeof( text ) ;
                     scope.tools['isCustom'] = true ;
                     if( type == 'string' )
                        scope.tools['text'] = text ;
                     else if( type == 'function' )
                        scope.tools['text'] = text( scope.loadStatus['showNum'], scope.loadStatus['length'], scope.tools['text'] ) ;
                  }

                  //排序函数
                  var sortFun = function( index, key, isKeep ){
                     scope.loadStatus['onSort']['last'] = { 'index': index, 'key': key } ;
                     //还原所有图标样式和状态
                     $.each( scope.loadStatus['onSort']['iconEles'], function( iconKey, icon ){
                        if( icon != null )
                           $( icon ).removeClass() ;
                        if( iconKey != key )
                        {
                           scope.loadStatus['onSort']['status'][iconKey] = 0 ;
                           $( icon ).addClass( 'fa fa-sort' ) ;
                        }
                     } ) ;
                     //排序
                     var orderType = scope.loadStatus['onSort']['status'][key] <= 0 ; //true: 正序; false: 反序
                     if( isKeep == true )
                        orderType = !orderType ;
                     scope.loadStatus['onSort']['iconEles'][key].addClass( orderType ? 'fa fa-sort-asc' : 'fa fa-sort-desc' ) ;
                     if( isKeep == false )
                        scope.loadStatus['onSort']['status'][key] = orderType ? 1 : -1 ;
                     if( scope.table['body'].length > 0 )
                     {
                        if( typeof( key ) !== 'undefined' )
                        {
                           var tmp = scope.table['body'] ;
                           scope.table['body'] = $filter( 'orderObjectBy' )( tmp, key, !orderType ) ;
                           tmp = null ;
                        }
                     }
                     createTableContents( scope.loadStatus['page'], false ) ;
                  }

                  //渲染排序
                  var createTableSort = function( sortList, index, key, tdEle, divEle ){

                     if( sortList[key] ) //该列开启排序
                     {
                        var fa = angular.element( '<i></i>' ).addClass( 'fa fa-sort' ) ;

                        scope.loadStatus['onSort']['iconEles'][key] = fa ;

                        divEle.append( fa ) ;
                        divEle.append( '&nbsp;' ) ;

                        tdEle.css( {
                              'cursor': 'pointer',
                              '-moz-user-select': 'none',
                              '-webkit-user-select': 'none',
                              '-ms-user-select': 'none',
                              '-khtml-user-select': 'none',
                              'user-select': 'none'
                           } )
                           .on( 'click', function(){
                              sortFun( index, key, false ) ;
                              scope.$digest() ;
                           } ) ;
                     }
                     else
                     {
                        scope.loadStatus['onSort']['iconEles'][key] = null ;
                     }
                  }

                  //渲染标题
                  var createTableTitle = function( widthList, sortList, title, index, key, isLast )
                  {
                     var td = angular.element( '<td></td>' ).attr( 'table-key', key ) ; ;
                     if( typeof( widthList[key] ) != 'undefined' ) //如果有配置就设置宽度
                     {
                        td.css( { 'width': widthList[key] } ) ;
                     }

                     var div = angular.element( '<div></div>' ).addClass( 'Ellipsis' ) ;

                     var span = angular.element( '<span></span>' ).text( title ) ;

                     //创建排序
                     createTableSort( sortList, index, key, td, div ) ;

                     div.append( span ) ;

                     td.append( div ) ;

                     titleEle.append( td ) ;

                     //创建用来调整宽度的td
                     if( isLast == false && scope.table['options']['trim'] )
                     {
                        var td2 = $compile( '<td ng-mousedown="mouseDown($event)"></td>' )( scope ).addClass( 'trim' ).attr( 'table-key', key ) ;
                        titleEle.append( td2 ) ;
                     }
                     else //最后一个td 或 不允许调整宽度
                     {
                        var td2 = angular.element( '<td></td>' ).addClass( 'trimLast' ).attr( 'table-key', key ) ;
                        titleEle.append( td2 ) ;
                     }
                  }

                  //字符匹配
                  var strIndexOf = function( value, match ){
                     var type = typeof( value ) ;
                     if( type == 'undefined' || value === null )
                        value = '' ;
                     if( type != 'string' )
                        value = String( value ) ;
                     return ( value.toLowerCase().indexOf( match.toLowerCase() ) >= 0 ) ;
                  }

                  //数字匹配
                  var numberCompare = function( value, match ){
                     match = match.split( ',' ) ;
                     var operator = match[0] ;
                     var num      = isNaN( match[1] ) || match[1] == '' ? match[1] : Number( match[1] ) ;
                     if( num === '' ) //没填，所以都显示
                        return true ;
                     if( operator == 'gt' )
                     {
                        return value > num ;
                     }
                     else if( operator == 'gte' )
                     {
                        return value >= num ;
                     }
                     else if( operator == 'eq' )
                     {
                        return value === num ;
                     }
                     else if( operator == 'neq' )
                     {
                        return value !== num ;
                     }
                     else if( operator == 'lt' )
                     {
                        return value < num ;
                     }
                     else if( operator == 'lte' )
                     {
                        return value <= num ;
                     }
                     return false ;
                  }

                  //正则匹配
                  var strRegex = function( value, match ){
                     var type = typeof( value ) ;
                     if( type == 'undefined' || value === null )
                        value = '' ;
                     if( type != 'string' )
                        value = String( value ) ;
                     var patt = new RegExp( match )
                     return patt.test( value ) ;
                  }

                  //select的匹配
                  var selectMatch = function( value, match )
                  {
                     if( typeof( match ) == 'function' )
                     {
                        return match( value ) ;
                     }
                     else
                     {
                        return ( value == match ) ;
                     }
                  }

                  //渲染过滤
                  var createTableFilter = function( filterList, index, key, isLast ){
                     var td = angular.element( '<td></td>' ) ;
                     var type = typeof( filterList[key] ) ;
                     if( type == 'string' )
                     {
                        var input = null ;
                        if( filterList[key] == 'indexof' )
                        {
                           //用indexOf做匹配
                           input = $compile( '<input class="form-control" ng-model="loadStatus.onFilter.expre[\'' + key + '\']" ng-change="find()">' )( scope ) ;
                           scope.loadStatus['onFilter']['condition'][key] = strIndexOf ;
                        }
                        else if( filterList[key] == 'number' )
                        {
                           //数字比大小
                           var input  = $( '<div></div>' ).css( { 'display': 'table', 'width': '100%' } ) ;
                           var div1   = $( '<div></div>' ).css( { 'display': 'table-cell', 'width': '35%', 'float':'left' } ) ;
                           var div2   = $( '<div></div>' ).css( { 'display': 'table-cell', 'width': '55%', 'float':'left' } ) ;
                           var vertical = ( brower[0] == 'firefox' ? '' : 'vertical-align:top;' ) ;
                           var select = $compile( '<select class="form-control" style="border-right:0;' + vertical + '" ng-model="loadStatus.onFilter.expre[\'' + key + '\'][0]" ng-change="find()"><option value="gt">&gt;</option><option value="gte">&gt;=</option><option value="eq">=</option><option value="neq">!=</option><option value="lt">&lt;</option><option value="lte">&lt;=</option></select>')( scope ) ;
                           div1.append( select ) ;
                           var text = $compile( '<input class="form-control" style="' + vertical + '" ng-model="loadStatus.onFilter.expre[\'' + key + '\'][1]" ng-change="find()">' )( scope ) ;
                           div2.append( text ) ;
                           scope.loadStatus['onFilter']['condition'][key] = numberCompare ;
                           input.append( div1 ) ;
                           input.append( div2 ) ;
                        }
                        else
                        {
                           //用正则做匹配
                           input = $compile( '<input class="form-control" ng-model="loadStatus.onFilter.expre[\'' + key + '\']" ng-change="find()">' )( scope ) ;
                           scope.loadStatus['onFilter']['condition'][key] = strRegex ;
                        }
                        td.append( input ) ;
                     }
                     else if( type == 'function' )
                     {
                        //函数匹配
                        var input = $compile( '<input class="form-control" ng-model="loadStatus.onFilter.expre[\'' + key + '\']" ng-change="find()">' )( scope ) ;
                        td.append( input ) ;
                        scope.loadStatus['onFilter']['condition'][key] = filterList[key] ;
                     }
                     else if( type == 'object' && isArray( filterList[key] ) )
                     {
                        //下拉菜单
                        var select = $compile( '<select class="form-control" ng-model="loadStatus.onFilter.expre[\'' + key + '\']" ng-change="find()"  ng-options="item.value as item.key for item in table.options.filter[\'' + key + '\']"></select>')( scope ) ;
                        td.append( select ) ;
                        scope.loadStatus['onFilter']['condition'][key] = selectMatch ;
                     }
                     else
                     {
                        scope.loadStatus['onFilter']['condition'][key] = null ;
                     }
                     filterEle.append( td ) ;
                     //创建用来调整宽度的td
                     if( isLast == false && scope.table['options']['trim'] )
                     {
                        var td2 = $compile( '<td ng-mousedown="mouseDown($event)"></td>' )( scope ).addClass( 'trim' ).attr( 'table-key', key ) ;
                        filterEle.append( td2 ) ;
                     }
                     else //最后一个td 或 不允许调整宽度
                     {
                        var td2 = angular.element( '<td></td>' ).addClass( 'trimLast' ).attr( 'table-key', key ) ;
                        filterEle.append( td2 ) ;
                     }
                  }

                  //渲染表格头
                  var createTableHeaders = function(){

                     //移除旧的
                     $( '> td', titleEle ).remove() ;
                     $( '> td', filterEle ).remove() ;

                     //统计标题数量
                     var titleLength = 0 ;
                     $.each( scope.table['title'], function( key, value ){
                        if( typeof( value ) == 'string' )
                           ++titleLength ;
                     } ) ;

                     //初始化图标
                     scope.loadStatus['onSort']['iconEles'] = {} ;

                     //表格宽度列表
                     var widthList = scope.loadStatus['width'] ;

                     //表格排序列表
                     var sortList = scope.table['options']['sort'] ;

                     //表格过滤列表
                     var filterList = scope.table['options']['filter'] ;

                     //创建列
                     var index = 0 ;
                     $.each( scope.table['title'], function( key, title ){
                        if( title === false )
                        {
                           //跳过该列
                           return true ;
                        }
                        var isLast = index >= titleLength - 1 ;

                        if( typeof( scope.table['options']['default'][key] ) == 'undefined' )   //没有默认值
                        {
                           if( scope.loadStatus['onFilter']['isInit'] == true ) //只有在初始化才需要初始过滤条件
                           {
                              if( filterList[key] == 'number' )
                                 scope.loadStatus['onFilter']['expre'][key] = [ 'eq', '' ] ;
                              else
                                 scope.loadStatus['onFilter']['expre'][key] = '' ;
                           }
                           else
                           {
                              if( typeof( scope.loadStatus['onFilter']['expre'][key] ) == 'undefined' )
                              {
                                 if( filterList[key] == 'number' )
                                    scope.loadStatus['onFilter']['expre'][key] = [ 'eq', '' ] ;
                                 else
                                    scope.loadStatus['onFilter']['expre'][key] = '' ;
                              }
                           }
                        }
                        else
                           scope.loadStatus['onFilter']['expre'][key] = scope.table['options']['default'][key] ;

                        //创建标题
                        createTableTitle( widthList, sortList, title, index, key, isLast ) ;

                        if( typeof( filterList ) == 'object' && filterList !== null && getObjectSize( filterList ) > 0 )
                           createTableFilter( filterList, index, key, isLast ) ; //创建过滤

                        ++index ;

                     } ) ;

                     scope.table['options']['default'] = {} ;//过滤默认值，只有初始化有效

                     scope.loadStatus['onFilter']['isInit'] = false ; //初始化完成

                  }

                  //渲染表格内容
                  var createTableContents = function( page, isRecoveryWidth ){

                     if( scope.table.options['mode'] == 'dynamic' )
                     {
                        page = 1 ;
                     }

                     //释放旧的scope
                     $.each( scope.lastScope, function( index, rowScope ){
                        rowScope.$destroy();
                        scope.lastScope[index] = null ;
                     } ) ;

                     //移除旧的dom
                     $.each( scope.lastTr, function( index, trEle ){
                        $animate.leave( trEle ) ;
                        scope.lastTr[index] = null ;
                     } ) ;

                     scope.lastScope = [] ;
                     scope.lastTr = [] ;
                     
                     //统计标题数量
                     var useTitleList = [] ;
                     var titleLength = 0 ;
                     $.each( scope.table['title'], function( key, value ){
                        if( typeof( value ) == 'string' )
                        {
                           ++titleLength ;
                           useTitleList.push( key ) ;
                        }
                     } ) ;

                     //计算显示的记录范围
                     var start, end ;
                     if( scope.table.options['mode'] == 'normal' )
                     {
                        start = ( page - 1 ) * scope.table['options']['max'] ;
                        end   = start + scope.table['options']['max'] ;
                     }
                     else if( scope.table.options['mode'] == 'dynamic' )
                     {
                        start = ( scope.loadStatus['page'] - 1 ) * scope.table['options']['max'] ;
                        end   = start + scope.table['options']['max'] ;
                     }
                     end = end > scope.loadStatus['length'] ? scope.loadStatus['length'] : end ;

                     var widthList = scope.loadStatus['width'] ;
                     
                     //计算总页数
                     scope.tools['page'] = numberCarry( scope.loadStatus['length'] / scope.table['options']['max'] ) ;

                     scope.loadStatus['showNum'] = end - start ;
                     if( scope.tools['isCustom'] == false )
                     {
                        scope.tools['text'] = sprintf( scope.loadStatus['onFilter']['status'] ? scope.table['options']['text']['filterDefault'] : scope.table['options']['text']['default'],
                                                       scope.loadStatus['showNum'],
                                                       scope.loadStatus['length'] ) ;
                     }

                     if( scope.table.options['mode'] == 'dynamic' )
                     {
                        start = 0 ;
                        end   = scope.loadStatus['showNum'] ;
                     }

                     //在网页上使用table-key，并且不是$auto的列表
                     var keyList = null ;
                     for( var index1 = start; index1 < end; ++index1 )
                     {
                        var tr = angular.element( '<tr></tr>' ) ;
                        scope.lastTr.push( tr ) ;
                        var childScope = scope.$new();
                        scope.lastScope.push( childScope ) ;
                        childScope['$index'] = index1 ;
                        childScope[item] = scope.table['body'][index1] ;
                        transclude( childScope, function( clone ){
                           var index2 = 0 ;
                           if( keyList === null )
                           {
                              keyList= [] ;
                              //获取所有table-key不是$auto的索引
                              $.each( clone, function( index3, col ){
                                 if( col.nodeType == 1 )
                                 {
                                    var tableKey = $( col ).attr( 'table-key' ) ;
                                    if( typeof( tableKey ) == 'string' && tableKey !== '$auto' )
                                    {
                                       var isPush = true ;
                                       if( scope['table']['options']['isRenderHide'] == false )
                                       {
                                          var ngif = $( col ).attr( 'ng-if' ) ;
                                          var ngshow = $( col ).attr( 'ng-show' ) ;
                                          if( ngif )
                                          {
                                             isPush = scope.$eval( ngif ) ;
                                          }
                                          if( isPush && ngshow )
                                          {
                                             isPush = scope.$eval( ngshow ) ;
                                          }
                                       }
                                       if( isPush )
                                       {
                                          keyList.push( tableKey ) ;
                                       }
                                    }
                                 }
                              } ) ;
                           }
                           $.each( clone, function( index3, col ){
                              if( col.nodeType == 1 )
                              {
                                 var tableKey = $( col ).attr( 'table-key' ) ;
                                 {
                                    var ngif = $( col ).attr( 'ng-if' ) ;
                                    var ngshow = $( col ).attr( 'ng-show' ) ;
                                    if( ngif )
                                    {
                                       if( scope.$eval( ngif ) == false )
                                       {
                                          return true ;
                                       }
                                    }
                                    if( ngshow )
                                    {
                                       if( scope.$eval( ngshow ) == false )
                                       {
                                          return true ;
                                       }
                                    }
                                 }
                                 if( typeof( tableKey ) != 'string' || scope.table['title'][tableKey] === false ||
                                     ( tableKey !== '$auto' && scope.table['title'][tableKey] === undefined ) || index2 >= titleLength )
                                 {
                                    return true ;
                                 }
                                 var hasAuto = false ;
                                 var autoHtml = '' ;
                                 if( tableKey == '$auto' )
                                 {
                                    autoHtml = $( col ).prop( 'outerHTML' ) ;
                                 }
                                 while( true )
                                 {
                                    //如果table-key属性是$auto，那么说明开发者也不知道字段名字，那么将通过标题找到对应字段
                                    if( tableKey == '$auto' || hasAuto == true )
                                    if( scope['table']['options']['isRenderHide'] == false )
                                    {
                                       var newAutoHtml = autoHtml ;
                                       hasAuto = true ;
                                       tableKey = useTitleList[index2] ;
                                       if( keyList.indexOf( tableKey ) >= 0 ) //发现这个字段在后面的html有，那就不需要使用$auto了
                                       {
                                          break ;
                                       }
                                       newAutoHtml = newAutoHtml.replace( /\$autoValue/g, item + '.' + tableKey ) ;
                                       newAutoHtml = newAutoHtml.replace( /\$auto/g, tableKey ) ;
                                       newAutoHtml = newAutoHtml.replace( /table-if/g, 'ng-if' ) ;
                                       col = $compile( newAutoHtml )( childScope ) ;
                                    }
                                    var td = angular.element( '<td></td>' ).attr( 'table-key', tableKey ) ; ;
                                    if( typeof( widthList[tableKey] ) != 'undefined' )
                                    {
                                       td.css( { 'width': widthList[tableKey] } ) ;
                                    }
                                    $animate.enter( col, td, null ) ;
                                    tr.append( td ) ;

                                    //创建用来调整宽度的td
                                    if( scope.table['options']['trim'] && index2 < titleLength - 1 )
                                    {
                                       var td2 = $compile( '<td ng-mousedown="mouseDown($event)"></td>' )( scope ).addClass( 'trim' ).attr( 'table-key', tableKey ) ;
                                       tr.append( td2 ) ;
                                    }
                                    else
                                    {
                                       var td2 = angular.element( '<td></td>' ).addClass( 'trimLast' ).attr( 'table-key', tableKey ) ;
                                       tr.append( td2 ) ;
                                    }
                                    ++index2 ;
                                    if( hasAuto == false || index2 >= titleLength )
                                    {
                                       break ;
                                    }
                                 }
                              }
                           } ) ;
                        } ) ;
                        childScope = null ;
                        $animate.enter( tr, bodyEle, null ) ;
                     }
                     $timeout( function(){
                        resizeFun( isRecoveryWidth ) ;
                     } ) ;
                  }

                  //调整表格头的列宽
                  var resizeTableHeaders = function(){
                     //列宽度
                     var widthList = scope.loadStatus['width'] ;

                     //修改标题宽度
                     $( '> td', titleEle ).each( function( index, ele ){
                        var td = $( ele ) ;
                        if( td.hasClass( 'trim' ) == false &&
                            td.hasClass( 'trimLast' ) == false ) // .trim是用来控制宽度的，所以不能修改
                        {
                           var tableKey = td.attr( 'table-key' ) ;
                           if( typeof( tableKey ) == 'string' && typeof( widthList[tableKey] ) == 'string' )
                           {
                              td.css( { 'width': widthList[tableKey] } ) ;
                           }
                           else
                           {
                              td.css( { 'width': 'auto' } ) ;
                           }
                        }
                        td = null ;
                     } ) ;
                  }

                  //调整表格内容的列宽
                  var resizeTableContents = function(){
                     //列宽度
                     var widthList = scope.loadStatus['width'] ;
                     //修改内容宽度
                     var firstTr = $( '> tr:first', bodyEle ) ;
                     $( '> td', firstTr ).each( function( index, ele ){
                        var td = $( ele ) ;
                        if( td.hasClass( 'trim' ) == false &&
                            td.hasClass( 'trimLast' ) == false ) // .trim是用来控制宽度的，所以不能修改
                        {
                           var tableKey = td.attr( 'table-key' ) ;
                           if( typeof( tableKey ) == 'string' && typeof( widthList[tableKey] ) == 'string' )
                           {
                              td.css( { 'width': widthList[tableKey] } ) ;
                           }
                           else
                           {
                              td.css( { 'width': 'auto' } ) ;
                           }
                        }
                        td = null ;
                     } ) ;
                     firstTr = null ;
                     widthList = null ;
                  }

                  //调整表格内容跟表个头的间距
                  var resetBodyTop = function(){
                     $( bodyBox ).css( { 'padding-top': ( $( headerBox ).height() + 1 ) + 'px' } ) ;
                  }

                  //重绘的函数
                  var resizeFun = function( isRecoveryWidth ){
                     //恢复原来的比例
                     if( isRecoveryWidth !== false )
                     {
                        var widthList = scope.table['options']['width'] ;
                        scope.loadStatus['width'] = $.extend( true, {}, widthList ) ;
                        widthList = null ;
                     }
                     resizeTableHeaders() ;
                     resizeTableContents() ;
                     $timeout( function(){
                        $timeout( function(){
                           resetBodyTop() ;
                        }, 1 ) ;
                        //表格头和表格内容宽度对齐
                        var width = $( bodyTable ).width() ;
                        $( headerTable ).width( width ) ;

                        //记录当前表格宽度
                        scope.loadStatus['tableWidth'] = width ;

                        //预留表格工具栏的高度
                        if( scope.table['options']['tools'] === false )
                        {
                           scope.tools['height'] = -1 ;
                        }
                        else
                        {
                           scope.tools['height'] = -1 * $( toolEle ).outerHeight() ;
                        }
                     }, 0, false ) ;
                  }

                  resizeFun() ; //为了兼容ie7

                  //重绘函数绑定到重绘队列
                  SdbFunction.defer( scope, resizeFun ) ;

                  //初始化表格内容跟表格头的间距
                  var initBodyTop = function(){
                     $timeout( function(){
                        resetBodyTop() ;
                     }, 0, false ) ;
                  } ;

                  initBodyTop() ;

                  //监控配置
                  scope.$watch( attributes.ngTable, function ngTable( options ){
                     if( typeof( options ) == 'object' )
                     {
                        //复制配置
                        $.each( scope.table['options'], function( key ){
                           if( typeof( options[key] ) != 'undefined' )
                           {
                              scope.table['options'][key] = options[key] ;
                           }
                        } ) ;
                        //复制列宽配置
                        scope.loadStatus['width'] = $.extend( true, {}, scope.table['options']['width'] ) ;
                        //初始化排序状态
                        scope.loadStatus['onSort']['status'] = [] ;
                        $.each( scope.table['options']['sort'], function(){
                           scope.loadStatus['onSort']['status'].push( '0' ) ;
                        } ) ;
                        resizeTableHeaders() ;
                        resizeTableContents() ;
                     }
                  }, true ) ;

                  //监控标题
                  scope.$watchCollection( attributes.tableTitle, function( titles ){
                     scope.table['title'] = setOptionsFun( titles, {} ) ;
                     createTableHeaders() ;
                  } ) ;

                  //监控内容
                  scope.$watchCollection( rhs, function( contents ){
                     scope.table['body'] = null ;
                     scope.table['body'] = setOptionsFun( contents, [] ) ;
                     if( scope.table.options['mode'] == 'normal' )
                     {
                        scope.loadStatus['length'] = scope.table['body'].length ;
                     }
                     if( typeof( scope.table['options']['autoSort'] ) == 'object' )
                     {
                        scope.table['body'] = $filter( 'orderObjectBy' )( scope.table['body'], scope.table['options']['autoSort']['key'],
                                                                                               !scope.table['options']['autoSort']['asc'] ) ;
                     }
                     createTableContents( 1, false ) ;
                     if( scope.loadStatus['onFilter']['status'] ) //如果已经做了过滤，那么要把数据复制到备份中，不然会丢数据
                     {
                        scope.loadStatus['onFilter']['dataBackup'] = null ;
                        scope.loadStatus['onFilter']['dataBackup'] = $.extend( [], scope.table['body'] ) ;
                     }
                     scope.find() ;
                     /*
                     if( scope.loadStatus['onSort']['last'] !== null ) //如果有排序，那么要做一次重新排序
                        sortFun( scope.loadStatus['onSort']['last']['index'], scope.loadStatus['onSort']['last']['key'], true ) ;
                        */
                  } ) ;

                  //获取指定页数据
                  var getPageData = function( pageNum ){
                     var dataList, result = [] ;
                     //判断过滤状态
                     if( scope.loadStatus['onFilter']['status'] ) //已经做过过滤
                        dataList = scope.loadStatus['onFilter']['dataBackup'] ; //取原数据
                     else //没有过过滤
                        dataList = scope.table['body'] ; //取当前数据
                     var start = ( pageNum - 1 ) * scope.table['options']['max'] ;
                     var end   = start + scope.table['options']['max'] ;
                     end = end > scope.loadStatus['length'] ? scope.loadStatus['length'] : end ;
                     for( var index1 = start; index1 < end; ++index1 )
                     {
                        result.push( dataList[index1] ) ;
                     }
                     return result ;
                  }

                  //获取所有数据
                  var getAllData = function(){
                     var dataList ;
                     //判断过滤状态
                     if( scope.loadStatus['onFilter']['status'] ) //已经做过过滤
                        dataList = scope.loadStatus['onFilter']['dataBackup'] ; //取原数据
                     else //没有过过滤
                        dataList = scope.table['body'] ; //取当前数据
                     return dataList ;
                  }

                  //获取过滤后指定页的数据
                  var getFilterPageData = function( pageNum ){
                     var dataList = scope.table['body'], result = [] ;
                     var start = ( pageNum - 1 ) * scope.table['options']['max'] ;
                     var end   = start + scope.table['options']['max'] ;
                     end = end > scope.loadStatus['length'] ? scope.loadStatus['length'] : end ;
                     for( var index1 = start; index1 < end; ++index1 )
                     {
                        result.push( dataList[index1] ) ;
                     }
                     return result ;
                  }

                  //获取过滤后所有的数据
                  var getFilterAllData = function(){
                     return scope.table['body'] ;
                  }

                  //获取过滤状态
                  var getFilterStatus = function(){
                     return scope.loadStatus['onFilter']['status'] ;
                  }

                  //获取当前页
                  var getCurrentPageNum = function(){
                     return scope.loadStatus['page'] ;
                  }

                  //获取总页数
                  var getSumPageNum = function(){
                     return scope.tools['page'] ;
                  }

                  //重绘当前页
                  var showCurrentPage = function(){
                     createTableContents( scope.loadStatus['page'] ) ;
                     try
                     {
                        scope.$digest() ;
                     }
                     catch(e){}
                  }

                  //接口专用排序，跟表格的标题排序不同，不会存在状态中
                  var sortData = function( key, asc ){
                     scope.table['body'] = $filter( 'orderObjectBy' )( scope.table['body'], key, !asc ) ;
                     createTableContents( scope.loadStatus['page'], false ) ;
                     scope.$digest() ;
                  }

                  /*
                     添加工具栏自定义按钮
                     option   {
                                 'position':    'left|right',
                                 'style':       'xxx'
                              }
                  */
                  var addToolBtn = function( icon, option, func ){
                     if( typeof( option['position'] ) == 'undefined' )
                     {
                        option['position'] = 'left' ;
                     }
                     option['icon'] = icon ;
                     option['onClick'] = func ;
                     scope.tools.custom.push( option ) ;
                  }

                  var resetToolBtn = function( icon, option ){
                     $.each( scope.tools.custom, function( index, btn ){
                        if( btn['icon'] == icon )
                        {
                           $.each( option, function( key, value ){
                              btn[key] = value ;
                           } ) ;
                           return false ;
                        }
                     } ) ;
                  }

                  //自定义工具栏翻页事件
                  var setToolPageBtn = function( name, func )
                  {
                     if( typeof( scope.customEvent[name] ) == 'undefined' )
                     {
                        alert( 'Invalid event name.') ;
                        return ;
                     }
                     scope.customEvent[name] = func ;
                  }

                  //设置总记录数
                  var setTotalRecords = function( totalNum )
                  {
                     if( scope.table.options['mode'] !== 'dynamic' )
                     {
                        alert( 'table.options.mode is not dynamic.' ) ;
                        return ;
                     }

                     scope.loadStatus['length'] = totalNum ;
                     scope.tools['page'] = numberCarry( scope.loadStatus['length'] / scope.table['options']['max'] ) ;
                     showCurrentPage() ;
                  }

                  //跳转到指定页
                  var jumpPage = function( pageNum ){
                     if( pageNum > scope.tools['page'] )
                     {
                        scope.loadStatus['page'] = scope.tools['page'] ;
                     }

                     if( pageNum <= 0 )
                     {
                        scope.loadStatus['page'] = 1 ;
                     }
                     else
                     {
                        scope.loadStatus['page'] = pageNum ;
                     }
                     if( typeof( scope.customEvent['jump'] ) === 'function' )
                     {
                        scope.customEvent['jump']( scope.loadStatus['page'] ) ;
                     }
                     else
                     {
                        createTableContents( scope.loadStatus['page'], false ) ;
                     }
                  }

                  //如果有table-callback，那么把回调的函数传给他
                  scope.$watch( attributes.tableCallback, function ( callbackGetter ){
                     if( typeof( callbackGetter ) == 'object' )
                     {
                        callbackGetter['GetPageData']       = getPageData ;
                        callbackGetter['GetAllData']        = getAllData ;
                        callbackGetter['GetFilterPageData'] = getFilterPageData ;
                        callbackGetter['GetFilterAllData']  = getFilterAllData ;
                        callbackGetter['GetFilterStatus']   = getFilterStatus ;
                        callbackGetter['GetCurrentPageNum'] = getCurrentPageNum ;
                        callbackGetter['GetSumPageNum']     = getSumPageNum ;
                        callbackGetter['SetFilter']         = setFilter ;
                        callbackGetter['SetToolText']       = setToolText ;
                        callbackGetter['ShowCurrentPage']   = showCurrentPage ;
                        callbackGetter['ResizeTableHeader'] = resizeTableHeaders ;
                        callbackGetter['ResetBodyTop']      = resetBodyTop ;
                        callbackGetter['ResetBodyTopAfterRender'] = initBodyTop ;
                        callbackGetter['SortData']          = sortData ;
                        callbackGetter['AddToolButton']     = addToolBtn ;
                        callbackGetter['SetToolButton']     = resetToolBtn ;
                        callbackGetter['SetTotalNum']       = setTotalRecords ;
                        callbackGetter['SetToolPageButton'] = setToolPageBtn ;
                        callbackGetter['Jump']              = jumpPage ;
                     }
                  } ) ;

                  //第一页
                  scope.first = function(){
                     if( typeof( scope.customEvent['first'] ) === 'function' )
                     {
                        if( scope.loadStatus['page'] > 1 )
                        {
                           scope.loadStatus['page'] = 1 ;
                           scope.customEvent['first']() ;
                        }
                     }
                     else
                     {
                        if( scope.table['body'].length == 0 )
                           return ;
                        scope.loadStatus['page'] = 1 ;
                        createTableContents( scope.loadStatus['page'], false ) ;
                     }
                  }

                  //上一页
                  scope.previous = function(){
                     if( typeof( scope.customEvent['previous'] ) === 'function' )
                     {
                        --scope.loadStatus['page'] ;
                        if( scope.loadStatus['page'] <= 0 )
                        {
                           scope.loadStatus['page'] = 1 ;
                        }
                        else
                        {
                           scope.customEvent['previous']() ;
                        }
                     }
                     else
                     {
                        if( scope.table['body'].length == 0 )
                           return ;
                        --scope.loadStatus['page'] ;
                        if( scope.loadStatus['page'] <= 0 )
                           scope.loadStatus['page'] = 1 ;
                        createTableContents( scope.loadStatus['page'], false ) ;
                     }
                  }

                  //下一页
                  scope.next = function(){
                     if( typeof( scope.customEvent['next'] ) === 'function' )
                     {
                        ++scope.loadStatus['page'] ;
                        if( scope.loadStatus['page'] > scope.tools['page'] )
                        {
                           scope.loadStatus['page'] = scope.tools['page'] ;
                        }
                        else
                        {
                           scope.customEvent['next']() ;
                        }
                     }
                     else
                     {
                        if( scope.table['body'].length == 0 )
                           return ;
                        ++scope.loadStatus['page'] ;
                        if( scope.loadStatus['page'] > scope.tools['page'] )
                           scope.loadStatus['page'] = scope.tools['page'] ;
                        createTableContents( scope.loadStatus['page'], false ) ;
                     }
                  }

                  //最后一页
                  scope.last = function(){
                     if( typeof( scope.customEvent['last'] ) === 'function' )
                     {
                        if( scope.loadStatus['page'] < scope.tools['page'] )
                        {
                           scope.loadStatus['page'] = scope.tools['page'] ;
                           scope.customEvent['last']() ;
                        }
                     }
                     else
                     {
                        if( scope.table['body'].length == 0 )
                           return ;
                        scope.loadStatus['page'] = scope.tools['page'] ;
                        createTableContents( scope.loadStatus['page'], false ) ;
                     }
                  }

                  //检查输入的页数格式
                  scope.check = function(){
                     if( typeof( scope.loadStatus['page'] ) == 'string' && scope.loadStatus['page'].length == 0 )
                     {
                     }
                     else if( isNaN( scope.loadStatus['page'] ) )
                     {
                        scope.loadStatus['page'] = parseInt( scope.loadStatus['page'] ) ;
                        if( scope.loadStatus['page'] <= 0 )
                           scope.loadStatus['page'] = 1 ;
                     }
                     else if( scope.loadStatus['page'] > scope.tools['page'] )
                     {
                        scope.loadStatus['page'] = scope.tools['page'] ;
                     }
                     else if( scope.loadStatus['page'] <= 0 )
                     {
                        scope.loadStatus['page'] = 1 ;
                     }
                  }

                  //跳转
                  scope.jump = function( event ){
                     if( event.keyCode == 13 )
                     {
                        if( typeof( scope.customEvent['jump'] ) === 'function' )
                        {
                           scope.customEvent['jump']( scope.loadStatus['page'] ) ;
                        }
                        else
                        {
                           createTableContents( scope.loadStatus['page'], false ) ;
                        }
                     }
                  }

                  //按住
                  scope.mouseDown = function( event ){
                     var trim = event.currentTarget ;
                     scope.loadStatus['onMove']['isMove'] = true ;
                     scope.loadStatus['onMove']['mouseX'] = event['pageX'] ;
                     scope.loadStatus['onMove']['prevWidth'] = $( trim ).prev().width() - 1 ;
                     scope.loadStatus['onMove']['nextWidth'] = $( trim ).next().width() - 1 ;
                     scope.loadStatus['onMove']['key'] = $( trim ).attr( 'table-key' ) ;
                     scope.loadStatus['onMove']['nextKey'] = getNextKey( scope.table['title'], scope.loadStatus['onMove']['key'] ) ;
                     $( tableEle ).css( { '-moz-user-select': 'none', '-webkit-user-select': 'none', '-ms-user-select': 'none', '-khtml-user-select': 'none', 'user-select': 'none' } ) ;
                  }

                  //移动
                  scope.move = function( event ){
                     if( scope.loadStatus['onMove']['isMove'] == true )
                     {
                        var minWidth = 30 ;
                        var width = scope.loadStatus['tableWidth'] ;
                        var offsetX = event['pageX'] - scope.loadStatus['onMove']['mouseX'] ;
                        var key = scope.loadStatus['onMove']['key'] ;
                        var nextKey = scope.loadStatus['onMove']['nextKey'] ;

                        offsetX = fixedNumber( offsetX, 0 ) ;
                        if( offsetX < 0 && scope.loadStatus['onMove']['prevWidth'] > minWidth && scope.loadStatus['onMove']['prevWidth'] + offsetX < minWidth )
                        {
                           offsetX = minWidth - scope.loadStatus['onMove']['prevWidth'] ;
                        }
                        else if( offsetX < 0 && scope.loadStatus['onMove']['prevWidth'] < minWidth )
                        {
                           offsetX = minWidth - scope.loadStatus['onMove']['prevWidth'] ;
                        }
                        else if( offsetX > 0 && scope.loadStatus['onMove']['nextWidth'] > minWidth && scope.loadStatus['onMove']['nextWidth'] - offsetX < minWidth )
                        {
                           offsetX = scope.loadStatus['onMove']['nextWidth'] - minWidth ;
                        }
                        else if( offsetX > 0 && scope.loadStatus['onMove']['nextWidth'] < minWidth )
                        {
                           offsetX = minWidth - scope.loadStatus['onMove']['nextWidth'] ;
                        }
                        scope.loadStatus['width'][key] = scope.loadStatus['onMove']['prevWidth'] + offsetX + 'px' ;
                        scope.loadStatus['width'][nextKey] = scope.loadStatus['onMove']['nextWidth'] - offsetX + 'px' ;
                        resizeTableHeaders() ;
                        resizeTableContents() ;
                     }
                  }

                  //弹起
                  scope.mouseUp = function(){
                     scope.loadStatus['onMove']['isMove'] = false ;
                     $( tableEle ).css( { '-moz-user-select': 'text', '-webkit-user-select': 'text', '-ms-user-select': 'text', '-khtml-user-select': 'text', 'user-select': 'text' } ) ;
                  }

                  //字符串匹配
                  scope.find = function(){

                     //匹配的表达式
                     var match = scope.loadStatus['onFilter']['expre'] ;
                     //匹配的条件
                     var condition = scope.loadStatus['onFilter']['condition'] ;
                     //检查过滤条件是否全部为空
                     var isEmpty = true ;
                     $.each( match, function( index, matchVal ){
                        var matchType = typeof( matchVal ) ;
                        var isArrType = false ;
                        if( matchType == 'object' )
                        {
                           isArrType = isArray( matchVal ) ;
                           if( isArrType == false )
                              matchVal = matchVal['value'] ;
                        }
                        else
                           matchVal = String( matchVal ) ;
                        if( isArrType )
                        {
                           if( isArray( matchVal ) == true && matchVal[1].length > 0 )
                           {
                              isEmpty = false ;
                              return false ;
                           }
                        }
                        else
                        {
                           if( matchVal !== null && ( typeof( matchVal ) != 'string' || matchVal.length > 0 ) )
                           {
                              isEmpty = false ;
                              return false ;
                           }
                        }
                     } ) ;

                     var dataList = null ;
                     //判断过滤状态
                     if( scope.loadStatus['onFilter']['status'] )
                     {
                        //已经做过过滤
                        if( isEmpty )
                        {
                           //因为没有过滤条件，所以没必要做数据复制
                           dataList = scope.loadStatus['onFilter']['dataBackup'] ;
                           scope.loadStatus['onFilter']['status'] = false ;
                        }
                        else
                        {
                           //有过滤条件，必须做数据复制
                           dataList = $.extend( [], scope.loadStatus['onFilter']['dataBackup'] ) ;
                        }
                     }
                     else
                     {
                        //没有做过过滤
                        if( isEmpty )
                        {
                           dataList = scope.table['body'] ;
                        }
                        else
                        {
                           scope.loadStatus['onFilter']['dataBackup'] = null ;
                           scope.loadStatus['onFilter']['dataBackup'] = scope.table['body'] ;
                           dataList = $.extend( [], scope.table['body'] ) ;
                           scope.loadStatus['onFilter']['status'] = true ;
                        }
                     }
                     scope.table['body'] = null ;
                     if( isEmpty )
                     {
                        //没用过滤条件，全部显示
                        scope.table['body'] = dataList ;
                        if( scope.table.options['mode'] == 'normal' )
                        {
                           scope.loadStatus['length'] = scope.table['body'].length ;
                        }
                     }
                     else
                     {
                        //有条件，开始过滤
                        scope.table['body'] = [] ;
                        $.each( dataList, function( index, row ){
                           var index2 = 0 ;
                           var isMatch = true ;
                           $.each( scope.table['title'], function( key, title ){
                              if( title == false ) //如果该列是关闭状态，那么就不过滤
                                 return true ;
                              var matchType = typeof( match[key] ) ;
                              var isMatchType = ( matchType == 'function' || matchType == 'boolean' ) ;
                              var matchStr = isMatchType ? match[key] : String( match[key] ) ;
                              if( condition[key] !== null && ( isMatchType || matchStr.length > 0 ) )
                              {
                                 var value = row ;
                                 var fields = key.split( '.' ) ;
                                 for( var y = 0; y < fields.length; ++y )
                                 {
                                    value = value[ fields[y] ] ;
                                 }
                                 isMatch = condition[key]( value, matchStr ) ;
                                 value = null ;
                                 if( isMatch == false )
                                    return false ;
                              }
                              ++index2 ;
                           } ) ;
                           if( isMatch == true )
                              scope.table['body'].push( row ) ;
                        } ) ;
                        if( scope.table.options['mode'] == 'normal' )
                        {
                           scope.loadStatus['length'] = scope.table['body'].length ;
                        }
                     }
                     dataList = null ;
                     createTableContents( 1, false ) ;
                     if( scope.loadStatus['onSort']['last'] !== null ) //如果有排序，那么要做一次重新排序
                        sortFun( scope.loadStatus['onSort']['last']['index'], scope.loadStatus['onSort']['last']['key'], true ) ;
                  }
               }
            } ;
         }
      } ;
      return dire ;
   } ) ;

}());

/*
模板
sacApp.directive( 'createGrid', function(){
   var dire = {

      // 模式 建议用A 支持 IE6、7
      // E - 元素名称： <my-directive></my-directive>
      // A - 属性名： <div my-directive=”exp”></div>
      // C - class名： <div class=”my-directive:exp;”></div>
      // M - 注释 ： <!-- directive: my-directive exp -->
      restrict: 'A',

      // scope重定义
      scope: {
         data: '=para'
      },

      // 模版 直接写html，也可以用 templateUrl 来异步加载文件
      template: '<p>Hello {{data}}</p>',
      
      // replace - 如果设置为true，那么模版将会替换当前元素，而不是作为子元素添加到当前元素中。（注：为true时，模版必须有一个根节点）
      replace: false,

      // 专用控制器
      controller: function( $scope, $element, $attrs, $transclude ){
         $scope.data = $scope.data + "22222 ";
      },

      // 编译
      compile: function( element, attributes ){
         //DOM变形
         //xxx代码xxx
         return {
            pre: function preLink( scope, element, attributes ){
               //scope的一些初始化或者运算
            },
            post: function postLink( scope, element, attributes ){
               //绑定事件等
            }
         } ;
      }
   } ;
   return dire ;
});
*/