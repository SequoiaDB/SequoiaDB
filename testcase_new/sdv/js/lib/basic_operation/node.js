/* *****************************************************************************
@discription: return object of Node
@author: yinzhen
@parameter:
   node: primitive SdbNode object
***************************************************************************** */
function Node ( node )
{
   this.toString =
      function()
      {
         try
         {
            var str = node.toString();
         }
         catch( e )
         {
            throw new Error( e );
         }
         return str;
      }

   this.getNodeDetail =
      function()
      {
         try
         {
            var obj = node.getNodeDetail();
         }
         catch( e )
         {
            throw new Error( e );
         }
         return obj;
      }

   this.connect =
      function()
      {
         try
         {
            var conn = node.connect();
         }
         catch( e )
         {
            throw new Error( e );
         }
         return new Sequoiadb( conn.toString() );
      }

   this.getHostName =
      function()
      {
         try
         {
            var str = node.getHostName();
         }
         catch( e )
         {
            throw new Error( e );
         }
         return str;
      }

   this.getServiceName =
      function()
      {
         try
         {
            var str = node.getServiceName();
         }
         catch( e )
         {
            throw new Error( e );
         }
         return str;
      }

   this.start =
      function()
      {
         try
         {
            node.start();
         }
         catch( e )
         {
            throw new Error( e );
         }
      }

   this.stop =
      function()
      {
         try
         {
            node.stop();
         }
         catch( e )
         {
            throw new Error( e );
         }
      }
}
