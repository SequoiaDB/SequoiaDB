if ( tmpSdbSnapshotOption == undefined )
{
   var tmpSdbSnapshotOption = {
      cond: SdbSnapshotOption.prototype.cond,
      flags: SdbSnapshotOption.prototype.flags,
      help: SdbSnapshotOption.prototype.help,
      hint: SdbSnapshotOption.prototype.hint,
      limit: SdbSnapshotOption.prototype.limit,
      options: SdbSnapshotOption.prototype.options,
      sel: SdbSnapshotOption.prototype.sel,
      skip: SdbSnapshotOption.prototype.skip,
      sort: SdbSnapshotOption.prototype.sort,
      toString: SdbSnapshotOption.prototype.toString
   };
}
var funcSdbSnapshotOption = ( funcSdbSnapshotOption == undefined ) ? SdbSnapshotOption : funcSdbSnapshotOption;
var funcSdbSnapshotOptionhelp = funcSdbSnapshotOption.help;
SdbSnapshotOption=function(){try{return funcSdbSnapshotOption.apply( this, arguments ); } catch( e ) { throw new Error(e) } };
SdbSnapshotOption.help = function(){try{ return funcSdbSnapshotOptionhelp.apply( this, arguments ); } catch( e ) { throw new Error(e) } };
SdbSnapshotOption.prototype.cond=function(){try{return tmpSdbSnapshotOption.cond.apply(this,arguments);}catch(e){throw new Error(e);}};
SdbSnapshotOption.prototype.flags=function(){try{return tmpSdbSnapshotOption.flags.apply(this,arguments);}catch(e){throw new Error(e);}};
SdbSnapshotOption.prototype.help=function(){try{return tmpSdbSnapshotOption.help.apply(this,arguments);}catch(e){throw new Error(e);}};
SdbSnapshotOption.prototype.hint=function(){try{return tmpSdbSnapshotOption.hint.apply(this,arguments);}catch(e){throw new Error(e);}};
SdbSnapshotOption.prototype.limit=function(){try{return tmpSdbSnapshotOption.limit.apply(this,arguments);}catch(e){throw new Error(e);}};
SdbSnapshotOption.prototype.options=function(){try{return tmpSdbSnapshotOption.options.apply(this,arguments);}catch(e){throw new Error(e);}};
SdbSnapshotOption.prototype.sel=function(){try{return tmpSdbSnapshotOption.sel.apply(this,arguments);}catch(e){throw new Error(e);}};
SdbSnapshotOption.prototype.skip=function(){try{return tmpSdbSnapshotOption.skip.apply(this,arguments);}catch(e){throw new Error(e);}};
SdbSnapshotOption.prototype.sort=function(){try{return tmpSdbSnapshotOption.sort.apply(this,arguments);}catch(e){throw new Error(e);}};
SdbSnapshotOption.prototype.toString=function(){try{return tmpSdbSnapshotOption.toString.apply(this,arguments);}catch(e){throw new Error(e);}};
