var tmpMaxKey = {
   help: MaxKey.prototype.help,
   toString: MaxKey.prototype.toString
};
var funcMaxKey = MaxKey;
var funcMaxKeyhelp = MaxKey.help;
MaxKey=function(){try{return funcMaxKey.apply( this, arguments ); } catch( e ) { throw new Error(e) } };
MaxKey.help = function(){try{ return funcMaxKeyhelp.apply( this, arguments ); } catch( e ) { throw new Error(e) } };
MaxKey.prototype.help=function(){try{return tmpMaxKey.help.apply(this,arguments);}catch(e){throw new Error(e);}};
MaxKey.prototype.toString=function(){try{return tmpMaxKey.toString.apply(this,arguments);}catch(e){throw new Error(e);}};
