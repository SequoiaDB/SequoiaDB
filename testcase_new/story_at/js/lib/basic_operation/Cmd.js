if ( tmpCmd == undefined )
{
   var tmpCmd = {
      _getCommand: Cmd.prototype._getCommand,
      _getInfo: Cmd.prototype._getInfo,
      _getLastOut: Cmd.prototype._getLastOut,
      _getLastRet: Cmd.prototype._getLastRet,
      _run: Cmd.prototype._run,
      _start: Cmd.prototype._start,
      getCommand: Cmd.prototype.getCommand,
      getInfo: Cmd.prototype.getInfo,
      getLastOut: Cmd.prototype.getLastOut,
      getLastRet: Cmd.prototype.getLastRet,
      help: Cmd.prototype.help,
      run: Cmd.prototype.run,
      runJS: Cmd.prototype.runJS,
      start: Cmd.prototype.start,
      toString: Cmd.prototype.toString
   };
}
var funcCmd = ( funcCmd == undefined ) ? Cmd : funcCmd;
var funcCmdhelp = funcCmd.help;
Cmd=function(){try{return funcCmd.apply( this, arguments ); } catch( e ) { throw new Error(e) } };
Cmd.help = function(){try{ return funcCmdhelp.apply( this, arguments ); } catch( e ) { throw new Error(e) } };
Cmd.prototype._getCommand=function(){try{return tmpCmd._getCommand.apply(this,arguments);}catch(e){throw new Error(e);}};
Cmd.prototype._getInfo=function(){try{return tmpCmd._getInfo.apply(this,arguments);}catch(e){throw new Error(e);}};
Cmd.prototype._getLastOut=function(){try{return tmpCmd._getLastOut.apply(this,arguments);}catch(e){throw new Error(e);}};
Cmd.prototype._getLastRet=function(){try{return tmpCmd._getLastRet.apply(this,arguments);}catch(e){throw new Error(e);}};
Cmd.prototype._run=function(){try{return tmpCmd._run.apply(this,arguments);}catch(e){throw new Error(e);}};
Cmd.prototype._start=function(){try{return tmpCmd._start.apply(this,arguments);}catch(e){throw new Error(e);}};
Cmd.prototype.getCommand=function(){try{return tmpCmd.getCommand.apply(this,arguments);}catch(e){throw new Error(e);}};
Cmd.prototype.getInfo=function(){try{return tmpCmd.getInfo.apply(this,arguments);}catch(e){throw new Error(e);}};
Cmd.prototype.getLastOut=function(){try{return tmpCmd.getLastOut.apply(this,arguments);}catch(e){throw new Error(e);}};
Cmd.prototype.getLastRet=function(){try{return tmpCmd.getLastRet.apply(this,arguments);}catch(e){throw new Error(e);}};
Cmd.prototype.help=function(){try{return tmpCmd.help.apply(this,arguments);}catch(e){throw new Error(e);}};
Cmd.prototype.run=function(){try{return tmpCmd.run.apply(this,arguments);}catch(e){throw new Error(e);}};
Cmd.prototype.runJS=function(){try{return tmpCmd.runJS.apply(this,arguments);}catch(e){throw new Error(e);}};
Cmd.prototype.start=function(){try{return tmpCmd.start.apply(this,arguments);}catch(e){throw new Error(e);}};
Cmd.prototype.toString=function(){try{return tmpCmd.toString.apply(this,arguments);}catch(e){throw new Error(e);}};
