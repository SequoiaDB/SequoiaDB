if ( tmpMinKey == undefined )
{
   var tmpMinKey = {
      help: MinKey.prototype.help,
      toString: MinKey.prototype.toString
   };
}
var funcMinKey = ( funcMinKey == undefined ) ? MinKey : funcMinKey;
var funcMinKeyhelp = funcMinKey.help;
MinKey=function(){try{return funcMinKey.apply( this, arguments ); } catch( e ) { throw new Error(e) } };
MinKey.help = function(){try{ return funcMinKeyhelp.apply( this, arguments ); } catch( e ) { throw new Error(e) } };
MinKey.prototype.help=function(){try{return tmpMinKey.help.apply(this,arguments);}catch(e){throw new Error(e);}};
MinKey.prototype.toString=function(){try{return tmpMinKey.toString.apply(this,arguments);}catch(e){throw new Error(e);}};
