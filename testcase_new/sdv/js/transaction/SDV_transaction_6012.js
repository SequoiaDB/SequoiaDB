/* *****************************************************************************
@discretion: ����Ψһ������������ͬ��¼
@author��2015-11-21 wuyan  Init
***************************************************************************** */
main();
function main ()
{
   try
   {
      var clName = CHANGEDPREFIX + "_transaction6012";

      var cl = commCreateCL( db, COMMCSNAME, clName, {}, true, true );
      commCreateIndex( cl, 'testIndex', { no: 1 }, { Unique: true }, false )
      transOperation( cl )

      //@ clean end
      commDropCL( db, COMMCSNAME, clName, false, false, "drop CL in the beginning" );
   }
   catch( e )
   {
      throw e;
   }
   finally
   {
      if( undefined !== db )
      {
         db.close();
      }
   }
}

function transOperation ( cl )
{
   var dataNum = 1000;
   var insert = new insertData( cl, dataNum );
   execTransaction( beginTrans, insert );
   checkResult( cl, true, insert );

   //insert  the same datas
   try
   {
      execTransaction( insert );
   }
   catch( e )   
   {
      if( e == "insertData.exec() unknown error expect: -38" )
      {
         // think right
      }
      else  
      {
         throw buildException( "execTransaction(insert)", e )
      }
   }

   //commit transaction after autoRollback 
   try
   {
      execTransaction( commitTrans );
   }
   catch( e )
   {
      if( e == "commitTrans() unknown error expect: -196" )
      {
         // think right
      }
      else
      {
         throw buildException( "execTransaction(commitTrans)", e )
      }
   }

   checkResult( cl, false, insert );
}

