if ( tmpRemote == undefined )
{
   var tmpRemote = {
      __runCommand: Remote.prototype.__runCommand,
      _runCommand: Remote.prototype._runCommand,
      close: Remote.prototype.close,
      getCmd: Remote.prototype.getCmd,
      getFile: Remote.prototype.getFile,
      getInfo: Remote.prototype.getInfo,
      getIniFile: Remote.prototype.getIniFile,
      getSystem: Remote.prototype.getSystem,
      help: Remote.prototype.help,
      toString: Remote.prototype.toString
   };
}
var funcRemote = ( funcRemote == undefined ) ? Remote : funcRemote;
var funcRemotehelp = funcRemote.help;
Remote=function(){try{return funcRemote.apply( this, arguments ); } catch( e ) { throw new Error(e) } };
Remote.help = function(){try{ return funcRemotehelp.apply( this, arguments ); } catch( e ) { throw new Error(e) } };
Remote.prototype.__runCommand=function(){try{return tmpRemote.__runCommand.apply(this,arguments);}catch(e){throw new Error(e);}};
Remote.prototype._runCommand=function(){try{return tmpRemote._runCommand.apply(this,arguments);}catch(e){throw new Error(e);}};
Remote.prototype.close=function(){try{return tmpRemote.close.apply(this,arguments);}catch(e){throw new Error(e);}};
Remote.prototype.getCmd=function(){try{return tmpRemote.getCmd.apply(this,arguments);}catch(e){throw new Error(e);}};
Remote.prototype.getFile=function(){try{return tmpRemote.getFile.apply(this,arguments);}catch(e){throw new Error(e);}};
Remote.prototype.getInfo=function(){try{return tmpRemote.getInfo.apply(this,arguments);}catch(e){throw new Error(e);}};
Remote.prototype.getIniFile=function(){try{return tmpRemote.getIniFile.apply(this,arguments);}catch(e){throw new Error(e);}};
Remote.prototype.getSystem=function(){try{return tmpRemote.getSystem.apply(this,arguments);}catch(e){throw new Error(e);}};
Remote.prototype.help=function(){try{return tmpRemote.help.apply(this,arguments);}catch(e){throw new Error(e);}};
Remote.prototype.toString=function(){try{return tmpRemote.toString.apply(this,arguments);}catch(e){throw new Error(e);}};
