if ( tmpSdbQueryOption == undefined )
{
   var tmpSdbQueryOption = {
      cond: SdbQueryOption.prototype.cond,
      flags: SdbQueryOption.prototype.flags,
      help: SdbQueryOption.prototype.help,
      hint: SdbQueryOption.prototype.hint,
      limit: SdbQueryOption.prototype.limit,
      remove: SdbQueryOption.prototype.remove,
      sel: SdbQueryOption.prototype.sel,
      skip: SdbQueryOption.prototype.skip,
      sort: SdbQueryOption.prototype.sort,
      toString: SdbQueryOption.prototype.toString,
      update: SdbQueryOption.prototype.update
   };
}
var funcSdbQueryOption = ( funcSdbQueryOption == undefined ) ? SdbQueryOption : funcSdbQueryOption;
var funcSdbQueryOptionhelp = funcSdbQueryOption.help;
SdbQueryOption=function(){try{return funcSdbQueryOption.apply( this, arguments ); } catch( e ) { throw new Error(e) } };
SdbQueryOption.help = function(){try{ return funcSdbQueryOptionhelp.apply( this, arguments ); } catch( e ) { throw new Error(e) } };
SdbQueryOption.prototype.cond=function(){try{return tmpSdbQueryOption.cond.apply(this,arguments);}catch(e){throw new Error(e);}};
SdbQueryOption.prototype.flags=function(){try{return tmpSdbQueryOption.flags.apply(this,arguments);}catch(e){throw new Error(e);}};
SdbQueryOption.prototype.help=function(){try{return tmpSdbQueryOption.help.apply(this,arguments);}catch(e){throw new Error(e);}};
SdbQueryOption.prototype.hint=function(){try{return tmpSdbQueryOption.hint.apply(this,arguments);}catch(e){throw new Error(e);}};
SdbQueryOption.prototype.limit=function(){try{return tmpSdbQueryOption.limit.apply(this,arguments);}catch(e){throw new Error(e);}};
SdbQueryOption.prototype.remove=function(){try{return tmpSdbQueryOption.remove.apply(this,arguments);}catch(e){throw new Error(e);}};
SdbQueryOption.prototype.sel=function(){try{return tmpSdbQueryOption.sel.apply(this,arguments);}catch(e){throw new Error(e);}};
SdbQueryOption.prototype.skip=function(){try{return tmpSdbQueryOption.skip.apply(this,arguments);}catch(e){throw new Error(e);}};
SdbQueryOption.prototype.sort=function(){try{return tmpSdbQueryOption.sort.apply(this,arguments);}catch(e){throw new Error(e);}};
SdbQueryOption.prototype.toString=function(){try{return tmpSdbQueryOption.toString.apply(this,arguments);}catch(e){throw new Error(e);}};
SdbQueryOption.prototype.update=function(){try{return tmpSdbQueryOption.update.apply(this,arguments);}catch(e){throw new Error(e);}};
