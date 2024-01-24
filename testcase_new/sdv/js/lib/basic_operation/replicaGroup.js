/* *****************************************************************************
@discription: return object of ReplicaGroup
@author: yinzhen
@parameter:
   group: primitive SdbReplicaGroup object
***************************************************************************** */
function ReplicaGroup ( group )
{
   this.getMaster =
      function()
      {
         try
         {
            var node = group.getMaster();
         }
         catch( e )
         {
            throw new Error( e );
         }
         return new Node( node );
      }

   this.getDetail =
      function()
      {
         try
         {
            var obj = group.getDetail();
         }
         catch( e )
         {
            throw new Error( e );
         }
         return obj;
      }

   this.getSlave =
      function( positions )
      {
         try
         {
            if( positions === undefined )
            {
               var node = group.getSlave();
            }
            else
            {
               var node = group.getSlave( positions );
            }
         }
         catch( e )
         {
            throw new Error( e );
         }
         return new Node( node );
      }

   this.reelect =
      function( options )
      {
         if( options === undefined ) { options = {}; }
         try
         {
            group.reelect( options );
         }
         catch( e )
         {
            throw new Error( e );
         }
      }
}
