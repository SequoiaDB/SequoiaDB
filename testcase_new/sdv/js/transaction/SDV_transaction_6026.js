/************************************************************************
*@Description:	seqDB-6026:δ��������ִ�лع�_SD.transaction.037
*@Author:  		TingYU  2015/11/24
               wuyan 2017/1/6(ִ�лع�������)
************************************************************************/
main();

function main ()
{
   var csName = COMMCSNAME + "_yt6026";
   var clName = COMMCLNAME + "_yt6026";

   try
   {
      var cl = readyCL( csName, clName, { ReplSize: 0 } );

      //insert
      var dataNum = 100;
      var insert = new insertData( cl, dataNum );
      execTransaction( insert );
      checkResult( cl, true, insert );

      //rollback
      try
      {
         execTransaction( rollbackTrans );
      }
      catch( e )
      {
         throw e;
      }
      checkResult( cl, true, insert );

      clean( csName, clName );
   }
   catch( e )
   {
      throw e;
   }
}
