if ( tmpUser == undefined )
{
   var tmpUser = {
      _promptPassword: User.prototype._promptPassword,
      getUsername: User.prototype.getUsername,
      help: User.prototype.help,
      promptPassword: User.prototype.promptPassword,
      toString: User.prototype.toString
   };
}
var funcUser = ( funcUser == undefined ) ? User : funcUser;
var funcUserhelp = funcUser.help;
User=function(){try{return funcUser.apply( this, arguments ); } catch( e ) { throw new Error(e) } };
User.help = function(){try{ return funcUserhelp.apply( this, arguments ); } catch( e ) { throw new Error(e) } };
User.prototype._promptPassword=function(){try{return tmpUser._promptPassword.apply(this,arguments);}catch(e){throw new Error(e);}};
User.prototype.getUsername=function(){try{return tmpUser.getUsername.apply(this,arguments);}catch(e){throw new Error(e);}};
User.prototype.help=function(){try{return tmpUser.help.apply(this,arguments);}catch(e){throw new Error(e);}};
User.prototype.promptPassword=function(){try{return tmpUser.promptPassword.apply(this,arguments);}catch(e){throw new Error(e);}};
User.prototype.toString=function(){try{return tmpUser.toString.apply(this,arguments);}catch(e){throw new Error(e);}};
