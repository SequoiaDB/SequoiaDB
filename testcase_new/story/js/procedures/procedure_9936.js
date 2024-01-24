/* *****************************************************************************
@Description:   seqDB-9936:执行索引相关的存储过程
@modify list:
         
***************************************************************************** */
testConf.skipStandAlone = true;
var csName = CHANGEDPREFIX + "_9936";
var clName = CHANGEDPREFIX + "_9936";
main( test );

function test ()
{
   var procName = new Array( CHANGEDPREFIX + "_createCS_CL", CHANGEDPREFIX + "_createIndex",
      CHANGEDPREFIX + "_getIndex", CHANGEDPREFIX + "_dropIndex" );
   var idxName = CHANGEDPREFIX + "_procIdx_9936";
   var idxKey = "idNum";
   fmpRemoveProcedures( procName, true );
   var index = 0;
   eval( "db.createProcedure( function " + procName[index] + "(csName,clName){" +
      "try" +
      "{" +
      "   db.dropCS(csName) ;" +
      "}" +
      "catch ( e )" +
      "{" +
      "   if (-34 !=e )" +
      "   {" +
      "      throw e ;" +
      "   }" +
      "}" +
      "try" +
      "{" +
      "   var cs = db.createCS(csName) ;" +
      "   var cl = cs.createCL(clName, {ReplSize:0} ) ;" +
      "}" +
      "catch ( e )" +
      "{" +
      "   throw e ;" +
      "}" +
      "return cl ;" +
      "}" +
      ")"
   );

   ++index;
   eval( "db.createProcedure(function " + procName[index] +
      "(csName, clName, idxName, idxKey ){" +
      "var cl = " + procName[0] + "( csName, clName ) ;" +
      "try" +
      "{" +
      "   cl.dropIndex( idxName ) ;" +
      "}" +
      "catch ( e )" +
      "{" +
      "   if ( SDB_IXM_NOTEXIST != e )" +
      "   {" +
      "      throw e ;" +
      "   }" +
      "}" +
      "try" +
      "{" +
      "   cl.createIndex( idxName, { idxKey : -1 } ) ;" +
      "}" +
      "catch ( e )" +
      "{" +
      "   throw e ;" +
      "}" +
      "})"
   );

   ++index;
   eval( "db.createProcedure(function " + procName[index] +
      "( csName, clName, idxName, idxKey ){" +
      "try" +
      "{" +
      "   var cl = " + procName[0] + "( csName, clName ) ;" +
      procName[1] + "( csName, clName, idxName, idxKey) ;" +
      "   var cnt = 0 ;" +
      "   var noSync = false ;" +
      "   do{ try {" +
      "      cl.insert( {\"note\":\"make sure all nodes have cs\"} ) ;" +
      "      var getIdx = cl.getIndex( idxName ) ;" +
      "      if( undefined == getIdx )" +
      "         noSync = true ;" +
      "      else" +
      "         noSync = false ;" +
      "   }catch(e){" +
      "      if( -23 == e || -34 == e )" +
      "      {" +
      "         ++cnt ;" +
      "         noSync = true ;" +
      "         if( cnt >= 1000 )" +
      "            break ;" +
      "      }" +
      "      else" +
      "      {" +
      "         throw e ;" +
      "         break ;" +
      "      }" +
      "   }}while( true == noSync ) ;" +
      "   cl.getIndex( idxName ) ;" +
      "}" +
      "catch ( e )" +
      "{" +
      "   throw e ;" +
      "}" +
      "})"
   );

   ++index;
   eval( "db.createProcedure(function " + procName[index] +
      "( csName, clName, idxName ){" +
      "try" +
      "{" +
      "   var cl = " + procName[0] + "( csName, clName ) ;" +
      "   cl.dropIndex( idxName ) ;" +
      "}" +
      "catch ( e )" +
      "{" +
      "   if (  -34!=e )" +
      "   {" +
      "      throw e ;" +
      "   }" +
      "}" +
      "})"
   );

   db.listProcedures();

   // list create index procedure
   db.listProcedures( { name: procName[1] } );

   // list create cs and cl procedure
   db.listProcedures( { name: procName[0] } );
   // run create index procedure
   index = 1; // createIndex
   db.eval( procName[index] + "(\"" + csName + "\", \"" + clName + "\", \"" + idxName + "\", \"" + idxKey + "\")" );

   index = 2; // get index
   db.eval( procName[index] + "(\"" + csName + "\",\"" + clName + "\",\"" + idxName + "\", \"" + idxKey + "\")" );
   commDropCL( db, csName, clName, false, false );

   index = 2; // get index
   db.removeProcedure( procName[index] );

   index = 3; // drop index
   db.removeProcedure( procName[index] );
   fmpRemoveProcedures( procName, true );
}
