var tmpSdbtool = {
   help: Sdbtool.prototype.help
};
var funcSdbtool = Sdbtool;
var funcSdbtoolhelp = Sdbtool.help;
var funcSdbtoollistNodes = Sdbtool.listNodes;
Sdbtool=function(){try{return funcSdbtool.apply( this, arguments ); } catch( e ) { throw new Error(e) } };
Sdbtool.help = function(){try{ return funcSdbtoolhelp.apply( this, arguments ); } catch( e ) { throw new Error(e) } };
Sdbtool.listNodes = function(){try{ return funcSdbtoollistNodes.apply( this, arguments ); } catch( e ) { throw new Error(e) } };
Sdbtool.prototype.help=function(){try{return tmpSdbtool.help.apply(this,arguments);}catch(e){throw new Error(e);}};
