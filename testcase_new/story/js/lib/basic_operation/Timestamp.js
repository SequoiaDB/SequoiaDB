var tmpTimestamp = {
   help: Timestamp.prototype.help,
   toString: Timestamp.prototype.toString
};
var funcTimestamp = Timestamp;
var funcTimestamphelp = Timestamp.help;
Timestamp=function(){try{return funcTimestamp.apply( this, arguments ); } catch( e ) { throw new Error(e) } };
Timestamp.help = function(){try{ return funcTimestamphelp.apply( this, arguments ); } catch( e ) { throw new Error(e) } };
Timestamp.prototype.help=function(){try{return tmpTimestamp.help.apply(this,arguments);}catch(e){throw new Error(e);}};
Timestamp.prototype.toString=function(){try{return tmpTimestamp.toString.apply(this,arguments);}catch(e){throw new Error(e);}};
