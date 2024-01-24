/******************************************************************************
*@Description : test ʹ��SdbOptionBase��ѯ������Ϣ
*               TestLink : seqDB-15729
*@auhor       : CSQ 
******************************************************************************/

main( test );

function test ()
{
   var cond = { "role": "data" };
   var sel = { "role": 1, "svcname": 1 };
   var sort = { "svcname": 1 };
   var hint = { "$Options": { "expand": false } };
   var option = new SdbOptionBase().cond( cond ).sel( sel ).sort( sort ).hint( hint ).limit( 5 ).skip( 0 ).flags( 1 );
   var cursor = db.snapshot( SDB_SNAP_CONFIGS, option );
   var count = 0;
   var tmp = 0;
   while( cursor.next() )
   {
      var obj = cursor.current().toObj();
      if( obj.svcname < tmp )
      {
         throw new Error( "SORT ERROE!" );
      }
      tmp = obj.svcname;
      count++;
   }
   if( count <= 0 )
   {
      throw new Error( "Expect count <=0, but act count is " + count );
   }
}
