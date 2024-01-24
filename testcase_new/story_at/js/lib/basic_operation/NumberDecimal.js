if ( tmpNumberDecimal == undefined )
{
   var tmpNumberDecimal = {
      help: NumberDecimal.prototype.help,
      toString: NumberDecimal.prototype.toString,
      valueOf: NumberDecimal.prototype.valueOf
   };
}
var funcNumberDecimal = ( funcNumberDecimal == undefined ) ? NumberDecimal : funcNumberDecimal;
var funcNumberDecimalhelp = funcNumberDecimal.help;
NumberDecimal=function(){try{return funcNumberDecimal.apply( this, arguments ); } catch( e ) { throw new Error(e) } };
NumberDecimal.help = function(){try{ return funcNumberDecimalhelp.apply( this, arguments ); } catch( e ) { throw new Error(e) } };
NumberDecimal.prototype.help=function(){try{return tmpNumberDecimal.help.apply(this,arguments);}catch(e){throw new Error(e);}};
NumberDecimal.prototype.toString=function(){try{return tmpNumberDecimal.toString.apply(this,arguments);}catch(e){throw new Error(e);}};
NumberDecimal.prototype.valueOf=function(){try{return tmpNumberDecimal.valueOf.apply(this,arguments);}catch(e){throw new Error(e);}};
