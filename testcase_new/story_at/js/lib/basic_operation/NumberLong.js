if ( tmpNumberLong == undefined )
{
   var tmpNumberLong = {
      help: NumberLong.prototype.help,
      toString: NumberLong.prototype.toString,
      valueOf: NumberLong.prototype.valueOf
   };
}
var funcNumberLong = ( funcNumberLong == undefined ) ? NumberLong : funcNumberLong;
var funcNumberLonghelp = funcNumberLong.help;
NumberLong=function(){try{return funcNumberLong.apply( this, arguments ); } catch( e ) { throw new Error(e) } };
NumberLong.help = function(){try{ return funcNumberLonghelp.apply( this, arguments ); } catch( e ) { throw new Error(e) } };
NumberLong.prototype.help=function(){try{return tmpNumberLong.help.apply(this,arguments);}catch(e){throw new Error(e);}};
NumberLong.prototype.toString=function(){try{return tmpNumberLong.toString.apply(this,arguments);}catch(e){throw new Error(e);}};
NumberLong.prototype.valueOf=function(){try{return tmpNumberLong.valueOf.apply(this,arguments);}catch(e){throw new Error(e);}};
