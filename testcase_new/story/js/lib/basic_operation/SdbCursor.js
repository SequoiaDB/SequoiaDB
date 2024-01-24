var tmpSdbCursor = {
   arrayAccess: SdbCursor.prototype.arrayAccess,
   close: SdbCursor.prototype.close,
   current: SdbCursor.prototype.current,
   help: SdbCursor.prototype.help,
   next: SdbCursor.prototype.next,
   size: SdbCursor.prototype.size,
   toArray: SdbCursor.prototype.toArray,
   toString: SdbCursor.prototype.toString
};
var funcSdbCursor = SdbCursor;
var funcSdbCursorhelp = SdbCursor.help;
SdbCursor=function(){try{return funcSdbCursor.apply( this, arguments ); } catch( e ) { throw new Error(e) } };
SdbCursor.help = function(){try{ return funcSdbCursorhelp.apply( this, arguments ); } catch( e ) { throw new Error(e) } };
SdbCursor.prototype.arrayAccess=function(){try{return tmpSdbCursor.arrayAccess.apply(this,arguments);}catch(e){throw new Error(e);}};
SdbCursor.prototype.close=function(){try{return tmpSdbCursor.close.apply(this,arguments);}catch(e){throw new Error(e);}};
SdbCursor.prototype.current=function(){try{return tmpSdbCursor.current.apply(this,arguments);}catch(e){throw new Error(e);}};
SdbCursor.prototype.help=function(){try{return tmpSdbCursor.help.apply(this,arguments);}catch(e){throw new Error(e);}};
SdbCursor.prototype.next=function(){try{return tmpSdbCursor.next.apply(this,arguments);}catch(e){throw new Error(e);}};
SdbCursor.prototype.size=function(){try{return tmpSdbCursor.size.apply(this,arguments);}catch(e){throw new Error(e);}};
SdbCursor.prototype.toArray=function(){try{return tmpSdbCursor.toArray.apply(this,arguments);}catch(e){throw new Error(e);}};
SdbCursor.prototype.toString=function(){try{return tmpSdbCursor.toString.apply(this,arguments);}catch(e){throw new Error(e);}};
