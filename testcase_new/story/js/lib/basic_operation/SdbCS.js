var tmpSdbCS = {
   _resolveCL: SdbCS.prototype._resolveCL,
   alter: SdbCS.prototype.alter,
   createCL: SdbCS.prototype.createCL,
   disableCapped: SdbCS.prototype.disableCapped,
   dropCL: SdbCS.prototype.dropCL,
   enableCapped: SdbCS.prototype.enableCapped,
   getCL: SdbCS.prototype.getCL,
   help: SdbCS.prototype.help,
   removeDomain: SdbCS.prototype.removeDomain,
   renameCL: SdbCS.prototype.renameCL,
   setAttributes: SdbCS.prototype.setAttributes,
   setDomain: SdbCS.prototype.setDomain,
   toString: SdbCS.prototype.toString
};
var funcSdbCS = SdbCS;
var funcSdbCShelp = SdbCS.help;
SdbCS=function(){try{return funcSdbCS.apply( this, arguments ); } catch( e ) { throw new Error(e) } };
SdbCS.help = function(){try{ return funcSdbCShelp.apply( this, arguments ); } catch( e ) { throw new Error(e) } };
SdbCS.prototype._resolveCL=function(){try{return tmpSdbCS._resolveCL.apply(this,arguments);}catch(e){throw new Error(e);}};
SdbCS.prototype.alter=function(){try{return tmpSdbCS.alter.apply(this,arguments);}catch(e){throw new Error(e);}};
SdbCS.prototype.createCL=function(){try{return tmpSdbCS.createCL.apply(this,arguments);}catch(e){throw new Error(e);}};
SdbCS.prototype.disableCapped=function(){try{return tmpSdbCS.disableCapped.apply(this,arguments);}catch(e){throw new Error(e);}};
SdbCS.prototype.dropCL=function(){try{return tmpSdbCS.dropCL.apply(this,arguments);}catch(e){throw new Error(e);}};
SdbCS.prototype.enableCapped=function(){try{return tmpSdbCS.enableCapped.apply(this,arguments);}catch(e){throw new Error(e);}};
SdbCS.prototype.getCL=function(){try{return tmpSdbCS.getCL.apply(this,arguments);}catch(e){throw new Error(e);}};
SdbCS.prototype.help=function(){try{return tmpSdbCS.help.apply(this,arguments);}catch(e){throw new Error(e);}};
SdbCS.prototype.removeDomain=function(){try{return tmpSdbCS.removeDomain.apply(this,arguments);}catch(e){throw new Error(e);}};
SdbCS.prototype.renameCL=function(){try{return tmpSdbCS.renameCL.apply(this,arguments);}catch(e){throw new Error(e);}};
SdbCS.prototype.setAttributes=function(){try{return tmpSdbCS.setAttributes.apply(this,arguments);}catch(e){throw new Error(e);}};
SdbCS.prototype.setDomain=function(){try{return tmpSdbCS.setDomain.apply(this,arguments);}catch(e){throw new Error(e);}};
SdbCS.prototype.toString=function(){try{return tmpSdbCS.toString.apply(this,arguments);}catch(e){throw new Error(e);}};
