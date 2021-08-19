(function(){
   var sacApp = window.SdbSacManagerModule ;
   //智能布局
   sacApp.service( 'InheritSize', function( $window ){
      var g = this ;
      var list = [] ;
      function _renderWidth( ele, width )
      {  
         var marginLeft  = $( ele ).attr( 'data-mLeft' ) ;
         var marginRight = $( ele ).attr( 'data-mRight' ) ;
         var offsetX = $( ele ).attr( 'data-offsetX' ) ;
         marginLeft = ( isNaN( marginLeft ) ? 0 : parseInt( marginLeft ) ) ;
         marginRight = ( isNaN( marginRight ) ? 0 : parseInt( marginRight ) ) ;
         offsetX = ( isNaN( offsetX ) ? 0 : parseInt( offsetX ) ) ;
         width += offsetX ;
         $( ele ).outerWidth( width ).css( { marginLeft: marginLeft, marginRight: marginRight } ) ;
      }
      function _renderHeight( ele, height )
      {
         var marginTop  = $( ele ).attr( 'data-mTop' ) ;
         var marginBottom = $( ele ).attr( 'data-mBottom' ) ;
         var offsetY = $( ele ).attr( 'data-offsetY' ) ;
         marginTop = ( isNaN( marginTop ) ? 0 : parseInt( marginTop ) ) ;
         marginBottom = ( isNaN( marginBottom ) ? 0 : parseInt( marginBottom ) ) ;
         offsetY = ( isNaN( offsetY ) ? 0 : parseInt( offsetY ) ) ;
         height += offsetY ;
         $( ele ).outerHeight( height ).css( { marginTop: marginTop, marginBottom: marginBottom } ) ;
      }
      function _render( ele )
      {
         $( document.body ).css( 'overflow', 'hidden' ) ;
         var width  = $( ele ).attr( 'data-width' ) ;
         var height = $( ele ).attr( 'data-height' ) ;
         width  = ( isNaN( width )  ? $( ele ).parent().width()  : parseInt( width  ) ) ;
         height = ( isNaN( height ) ? $( ele ).parent().height() : parseInt( height ) ) ;
         _renderWidth( ele, width ) ;
         _renderHeight( ele, height ) ;
         $( document.body ).css( 'overflow', 'auto' ) ;
      }
      g.renderAll = function( element )
      {
         var isStart = false ;
         if( typeof( element ) == 'undefined' ) isStart = true ;
         $.each( list, function( index, ele ){
            if( isStart )
            {
               _render( ele ) ;
            }
            else
            {
               if( $( element ).get(0) === $( ele ).get(0) )
               {
                  isStart = true ;
               }
            }
         } ) ;
      }
      g.append = function( ele ){
         list.push( ele ) ;
         _render( ele ) ;
      } ;
      angular.element( $window ).bind( 'resize', function () {
         g.renderAll() ;
      } ) ;
   } ) ;
   //动态响应布局
   sacApp.service( 'ResponseLayout', function(){
      var g = this ;
      var list = [] ;
      function _getLineWidth( parentWidth, layoutData ){
         var len = layoutData['para'].length ;
         var lineWidth = 0 ;
         var rvArr = [] ;
         var lineArr = [] ;
         var offset = parseInt( parentWidth * 0.1 ) ;
         var regulateWidth = 0 ;
         $.each( layoutData['para'], function( index, widthJson ){
            var tmpWidth = 0 ;
            var tmpRegulateWidth = 0 ;
            if( widthJson['width'] > 0 )
            {
               tmpWidth = widthJson['width'] ;
            }
            else if( widthJson['minWidth'] > 0 && widthJson['maxWidth'] == 0 )
            {
               tmpWidth = parseInt( ( widthJson['minWidth'] + parentWidth ) / 2 ) ;
               tmpRegulateWidth = parentWidth - tmpWidth ;
            }
            else
            {
               tmpWidth = parseInt( ( widthJson['maxWidth'] + widthJson['minWidth'] ) / 2 ) ;
               tmpRegulateWidth = widthJson['maxWidth'] - tmpWidth ;
            }
            regulateWidth += tmpRegulateWidth ;
            if( lineWidth + tmpWidth - regulateWidth > parentWidth && lineWidth + tmpWidth > parentWidth - offset )
            {
               rvArr.push( lineArr ) ;
               lineWidth = tmpWidth ;
               lineArr = [ widthJson ] ;
               regulateWidth = tmpRegulateWidth ;
            }
            else
            {
               lineArr.push( widthJson ) ;
               lineWidth += tmpWidth ;
            }
            if( index + 1 == len )
            {
               rvArr.push( lineArr ) ;
            }
         } ) ;
         return rvArr ;
      }
      function _setWidth( parentWidth, layoutData, lineInfo ){
         var divs = layoutData['divs'] ;
         var divNum = 0 ;
         //遍历每一行
         $.each( lineInfo, function( index, lineArr ){
            //估算需要的宽度
            var sumWidth = 0 ;
            //动态宽度的数量
            var dynaNum = 0 ;
            //估算一行的宽度
            $.each( lineArr, function( index2, boxWidthArr ) {
               if( boxWidthArr['width'] > 0 )
               {
                  boxWidthArr['newWidth'] = boxWidthArr['width'] ;
                  boxWidthArr['canChange'] = false ;
               }
               else if( boxWidthArr['minWidth'] > 0 && boxWidthArr['maxWidth'] == 0 )
               {
                  boxWidthArr['newWidth'] = parseInt( ( boxWidthArr['minWidth'] + parentWidth ) / 2 ) ;
                  if( boxWidthArr['newWidth'] > parentWidth ) boxWidthArr['newWidth'] = parentWidth ;
                  boxWidthArr['canChange'] = true ;
                  ++dynaNum ;
               }
               else
               {
                  boxWidthArr['newWidth'] = parseInt( ( boxWidthArr['maxWidth'] + boxWidthArr['minWidth'] ) / 2 ) ;
                  if( boxWidthArr['newWidth'] > parentWidth ) boxWidthArr['newWidth'] = parentWidth ;
                  if( boxWidthArr['newWidth'] < boxWidthArr['minWidth'] ) boxWidthArr['newWidth'] = boxWidthArr['minWidth'] ;
                  boxWidthArr['canChange'] = true ;
                  ++dynaNum ;
               }
               sumWidth += boxWidthArr['newWidth'] ;
            } ) ;
            //增量
            var increment = parentWidth - sumWidth ;
            //调整宽度
            function adjustWidth( unuseIncrement )
            {
               if( dynaNum <= 0 ) return;
               //平均增量
               var aveIncrement = parseInt( unuseIncrement / dynaNum ) ;
               //已经使用的增量
               var useIncrement = 0 ;
               //已经使用的动态宽度数量
               var useDynaNum = 0 ;
               //调整所有宽度，除了固定宽度
               $.each( lineArr, function( index2, boxWidthArr ) {
                  if( boxWidthArr['canChange'] == true )
                  {
                     if( useDynaNum + 1 == dynaNum )
                     {
                        boxWidthArr['newWidth'] += ( unuseIncrement - useIncrement ) ;
                        if( boxWidthArr['newWidth'] > boxWidthArr['maxWidth'] ) boxWidthArr['newWidth'] = boxWidthArr['maxWidth'] ;
                        return false ;
                     }
                     else
                     {
                        boxWidthArr['newWidth'] += aveIncrement ;
                        useIncrement += aveIncrement ;
                     }
                     ++useDynaNum ;
                     //检测是否超出范围
                     if( boxWidthArr['newWidth'] < boxWidthArr['minWidth'] )
                     {
                        unuseIncrement += ( boxWidthArr['minWidth'] - boxWidthArr['newWidth'] ) ;
                        unuseIncrement -= useIncrement ;
                        boxWidthArr['newWidth'] = boxWidthArr['minWidth'] ;
                        boxWidthArr['canChange'] = false ;
                        --dynaNum ;
                        adjustWidth( unuseIncrement ) ;
                        return false ;
                     }
                     else if( boxWidthArr['newWidth'] > boxWidthArr['maxWidth'] )
                     {
                        unuseIncrement += ( boxWidthArr['newWidth'] - boxWidthArr['maxWidth'] ) ;
                        unuseIncrement -= useIncrement ;
                        boxWidthArr['newWidth'] = boxWidthArr['maxWidth'] ;
                        boxWidthArr['canChange'] = false ;
                        --dynaNum ;
                        adjustWidth( unuseIncrement ) ;
                        return false ;
                     }
                  }
               } ) ;
            }
            adjustWidth( increment ) ;
            $.each( lineArr, function( index2, boxWidthArr ) {
               $( divs[ divNum ] ).outerWidth( boxWidthArr['newWidth'] ) ;
               ++divNum ;
            } ) ;
         } ) ;
      }
      function _resize( layoutData )
      {
         var parentWidth = layoutData['element'].parent().width() - 2 ;
         var para = layoutData['para'] ;
         if( parentWidth <= 0 )
         {
            return;
         }
         var lineInfo = _getLineWidth( parentWidth, layoutData ) ;
         _setWidth( parentWidth, layoutData, lineInfo ) ;
      }
      g.resize = function( id ){
         var layoutData = list[id] ;
         if( typeof( layoutData ) == 'undefined' ) throw '不存在' ;
         _resize( layoutData ) ;
      }
      g.resizeAll = function(){
         $.each( list, function( index, layoutData ){
            _resize( layoutData ) ;
         } ) ;
      }
      g.render = function( ele ){
         //根对象
         var response = $( ele ) ;
         var parentWidth = response.parent().width() ;
         var divPara = [] ;
         var divs = $( ' > div', response ) ;
         divs.each( function( index ){
            var width = $( this ).attr( 'data-width' ) ;
            var minWidth = $( this ).attr( 'data-min-width' ) ;
            var maxWidth = $( this ).attr( 'data-max-width' ) ;
            if( typeof( width ) == 'undefined' ) width = 0 ;
            if( typeof( minWidth ) == 'undefined' ) minWidth = 0 ;
            if( typeof( maxWidth ) == 'undefined' ) maxWidth = 0 ;
            width = parseInt( width ) ;
            minWidth = parseInt( minWidth ) ;
            maxWidth = parseInt( maxWidth ) ;
            divPara.push( { width: width, minWidth: minWidth, maxWidth: maxWidth } ) ;
            $( this ).css( 'float', 'left' ) ;
         } ) ;
         $( divs[ divs.length - 1 ] ).after( '<div class="clear-float"></div>' ) ;
         var layoutData = { element: response, divs: divs, para: divPara } ;
         list.push( layoutData ) ;
         var id = list.length - 1 ;
         _resize( layoutData ) ;
         return id ;
      }
   } ) ;
}());