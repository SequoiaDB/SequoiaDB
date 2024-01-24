if ( tmpSdbRecycleBin == undefined )
{
   var tmpSdbRecycleBin = {
      alter: SdbRecycleBin.prototype.alter,
      count: SdbRecycleBin.prototype.count,
      disable: SdbRecycleBin.prototype.disable,
      dropAll: SdbRecycleBin.prototype.dropAll,
      dropItem: SdbRecycleBin.prototype.dropItem,
      enable: SdbRecycleBin.prototype.enable,
      getDetail: SdbRecycleBin.prototype.getDetail,
      help: SdbRecycleBin.prototype.help,
      list: SdbRecycleBin.prototype.list,
      returnItem: SdbRecycleBin.prototype.returnItem,
      returnItemToName: SdbRecycleBin.prototype.returnItemToName,
      setAttributes: SdbRecycleBin.prototype.setAttributes,
      snapshot: SdbRecycleBin.prototype.snapshot
   };
}
var funcSdbRecycleBin = ( funcSdbRecycleBin == undefined ) ? SdbRecycleBin : funcSdbRecycleBin;
var funcSdbRecycleBinhelp = funcSdbRecycleBin.help;
SdbRecycleBin=function(){try{return funcSdbRecycleBin.apply( this, arguments ); } catch( e ) { throw new Error(e) } };
SdbRecycleBin.help = function(){try{ return funcSdbRecycleBinhelp.apply( this, arguments ); } catch( e ) { throw new Error(e) } };
SdbRecycleBin.prototype.alter=function(){try{return tmpSdbRecycleBin.alter.apply(this,arguments);}catch(e){throw new Error(e);}};
SdbRecycleBin.prototype.count=function(){try{return tmpSdbRecycleBin.count.apply(this,arguments);}catch(e){throw new Error(e);}};
SdbRecycleBin.prototype.disable=function(){try{return tmpSdbRecycleBin.disable.apply(this,arguments);}catch(e){throw new Error(e);}};
SdbRecycleBin.prototype.dropAll=function(){try{return tmpSdbRecycleBin.dropAll.apply(this,arguments);}catch(e){throw new Error(e);}};
SdbRecycleBin.prototype.dropItem=function(){try{return tmpSdbRecycleBin.dropItem.apply(this,arguments);}catch(e){throw new Error(e);}};
SdbRecycleBin.prototype.enable=function(){try{return tmpSdbRecycleBin.enable.apply(this,arguments);}catch(e){throw new Error(e);}};
SdbRecycleBin.prototype.getDetail=function(){try{return tmpSdbRecycleBin.getDetail.apply(this,arguments);}catch(e){throw new Error(e);}};
SdbRecycleBin.prototype.help=function(){try{return tmpSdbRecycleBin.help.apply(this,arguments);}catch(e){throw new Error(e);}};
SdbRecycleBin.prototype.list=function(){try{return tmpSdbRecycleBin.list.apply(this,arguments);}catch(e){throw new Error(e);}};
SdbRecycleBin.prototype.returnItem=function(){try{return tmpSdbRecycleBin.returnItem.apply(this,arguments);}catch(e){throw new Error(e);}};
SdbRecycleBin.prototype.returnItemToName=function(){try{return tmpSdbRecycleBin.returnItemToName.apply(this,arguments);}catch(e){throw new Error(e);}};
SdbRecycleBin.prototype.setAttributes=function(){try{return tmpSdbRecycleBin.setAttributes.apply(this,arguments);}catch(e){throw new Error(e);}};
SdbRecycleBin.prototype.snapshot=function(){try{return tmpSdbRecycleBin.snapshot.apply(this,arguments);}catch(e){throw new Error(e);}};
