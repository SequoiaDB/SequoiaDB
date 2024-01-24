var tmpSdbTraceOption = {
   breakPoints: SdbTraceOption.prototype.breakPoints,
   components: SdbTraceOption.prototype.components,
   functionNames: SdbTraceOption.prototype.functionNames,
   help: SdbTraceOption.prototype.help,
   threadTypes: SdbTraceOption.prototype.threadTypes,
   tids: SdbTraceOption.prototype.tids,
   toString: SdbTraceOption.prototype.toString
};
var funcSdbTraceOption = SdbTraceOption;
var funcSdbTraceOptionhelp = SdbTraceOption.help;
SdbTraceOption=function(){try{return funcSdbTraceOption.apply( this, arguments ); } catch( e ) { throw new Error(e) } };
SdbTraceOption.help = function(){try{ return funcSdbTraceOptionhelp.apply( this, arguments ); } catch( e ) { throw new Error(e) } };
SdbTraceOption.prototype.breakPoints=function(){try{return tmpSdbTraceOption.breakPoints.apply(this,arguments);}catch(e){throw new Error(e);}};
SdbTraceOption.prototype.components=function(){try{return tmpSdbTraceOption.components.apply(this,arguments);}catch(e){throw new Error(e);}};
SdbTraceOption.prototype.functionNames=function(){try{return tmpSdbTraceOption.functionNames.apply(this,arguments);}catch(e){throw new Error(e);}};
SdbTraceOption.prototype.help=function(){try{return tmpSdbTraceOption.help.apply(this,arguments);}catch(e){throw new Error(e);}};
SdbTraceOption.prototype.threadTypes=function(){try{return tmpSdbTraceOption.threadTypes.apply(this,arguments);}catch(e){throw new Error(e);}};
SdbTraceOption.prototype.tids=function(){try{return tmpSdbTraceOption.tids.apply(this,arguments);}catch(e){throw new Error(e);}};
SdbTraceOption.prototype.toString=function(){try{return tmpSdbTraceOption.toString.apply(this,arguments);}catch(e){throw new Error(e);}};
