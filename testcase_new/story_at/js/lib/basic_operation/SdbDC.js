if ( tmpSdbDC == undefined )
{
   var tmpSdbDC = {
      activate: SdbDC.prototype.activate,
      attachGroups: SdbDC.prototype.attachGroups,
      createImage: SdbDC.prototype.createImage,
      deactivate: SdbDC.prototype.deactivate,
      detachGroups: SdbDC.prototype.detachGroups,
      disableImage: SdbDC.prototype.disableImage,
      disableReadonly: SdbDC.prototype.disableReadonly,
      enableImage: SdbDC.prototype.enableImage,
      enableReadonly: SdbDC.prototype.enableReadonly,
      getDetail: SdbDC.prototype.getDetail,
      help: SdbDC.prototype.help,
      removeImage: SdbDC.prototype.removeImage,
      setActiveLocation: SdbDC.prototype.setActiveLocation,
      toString: SdbDC.prototype.toString
   };
}
var funcSdbDC = ( funcSdbDC == undefined ) ? SdbDC : funcSdbDC;
var funcSdbDChelp = funcSdbDC.help;
SdbDC=function(){try{return funcSdbDC.apply( this, arguments ); } catch( e ) { throw new Error(e) } };
SdbDC.help = function(){try{ return funcSdbDChelp.apply( this, arguments ); } catch( e ) { throw new Error(e) } };
SdbDC.prototype.activate=function(){try{return tmpSdbDC.activate.apply(this,arguments);}catch(e){throw new Error(e);}};
SdbDC.prototype.attachGroups=function(){try{return tmpSdbDC.attachGroups.apply(this,arguments);}catch(e){throw new Error(e);}};
SdbDC.prototype.createImage=function(){try{return tmpSdbDC.createImage.apply(this,arguments);}catch(e){throw new Error(e);}};
SdbDC.prototype.deactivate=function(){try{return tmpSdbDC.deactivate.apply(this,arguments);}catch(e){throw new Error(e);}};
SdbDC.prototype.detachGroups=function(){try{return tmpSdbDC.detachGroups.apply(this,arguments);}catch(e){throw new Error(e);}};
SdbDC.prototype.disableImage=function(){try{return tmpSdbDC.disableImage.apply(this,arguments);}catch(e){throw new Error(e);}};
SdbDC.prototype.disableReadonly=function(){try{return tmpSdbDC.disableReadonly.apply(this,arguments);}catch(e){throw new Error(e);}};
SdbDC.prototype.enableImage=function(){try{return tmpSdbDC.enableImage.apply(this,arguments);}catch(e){throw new Error(e);}};
SdbDC.prototype.enableReadonly=function(){try{return tmpSdbDC.enableReadonly.apply(this,arguments);}catch(e){throw new Error(e);}};
SdbDC.prototype.getDetail=function(){try{return tmpSdbDC.getDetail.apply(this,arguments);}catch(e){throw new Error(e);}};
SdbDC.prototype.help=function(){try{return tmpSdbDC.help.apply(this,arguments);}catch(e){throw new Error(e);}};
SdbDC.prototype.removeImage=function(){try{return tmpSdbDC.removeImage.apply(this,arguments);}catch(e){throw new Error(e);}};
SdbDC.prototype.setActiveLocation=function(){try{return tmpSdbDC.setActiveLocation.apply(this,arguments);}catch(e){throw new Error(e);}};
SdbDC.prototype.toString=function(){try{return tmpSdbDC.toString.apply(this,arguments);}catch(e){throw new Error(e);}};
