/************************************************************************
*@Description:	ִ�и���+�ύ���������ִ��ɾ��+�ع��������
               seqDB-6021:ִ������ع�_SD.transaction.032
*@Author:  		TingYU  2015/11/23
************************************************************************/
main();
function main ()
{
   try
   {
      var csName = COMMCSNAME + "_yt6021";
      var clName = COMMCLNAME + "_yt6021";

      var cl = readyCL( csName, clName, { ReplSize: 0 } );

      var dataNum = 100;
      var insert = new insertData( cl, dataNum );
      var update = new updateData( cl );
      execTransaction( insert, beginTrans, update, commitTrans );
      checkResult( cl, true, update );

      var remove = new removeData( cl );
      execTransaction( beginTrans, remove );
      checkResult( cl, true, remove );

      execTransaction( rollbackTrans );
      checkResult( cl, false, remove );

      clean( csName, clName );
   }
   catch( e )
   {
      throw e;
   }
}
