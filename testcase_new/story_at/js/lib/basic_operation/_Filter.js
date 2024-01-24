if ( tmp_Filter == undefined )
{
   var tmp_Filter = {
      _match: _Filter.prototype._match,
      help: _Filter.prototype.help,
      match: _Filter.prototype.match
   };
}
var func_Filter = ( func_Filter == undefined ) ? _Filter : func_Filter;
var func_Filterhelp = func_Filter.help;
_Filter=function(){try{return func_Filter.apply( this, arguments ); } catch( e ) { throw new Error(e) } };
_Filter.help = function(){try{ return func_Filterhelp.apply( this, arguments ); } catch( e ) { throw new Error(e) } };
_Filter.prototype._match=function(){try{return tmp_Filter._match.apply(this,arguments);}catch(e){throw new Error(e);}};
_Filter.prototype.help=function(){try{return tmp_Filter.help.apply(this,arguments);}catch(e){throw new Error(e);}};
_Filter.prototype.match=function(){try{return tmp_Filter.match.apply(this,arguments);}catch(e){throw new Error(e);}};
