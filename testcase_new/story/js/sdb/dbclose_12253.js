/******************************************************************************
*@Description : test db operation after close
*               TestLink : seqDB-12253 ����close��ִ�в���
*@auhor       : Liang XueWang
******************************************************************************/
main( test );

function test ()
{
   var db = new Sdb( COORDHOSTNAME, COORDSVCNAME );
   db.close();

   assert.tryThrow( [SDB_NOT_CONNECTED, SDB_INVALIDARG], function()
   {
      db.traceResume();
   } );
}

