(function(){
   var sacApp = window.SdbSacManagerModule ;
   sacApp.directive( 'inheritSize', function( $window, InheritSize ){
      return {
         restrict: 'A',
         priority: 100,
         link: function( scope, element ){
            InheritSize.append( element ) ;
            $( 'div', element ).each( function( index, ele ){
               InheritSize.append( ele ) ;
            } ) ;
         }
      }
   } ) ;
}());