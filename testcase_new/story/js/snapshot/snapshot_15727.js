/******************************************************************************
*@Description : test snapshot SDB_SNAP_CONFIGS 
*               TestLink : seqDB-15727:ָ�����ղ�ѯ������ѯ������Ϣ�����������
*@auhor       : CSQ 
******************************************************************************/

main( test );

function test ()
{
   var groups = commGetGroups( db );
   if( groups.length < 2 )
   {
      return;
   }
   var groupName = groups[0][0].GroupName;
   var groupNodeNum = groups[0].length - 1;

   //cond+sel+skip+limit
   var sel = { "role": 1 };
   var skip = groupNodeNum - 1;
   var limit = groupNodeNum;
   var cond = { "$and": [{ "GroupName": groupName }, { "role": "data" }] };
   var cursor = db.snapshot( SDB_SNAP_CONFIGS, new SdbSnapshotOption().cond( cond ).sel( sel ).skip( skip ).limit( limit ) );

   var actResult = [];
   var expResult = [{ "role": "data" }];
   while( cursor.next() )
   {
      actResult.push( cursor.current().toObj() );
   }
   checkResult( actResult, expResult );

   //Specify all parameters
   skip = 0;
   limit = 1;
   var count = 0;
   var options = { "expand": false };
   sel = { "archiveon": 1, "role": 1 };
   cond = { "$and": [{ "GroupName": groupName }, { "role": "data" }] };
   cursor = db.snapshot( SDB_SNAP_CONFIGS, new SdbSnapshotOption().cond( cond ).sel( sel ).skip( skip ).limit( limit ).options( options ) );
   var actResult = [];
   expResult = [{ "role": "data", "archiveon": 1 }];
   while( cursor.next() )
   {
      actResult.push( cursor.current().toObj() );
   }
   checkResult( actResult, expResult );

   //cond+sel+options+limit
   count = 0;
   limit = 1;
   sel = { "role": 1, "archiveon": 1 };
   var options = { "expand": false };
   cond = { $and: [{ GroupName: groupName }, { role: "data" }] };
   cursor = db.snapshot( SDB_SNAP_CONFIGS, new SdbSnapshotOption().cond( cond ).sel( sel ).options( options ).limit( limit ) )
   var actResult = [];
   expResult = [{ "role": "data" }];
   while( cursor.next() )
   {
      actResult.push( cursor.current().toObj() );
      count++;
   }
   if( count !== 1 )
   {
      throw new Error( "count: " + count );
   }
}
