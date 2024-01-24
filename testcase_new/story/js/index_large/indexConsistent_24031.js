/******************************************************************************
 * @Description   : seqDB-24031:直连coord创建cs
 * @Author        : Yi Pan
 * @CreateTime    : 2021.05.10
 * @LastEditTime  : 2021.09.23
 * @LastEditors   : liuli
 ******************************************************************************/
main( test );

function test ()
{
   var csName = "cs_24031_";
   var clName = "cl_24031";

   for( var i = 0; i < 10; i++ )
   {
      commDropCS( db, csName + i );
      commCreateCL( db, csName + i, clName );
   }

   var uniqueID = -1;
   for( var i = 0; i < 10; i++ )
   {
      // 快照查询uniqueID
      var cursor = db.snapshot( SDB_SNAP_COLLECTIONSPACES, { "Name": csName + i }, { "UniqueID": null } );
      cursor.next();
      var nextUniqueID = cursor.current().toObj().UniqueID;
      cursor.close();

      // 查询uniqueID是否比前一个大
      if( uniqueID == -1 )
      {
         uniqueID = nextUniqueID;
      } else
      {
         if( uniqueID >= nextUniqueID )
         {
            throw new Error( csName + i + " uniqueid=" + nextUniqueID + " not greater than" + csName + ( i - 1 ) + " uniqueid=" + uniqueID );
         }
         uniqueID = nextUniqueID;
      }
   }

   // 清理环境
   for( var i = 0; i < 10; i++ )
   {
      commDropCS( db, csName + i );
   }
}