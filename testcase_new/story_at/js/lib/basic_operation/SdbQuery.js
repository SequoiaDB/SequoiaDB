if ( tmpSdbQuery == undefined )
{
   var tmpSdbQuery = {
      _checkExecuted: SdbQuery.prototype._checkExecuted,
      _exec: SdbQuery.prototype._exec,
      arrayAccess: SdbQuery.prototype.arrayAccess,
      close: SdbQuery.prototype.close,
      count: SdbQuery.prototype.count,
      current: SdbQuery.prototype.current,
      explain: SdbQuery.prototype.explain,
      flags: SdbQuery.prototype.flags,
      getQueryMeta: SdbQuery.prototype.getQueryMeta,
      help: SdbQuery.prototype.help,
      hint: SdbQuery.prototype.hint,
      limit: SdbQuery.prototype.limit,
      next: SdbQuery.prototype.next,
      remove: SdbQuery.prototype.remove,
      size: SdbQuery.prototype.size,
      skip: SdbQuery.prototype.skip,
      sort: SdbQuery.prototype.sort,
      toArray: SdbQuery.prototype.toArray,
      toString: SdbQuery.prototype.toString,
      update: SdbQuery.prototype.update
   };
}
var funcSdbQuery = ( funcSdbQuery == undefined ) ? SdbQuery : funcSdbQuery;
var funcSdbQueryhelp = funcSdbQuery.help;
SdbQuery=function(){try{return funcSdbQuery.apply( this, arguments ); } catch( e ) { throw new Error(e) } };
SdbQuery.help = function(){try{ return funcSdbQueryhelp.apply( this, arguments ); } catch( e ) { throw new Error(e) } };
SdbQuery.prototype._checkExecuted=function(){try{return tmpSdbQuery._checkExecuted.apply(this,arguments);}catch(e){throw new Error(e);}};
SdbQuery.prototype._exec=function(){try{return tmpSdbQuery._exec.apply(this,arguments);}catch(e){throw new Error(e);}};
SdbQuery.prototype.arrayAccess=function(){try{return tmpSdbQuery.arrayAccess.apply(this,arguments);}catch(e){throw new Error(e);}};
SdbQuery.prototype.close=function(){try{return tmpSdbQuery.close.apply(this,arguments);}catch(e){throw new Error(e);}};
SdbQuery.prototype.count=function(){try{return tmpSdbQuery.count.apply(this,arguments);}catch(e){throw new Error(e);}};
SdbQuery.prototype.current=function(){try{return tmpSdbQuery.current.apply(this,arguments);}catch(e){throw new Error(e);}};
SdbQuery.prototype.explain=function(){try{return tmpSdbQuery.explain.apply(this,arguments);}catch(e){throw new Error(e);}};
SdbQuery.prototype.flags=function(){try{return tmpSdbQuery.flags.apply(this,arguments);}catch(e){throw new Error(e);}};
SdbQuery.prototype.getQueryMeta=function(){try{return tmpSdbQuery.getQueryMeta.apply(this,arguments);}catch(e){throw new Error(e);}};
SdbQuery.prototype.help=function(){try{return tmpSdbQuery.help.apply(this,arguments);}catch(e){throw new Error(e);}};
SdbQuery.prototype.hint=function(){try{return tmpSdbQuery.hint.apply(this,arguments);}catch(e){throw new Error(e);}};
SdbQuery.prototype.limit=function(){try{return tmpSdbQuery.limit.apply(this,arguments);}catch(e){throw new Error(e);}};
SdbQuery.prototype.next=function(){try{return tmpSdbQuery.next.apply(this,arguments);}catch(e){throw new Error(e);}};
SdbQuery.prototype.remove=function(){try{return tmpSdbQuery.remove.apply(this,arguments);}catch(e){throw new Error(e);}};
SdbQuery.prototype.size=function(){try{return tmpSdbQuery.size.apply(this,arguments);}catch(e){throw new Error(e);}};
SdbQuery.prototype.skip=function(){try{return tmpSdbQuery.skip.apply(this,arguments);}catch(e){throw new Error(e);}};
SdbQuery.prototype.sort=function(){try{return tmpSdbQuery.sort.apply(this,arguments);}catch(e){throw new Error(e);}};
SdbQuery.prototype.toArray=function(){try{return tmpSdbQuery.toArray.apply(this,arguments);}catch(e){throw new Error(e);}};
SdbQuery.prototype.toString=function(){try{return tmpSdbQuery.toString.apply(this,arguments);}catch(e){throw new Error(e);}};
SdbQuery.prototype.update=function(){try{return tmpSdbQuery.update.apply(this,arguments);}catch(e){throw new Error(e);}};
