if ( tmpSdbDate == undefined )
{
   var tmpSdbDate = {
      help: SdbDate.prototype.help,
      toString: SdbDate.prototype.toString
   };
}
var funcSdbDate = ( funcSdbDate == undefined ) ? SdbDate : funcSdbDate;
var funcSdbDatehelp = funcSdbDate.help;
SdbDate=function(){try{return funcSdbDate.apply( this, arguments ); } catch( e ) { throw new Error(e) } };
SdbDate.help = function(){try{ return funcSdbDatehelp.apply( this, arguments ); } catch( e ) { throw new Error(e) } };
SdbDate.prototype.help=function(){try{return tmpSdbDate.help.apply(this,arguments);}catch(e){throw new Error(e);}};
SdbDate.prototype.toString=function(){try{return tmpSdbDate.toString.apply(this,arguments);}catch(e){throw new Error(e);}};
