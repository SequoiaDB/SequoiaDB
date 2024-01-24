var tmpSdbNode = {
   connect: SdbNode.prototype.connect,
   getDetailObj: SdbNode.prototype.getDetailObj,
   getHostName: SdbNode.prototype.getHostName,
   getNodeDetail: SdbNode.prototype.getNodeDetail,
   getServiceName: SdbNode.prototype.getServiceName,
   help: SdbNode.prototype.help,
   start: SdbNode.prototype.start,
   stop: SdbNode.prototype.stop,
   toString: SdbNode.prototype.toString
};
var funcSdbNode = SdbNode;
var funcSdbNodehelp = SdbNode.help;
SdbNode=function(){try{return funcSdbNode.apply( this, arguments ); } catch( e ) { throw new Error(e) } };
SdbNode.help = function(){try{ return funcSdbNodehelp.apply( this, arguments ); } catch( e ) { throw new Error(e) } };
SdbNode.prototype.connect=function(){try{return tmpSdbNode.connect.apply(this,arguments);}catch(e){throw new Error(e);}};
SdbNode.prototype.getDetailObj=function(){try{return tmpSdbNode.getDetailObj.apply(this,arguments);}catch(e){throw new Error(e);}};
SdbNode.prototype.getHostName=function(){try{return tmpSdbNode.getHostName.apply(this,arguments);}catch(e){throw new Error(e);}};
SdbNode.prototype.getNodeDetail=function(){try{return tmpSdbNode.getNodeDetail.apply(this,arguments);}catch(e){throw new Error(e);}};
SdbNode.prototype.getServiceName=function(){try{return tmpSdbNode.getServiceName.apply(this,arguments);}catch(e){throw new Error(e);}};
SdbNode.prototype.help=function(){try{return tmpSdbNode.help.apply(this,arguments);}catch(e){throw new Error(e);}};
SdbNode.prototype.start=function(){try{return tmpSdbNode.start.apply(this,arguments);}catch(e){throw new Error(e);}};
SdbNode.prototype.stop=function(){try{return tmpSdbNode.stop.apply(this,arguments);}catch(e){throw new Error(e);}};
SdbNode.prototype.toString=function(){try{return tmpSdbNode.toString.apply(this,arguments);}catch(e){throw new Error(e);}};
