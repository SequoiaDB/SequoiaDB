/* *****************************************************************************
@discription: return new Cmd -> new Command
@author: yinzhen
@parameter
   NA
@e.g.
   var cmd = new Command();
   var cmd = new RemoteObj(hostName, svcName);
      var cmd = remoteObj.getCmd();
***************************************************************************** */
function Command ()
{
   try
   {
      var cmd = new Cmd();
   }
   catch( e )
   {
      throw new Error( e );
   }

   this.getCommand =
      function()
      {
         try
         {
            var str = cmd.getCommand();
         }
         catch( e )
         {
            throw new Error( e );
         }
         return str;
      }

   this.getInfo =
      function()
      {
         try
         {
            var obj = cmd.getInfo();
         }
         catch( e )
         {
            throw new Error( e );
         }
         return obj;
      }

   this.getLastOut =
      function()
      {
         try
         {
            var str = cmd.getLastOut();
         }
         catch( e )
         {
            throw new Error( e );
         }
         return str;
      }

   this.getLastRet =
      function()
      {
         try
         {
            var code = cmd.getLastRet();
         }
         catch( e )
         {
            throw new Error( e );
         }
         return code;
      }

   this.run =
      function( cmdStr, args, timeout, useShell )
      {
         if( args === undefined ) { args = ""; }
         if( timeout === undefined ) { timeout = 0; }
         if( useShell === undefined ) { useShell = 1; }
         try
         {
            var str = cmd.run( cmdStr, args, timeout, useShell );
         }
         catch( e )
         {
            throw new Error( e );
         }
         return str;
      }

   this.runJS =
      function( code )
      {
         try
         {
            var str = cmd.run( code );
         }
         catch( e )
         {
            throw new Error( e );
         }
         return str;
      }

   this.start =
      function( cmdStr, args, timeout, useShell )
      {
         if( args === undefined ) { args = ""; }
         if( timeout === undefined ) { timeout = 0; }
         if( useShell === undefined ) { useShell = 1; }
         try
         {
            var thID = cmd.start( cmdStr, args, timeout, useShell );
         }
         catch( e )
         {
            throw new Error( e );
         }
         return thID;
      }
}
