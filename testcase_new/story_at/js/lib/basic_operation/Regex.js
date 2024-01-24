if ( tmpRegex == undefined )
{
   var tmpRegex = {
      help: Regex.prototype.help,
      toString: Regex.prototype.toString
   };
}
var funcRegex = ( funcRegex == undefined ) ? Regex : funcRegex;
var funcRegexhelp = funcRegex.help;
Regex=function(){try{return funcRegex.apply( this, arguments ); } catch( e ) { throw new Error(e) } };
Regex.help = function(){try{ return funcRegexhelp.apply( this, arguments ); } catch( e ) { throw new Error(e) } };
Regex.prototype.help=function(){try{return tmpRegex.help.apply(this,arguments);}catch(e){throw new Error(e);}};
Regex.prototype.toString=function(){try{return tmpRegex.toString.apply(this,arguments);}catch(e){throw new Error(e);}};
