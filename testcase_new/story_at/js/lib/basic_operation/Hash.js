if ( tmpHash == undefined )
{
   var tmpHash = {
      help: Hash.prototype.help
   };
}
var funcHash = ( funcHash == undefined ) ? Hash : funcHash;
var funcHashfileMD5 = funcHash.fileMD5;
var funcHashhelp = funcHash.help;
var funcHashmd5 = funcHash.md5;
Hash=function(){try{return funcHash.apply( this, arguments ); } catch( e ) { throw new Error(e) } };
Hash.fileMD5 = function(){try{ return funcHashfileMD5.apply( this, arguments ); } catch( e ) { throw new Error(e) } };
Hash.help = function(){try{ return funcHashhelp.apply( this, arguments ); } catch( e ) { throw new Error(e) } };
Hash.md5 = function(){try{ return funcHashmd5.apply( this, arguments ); } catch( e ) { throw new Error(e) } };
Hash.prototype.help=function(){try{return tmpHash.help.apply(this,arguments);}catch(e){throw new Error(e);}};
