/* *****************************************************************************
@discretion: ���񲻴���ִ���ύ
@author��2015-11-23 wuyan  Init
***************************************************************************** */
main();
function main ()
{
   var clName = CHANGEDPREFIX + "_transaction6020";

   var cl = commCreateCL( db, COMMCSNAME, clName, {}, true, true );
   //commit transaction not exec beginTrans 
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
   //@ clean end
   commDropCL( db, COMMCSNAME, clName, false, false, "drop CL in the beginning" );
}



