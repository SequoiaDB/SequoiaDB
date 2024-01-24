if ( tmpBSONArray == undefined )
{
   var tmpBSONArray = {
      _formatStr: BSONArray.prototype._formatStr,
      arrayAccess: BSONArray.prototype.arrayAccess,
      help: BSONArray.prototype.help,
      index: BSONArray.prototype.index,
      more: BSONArray.prototype.more,
      next: BSONArray.prototype.next,
      pos: BSONArray.prototype.pos,
      size: BSONArray.prototype.size,
      toArray: BSONArray.prototype.toArray,
      toString: BSONArray.prototype.toString
   };
}
var funcBSONArray = ( funcBSONArray == undefined ) ? BSONArray : funcBSONArray;
var funcBSONArrayhelp = funcBSONArray.help;
BSONArray=function(){try{return funcBSONArray.apply( this, arguments ); } catch( e ) { throw new Error(e) } };
BSONArray.help = function(){try{ return funcBSONArrayhelp.apply( this, arguments ); } catch( e ) { throw new Error(e) } };
BSONArray.prototype._formatStr=function(){try{return tmpBSONArray._formatStr.apply(this,arguments);}catch(e){throw new Error(e);}};
BSONArray.prototype.arrayAccess=function(){try{return tmpBSONArray.arrayAccess.apply(this,arguments);}catch(e){throw new Error(e);}};
BSONArray.prototype.help=function(){try{return tmpBSONArray.help.apply(this,arguments);}catch(e){throw new Error(e);}};
BSONArray.prototype.index=function(){try{return tmpBSONArray.index.apply(this,arguments);}catch(e){throw new Error(e);}};
BSONArray.prototype.more=function(){try{return tmpBSONArray.more.apply(this,arguments);}catch(e){throw new Error(e);}};
BSONArray.prototype.next=function(){try{return tmpBSONArray.next.apply(this,arguments);}catch(e){throw new Error(e);}};
BSONArray.prototype.pos=function(){try{return tmpBSONArray.pos.apply(this,arguments);}catch(e){throw new Error(e);}};
BSONArray.prototype.size=function(){try{return tmpBSONArray.size.apply(this,arguments);}catch(e){throw new Error(e);}};
BSONArray.prototype.toArray=function(){try{return tmpBSONArray.toArray.apply(this,arguments);}catch(e){throw new Error(e);}};
BSONArray.prototype.toString=function(){try{return tmpBSONArray.toString.apply(this,arguments);}catch(e){throw new Error(e);}};
