var tmpSdbDataSource = {
   alter: SdbDataSource.prototype.alter,
   help: SdbDataSource.prototype.help,
   toString: SdbDataSource.prototype.toString
};
var funcSdbDataSource = SdbDataSource;
var funcSdbDataSourcehelp = SdbDataSource.help;
SdbDataSource=function(){try{return funcSdbDataSource.apply( this, arguments ); } catch( e ) { throw new Error(e) } };
SdbDataSource.help = function(){try{ return funcSdbDataSourcehelp.apply( this, arguments ); } catch( e ) { throw new Error(e) } };
SdbDataSource.prototype.alter=function(){try{return tmpSdbDataSource.alter.apply(this,arguments);}catch(e){throw new Error(e);}};
SdbDataSource.prototype.help=function(){try{return tmpSdbDataSource.help.apply(this,arguments);}catch(e){throw new Error(e);}};
SdbDataSource.prototype.toString=function(){try{return tmpSdbDataSource.toString.apply(this,arguments);}catch(e){throw new Error(e);}};
