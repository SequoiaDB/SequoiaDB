
/*
   文件路径
*/
var SdbTestcaseConf = function( file ){
   this.file = new File( file ) ;
   this.isParse = false ;
   this.config = {
      "Host": []
   } ;
} ;

SdbTestcaseConf.prototype = {} ;
SdbTestcaseConf.prototype.constructor = SdbTestcaseConf;

SdbTestcaseConf.prototype._parse = function(){
   if( this.isParse == false )
   {
      var content = "" ;
      this.isParse = true ;

      try
      {
         content = this.file.read( 1024 ) ;
      }
      catch( e )
      {
         println( "Warning: failed to read testcase config: " + e ) ;
         return ;
      }

      try
      {
         this.config = JSON.parse( content ) ;
      }
      catch( e )
      {
         println( "Warning: failed to read testcase config: " + e ) ;
         return ;
      }
   }
}

SdbTestcaseConf.prototype.getHostList = function(){
   this._parse() ;
   return this.config['Host'] ;
}