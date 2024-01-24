if ( tmpBinData == undefined )
{
   var tmpBinData = {
      help: BinData.prototype.help,
      toString: BinData.prototype.toString
   };
}
var funcBinData = ( funcBinData == undefined ) ? BinData : funcBinData;
var funcBinDatahelp = funcBinData.help;
BinData=function(){try{return funcBinData.apply( this, arguments ); } catch( e ) { throw new Error(e) } };
BinData.help = function(){try{ return funcBinDatahelp.apply( this, arguments ); } catch( e ) { throw new Error(e) } };
BinData.prototype.help=function(){try{return tmpBinData.help.apply(this,arguments);}catch(e){throw new Error(e);}};
BinData.prototype.toString=function(){try{return tmpBinData.toString.apply(this,arguments);}catch(e){throw new Error(e);}};
