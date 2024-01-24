/* *****************************************************************************
@Description:  seqDB-9937:执行插入查询相关的存储过程
@modify list:
         
***************************************************************************** */
main( test );

function test ()
{
   if( commIsStandalone( db ) )
   {
      return;
   }
   var testCsName = CHANGEDPREFIX + "_procedures";

   // clean
   fmpCleanProcedures( db, CHANGEDPREFIX );

   // create procedures
   var nameArray = new Array( CHANGEDPREFIX + "_createCSAndCL", CHANGEDPREFIX + "_insertRecord", CHANGEDPREFIX + "_readData", CHANGEDPREFIX + "_dropCS" );
   var index = 0;

   eval( "db.createProcedure( function " + nameArray[index] + "(csName,clName) {" +
      "try {" +
      "db.dropCS( csName ) ;" +
      "}" +
      "catch(e) {" +
      "if( e!=-34 ) {" +
      "throw e ;" +
      "}" +
      "}" +
      "try {" +
      "var cs = db.createCS( csName ); " +
      "var cl = cs.createCL( clName, {ReplSize:0} ); " +
      "}" +
      "catch(e) {" +
      "throw e ;" +
      "}" +
      "return cl ;" +
      "} )"
   );
   ++index;

   eval( "db.createProcedure( function " + nameArray[index] + "(num, cl) {" +
      "cl.remove();" +
      "for( var i = 0; i < num ; i++ ) " +
      "{" +
      "try" +
      "{" +
      "cl.insert({_id:Math.random(),group:i%5,price:i,name:'test'+i } ) ;" +
      "}" +
      "catch(e)" +
      "{" +
      "throw e ;" +
      "}" +
      "}" +
      "} )"
   );
   ++index;

   eval( "db.createProcedure( function " + nameArray[index] + "(num, csName, clName) {" +
      "var cl = " + nameArray[0] + "( csName, clName ) ; " +
      nameArray[1] + "( num, cl ) ;" +
      "return cl.aggregate({$group:{_id:'$group'}});" +
      "} ) "
   );
   ++index;

   eval( "db.createProcedure( function " + nameArray[index] + "( csName ) {" +
      "try" +
      "{" +
      "db.dropCS( csName ) ;" +
      "}" +
      "catch(e)" +
      "{" +
      "if( e != SDB_DMS_CS_NOTEXIST )" +
      "{" +
      "throw e ;" +
      "}" +
      "}" +
      "} )"
   );

   // check procedures
   var rc = db.listProcedures( { $or: [{ name: nameArray[0] }, { name: nameArray[1] }, { name: nameArray[2] }, { name: nameArray[3] }] } );
   assert.equal( rc.size(), 4 );

   // run procedures
   var cursor = db.eval( nameArray[2] + "( 100, \"" + testCsName + "\", \"" + COMMCLNAME + "\" )" );
   cursor.close();
   db.eval( nameArray[3] + "(\"" + testCsName + "\")" );

   // remove procedures
   fmpRemoveProcedures( nameArray, false );

   // check procedures
   var rc = db.listProcedures( { $or: [{ name: nameArray[0] }, { name: nameArray[1] }, { name: nameArray[2] }, { name: nameArray[3] }] } );
   assert.equal( rc.size(), 0 );
}


