if ( tmpSdbSequence == undefined )
{
   var tmpSdbSequence = {
      fetch: SdbSequence.prototype.fetch,
      getCurrentValue: SdbSequence.prototype.getCurrentValue,
      getNextValue: SdbSequence.prototype.getNextValue,
      help: SdbSequence.prototype.help,
      restart: SdbSequence.prototype.restart,
      setAttributes: SdbSequence.prototype.setAttributes,
      setCurrentValue: SdbSequence.prototype.setCurrentValue,
      toString: SdbSequence.prototype.toString
   };
}
var funcSdbSequence = ( funcSdbSequence == undefined ) ? SdbSequence : funcSdbSequence;
var funcSdbSequencehelp = funcSdbSequence.help;
SdbSequence=function(){try{return funcSdbSequence.apply( this, arguments ); } catch( e ) { throw new Error(e) } };
SdbSequence.help = function(){try{ return funcSdbSequencehelp.apply( this, arguments ); } catch( e ) { throw new Error(e) } };
SdbSequence.prototype.fetch=function(){try{return tmpSdbSequence.fetch.apply(this,arguments);}catch(e){throw new Error(e);}};
SdbSequence.prototype.getCurrentValue=function(){try{return tmpSdbSequence.getCurrentValue.apply(this,arguments);}catch(e){throw new Error(e);}};
SdbSequence.prototype.getNextValue=function(){try{return tmpSdbSequence.getNextValue.apply(this,arguments);}catch(e){throw new Error(e);}};
SdbSequence.prototype.help=function(){try{return tmpSdbSequence.help.apply(this,arguments);}catch(e){throw new Error(e);}};
SdbSequence.prototype.restart=function(){try{return tmpSdbSequence.restart.apply(this,arguments);}catch(e){throw new Error(e);}};
SdbSequence.prototype.setAttributes=function(){try{return tmpSdbSequence.setAttributes.apply(this,arguments);}catch(e){throw new Error(e);}};
SdbSequence.prototype.setCurrentValue=function(){try{return tmpSdbSequence.setCurrentValue.apply(this,arguments);}catch(e){throw new Error(e);}};
SdbSequence.prototype.toString=function(){try{return tmpSdbSequence.toString.apply(this,arguments);}catch(e){throw new Error(e);}};
