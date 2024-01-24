/************************************************************************
*@Description:	seqDB-6029:�ظ�ִ������ع�_SD.transaction.040
               ����ִ�п�������ɾ�����ع����ع�����
*@Author:  		TingYU  2015/11/24
               wuyan 2017/1/6(�޸��ظ�ִ�лع�������)
************************************************************************/
main();

function main ()
{
   var csName = COMMCSNAME + "_yt6029";
   var clName = COMMCLNAME + "_yt6029";

   try
   {
      var cl = readyCL( csName, clName, { ReplSize: 0 } );

      //begin and rollback
      var dataNum = 100;
      var insert = new insertData( cl, dataNum );
      var remove = new removeData( cl );
      execTransaction( insert, beginTrans, remove, rollbackTrans );
      checkResult( cl, false, remove );

      //rollback again
      try
      {
         execTransaction( rollbackTrans );
      }
      catch( e )
      {
         throw e;
      }
      checkResult( cl, false, remove );

      clean( csName, clName );
   }
   catch( e )
   {
      throw e;
   }
}
