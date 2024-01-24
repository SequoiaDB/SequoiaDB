import( 'lib/jsonEqual.js' ) ;

var SdbUnit = function( title ){
   this.title = title ;
   this.first = { 'name': 'setUp', 'func': function(){}, 'error': null, 'result': null } ;
   this.last = { 'name': 'finish', 'func': function(){}, 'error': null, 'result': null } ;
   this.current = null ;
   this.isInterrupt = false ;
   this.tests = [] ;
   this.hasError = false ;
   this.errorId = -1 ;
} ;

SdbUnit.prototype = {} ;
SdbUnit.prototype.constructor = SdbUnit;

//do some setup code
SdbUnit.prototype.setUp = function( func ){
   this.first['func'] = func ;
}

//do some cleanup code
SdbUnit.prototype.finish = function( func ){
   this.last['func'] = func ;
}

// test stuff
SdbUnit.prototype.test = function( name, func, isInterrupt ){
   this.tests.push( { 'name': name, 'func': func, 'error': null, 'result': null, 'isInterrupt': isInterrupt === true } ) ;
}

function _equal( v1, v2 ){
   var t1 = typeof( v1 ) ;
   var t2 = typeof( v2 ) ;

   if( t1 !== t2 )
   {
      return false ;
   }

   if( t1 == 'string' || t1 == 'number' || t1 == 'boolean' )
   {
      return ( v1 === v2 ) ;
   }
   else if( t1 == 'undefined' )
   {
      return true ;
   }
   else if( t1 == 'object' )
   {
      return _jsonEqual( [ v1 ], [ v2 ] ) ;
   }
   else
   {
      return ( v1 === v2 ) ;
   }
}

SdbUnit.prototype.assert = function( expected, actual, detail, isInterrupt ){
   if( _equal( expected, actual ) == false )
   {
      this.current['result'] = {
         'expected': expected,
         'actual': actual,
         'detail': detail
      } ;
      this.isInterrupt = isInterrupt === true ;
      throw new Error( "sdb unit assert" ) ;
   }
}

// start test
SdbUnit.prototype.start = function( showProgress ){
   var report ;
   this.current = this.first ;
   try
   {
      this.first['func']() ;
   }
   catch( e )
   {
      this.hasError = true ;
      this.first['error'] = e ;
      this.errorId = 0 ;
   }

   if( showProgress === true )
   {
      var title = _fillString( " Progress ", "=", 40, true ) ;
      title = _fillString( title, "=", 80, false ) ;
      println( title ) ;

      report = "===> " + this.first['name'] ;
      println( report ) ;
   }

   if( this.hasError == false )
   {
      for( var i in this.tests )
      {
         this.current = this.tests[i] ;
         try
         {
            this.tests[i]['func']() ;

            if( showProgress === true )
            {
               report = "===> " + this.tests[i]['name'] ;
               println( report ) ;
            }
         }
         catch( e )
         {
            this.hasError = true ;
            this.tests[i]['error'] = e ;
            if ( this.isInterrupt || this.tests[i]['isInterrupt'] )
            {
               this.errorId = i + 1 ;
               break ;
            }
         }
      }
   }

   this.current = this.first ;
   try
   {
      this.last['func']( this.hasError ) ;
   }
   catch( e )
   {
      this.last['error'] = e ;
      println( e ) ;
   }

   if( showProgress === true )
   {
      report = "===> " + this.last['name'] ;
      println( report ) ;
   }

   this.report() ;

   if( this.hasError )
   {
      throw new Error( "Unit test failed: " + this.title ) ;
   }
}

function _fillString( str, fill_str, num, isLeft )
{
   var fillNum = num - str.length ;

   if( fillNum < 5 )
   {
      fillNum = 5 ;
   }

   var fill_str_num = fill_str.length ;

   for( var i = 0; i < fillNum; i += fill_str_num )
   {
      if( isLeft )
      {
         str = fill_str + str ;
      }
      else
      {
         str = str + fill_str ;
      }
   }

   return str ;
}

function _printUnitTestReport( i, testInfo, isInterrupt )
{
   var report = "===> " + testInfo['name'] ;
   report = _fillString( report, " ", 65, false ) ;

   if( testInfo['error'] === null && testInfo['result'] === null )
   {
      if ( isInterrupt === true )
      {
         println( report + "[ ----:   " + i + " ]" ) ;
      }
      else
      {
         println( report + "[ Done:   " + i + " ]" ) ;
      }
   }
   else
   {
      println( report + "[ Failed: " + i + " ]" ) ;

      if ( testInfo['error'] !== null )
      {
         println( "\n*** Stack:\n" + testInfo['error'] ) ;
         println( testInfo['error'].stack ) ;
      }

      if ( testInfo['result'] !== null )
      {
         if( testInfo['result']['detail'] )
         {
            println( "\n### Detail:\n" + JSON.stringify( testInfo['result']['detail'], null, 3 ) ) ;
         }
         println( "### Expected:\n" + JSON.stringify( testInfo['result']['expected'], null, 3 ) ) ;
         println( "\n### Actual:\n" + JSON.stringify( testInfo['result']['actual'], null, 3 ) + "\n" ) ;
      }
   }
}

SdbUnit.prototype.report = function(){
   var title = _fillString( " " + this.title + " ", "=", 40, true ) ;
   title = _fillString( title, "=", 80, false ) ;
   println( title ) ;

   _printUnitTestReport( 1, this.first ) ;

   var isInterrupt = false ;

   if( this.errorId == 0 )
   {
      isInterrupt = true ;
   }

   for( var i in this.tests )
   {
      _printUnitTestReport( parseInt( i ) + 2, this.tests[i], isInterrupt ) ;
      if ( this.errorId >= 0 && this.errorId == i + 1 )
      {
         isInterrupt = true ;
      }
   }

   title = " Test finish " ;
   title = _fillString( title, "=", 40, true ) ;
   title = _fillString( title, "=", 80, false ) ;
   println( title ) ;
}
