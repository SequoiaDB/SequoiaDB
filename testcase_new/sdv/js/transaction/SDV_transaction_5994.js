/* *****************************************************************************
@discretion: ִ�����ݸ���/ɾ���������ύ����
@author��2015-11-18 wuyan  Init
***************************************************************************** */

main();
function main ()
{
   try
   {
      var clName = CHANGEDPREFIX + "_transaction5994";

      var cl = commCreateCL( db, COMMCSNAME, clName, {}, true, true );
      var dataNum = 1000;
      var insert = new insertData( cl, dataNum );
      //update data,then commmit transaction
      var update = new updateData( cl );
      execTransaction( insert, beginTrans, update, commitTrans );
      checkResult( cl, true, update );

      //remove data and left some datas,then commit transaction
      var removeNum = 88;
      var remove = new removeData( cl, removeNum );
      execTransaction( beginTrans, remove, commitTrans );
      checkResult( cl, true, remove );

      //@ clean end
      commDropCL( db, COMMCSNAME, clName, false, false, "drop CL in the beginning" );
   }
   catch( e )
   {
      throw e;
   }
}


