if ( tmpCLCount == undefined )
{
   var tmpCLCount = {
      _exec: CLCount.prototype._exec,
      help: CLCount.prototype.help,
      hint: CLCount.prototype.hint,
      toString: CLCount.prototype.toString,
      valueOf: CLCount.prototype.valueOf
   };
}
var funcCLCount = ( funcCLCount == undefined ) ? CLCount : funcCLCount;
var funcCLCounthelp = funcCLCount.help;
CLCount=function(){try{return funcCLCount.apply( this, arguments ); } catch( e ) { throw new Error(e) } };
CLCount.help = function(){try{ return funcCLCounthelp.apply( this, arguments ); } catch( e ) { throw new Error(e) } };
CLCount.prototype._exec=function(){try{return tmpCLCount._exec.apply(this,arguments);}catch(e){throw new Error(e);}};
CLCount.prototype.help=function(){try{return tmpCLCount.help.apply(this,arguments);}catch(e){throw new Error(e);}};
CLCount.prototype.hint=function(){try{return tmpCLCount.hint.apply(this,arguments);}catch(e){throw new Error(e);}};
CLCount.prototype.toString=function(){try{return tmpCLCount.toString.apply(this,arguments);}catch(e){throw new Error(e);}};
CLCount.prototype.valueOf=function(){try{return tmpCLCount.valueOf.apply(this,arguments);}catch(e){throw new Error(e);}};
