var tmpSdbOptionBase = {
   cond: SdbOptionBase.prototype.cond,
   flags: SdbOptionBase.prototype.flags,
   help: SdbOptionBase.prototype.help,
   hint: SdbOptionBase.prototype.hint,
   limit: SdbOptionBase.prototype.limit,
   sel: SdbOptionBase.prototype.sel,
   skip: SdbOptionBase.prototype.skip,
   sort: SdbOptionBase.prototype.sort,
   toString: SdbOptionBase.prototype.toString
};
var funcSdbOptionBase = SdbOptionBase;
var funcSdbOptionBasehelp = SdbOptionBase.help;
SdbOptionBase=function(){try{return funcSdbOptionBase.apply( this, arguments ); } catch( e ) { throw new Error(e) } };
SdbOptionBase.help = function(){try{ return funcSdbOptionBasehelp.apply( this, arguments ); } catch( e ) { throw new Error(e) } };
SdbOptionBase.prototype.cond=function(){try{return tmpSdbOptionBase.cond.apply(this,arguments);}catch(e){throw new Error(e);}};
SdbOptionBase.prototype.flags=function(){try{return tmpSdbOptionBase.flags.apply(this,arguments);}catch(e){throw new Error(e);}};
SdbOptionBase.prototype.help=function(){try{return tmpSdbOptionBase.help.apply(this,arguments);}catch(e){throw new Error(e);}};
SdbOptionBase.prototype.hint=function(){try{return tmpSdbOptionBase.hint.apply(this,arguments);}catch(e){throw new Error(e);}};
SdbOptionBase.prototype.limit=function(){try{return tmpSdbOptionBase.limit.apply(this,arguments);}catch(e){throw new Error(e);}};
SdbOptionBase.prototype.sel=function(){try{return tmpSdbOptionBase.sel.apply(this,arguments);}catch(e){throw new Error(e);}};
SdbOptionBase.prototype.skip=function(){try{return tmpSdbOptionBase.skip.apply(this,arguments);}catch(e){throw new Error(e);}};
SdbOptionBase.prototype.sort=function(){try{return tmpSdbOptionBase.sort.apply(this,arguments);}catch(e){throw new Error(e);}};
SdbOptionBase.prototype.toString=function(){try{return tmpSdbOptionBase.toString.apply(this,arguments);}catch(e){throw new Error(e);}};
