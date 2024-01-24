if ( tmpBSONObj == undefined )
{
   var tmpBSONObj = {
      help: BSONObj.prototype.help,
      toJson: BSONObj.prototype.toJson,
      toObj: BSONObj.prototype.toObj,
      toString: BSONObj.prototype.toString
   };
}
var funcBSONObj = ( funcBSONObj == undefined ) ? BSONObj : funcBSONObj;
var funcBSONObjhelp = funcBSONObj.help;
BSONObj=function(){try{return funcBSONObj.apply( this, arguments ); } catch( e ) { throw new Error(e) } };
BSONObj.help = function(){try{ return funcBSONObjhelp.apply( this, arguments ); } catch( e ) { throw new Error(e) } };
BSONObj.prototype.help=function(){try{return tmpBSONObj.help.apply(this,arguments);}catch(e){throw new Error(e);}};
BSONObj.prototype.toJson=function(){try{return tmpBSONObj.toJson.apply(this,arguments);}catch(e){throw new Error(e);}};
BSONObj.prototype.toObj=function(){try{return tmpBSONObj.toObj.apply(this,arguments);}catch(e){throw new Error(e);}};
BSONObj.prototype.toString=function(){try{return tmpBSONObj.toString.apply(this,arguments);}catch(e){throw new Error(e);}};
