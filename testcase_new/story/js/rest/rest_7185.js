/****************************************************
@description:   create collectionspace, normal case
                testlink cases: seqDB-7185 & 7188
@input:         run CURL command: 
                1 curl http://127.0.0.1:11814/ -d "create collectionspace&name=foo"
                2 curl http://127.0.0.1:11814/ -d "create collectionspace&name=FOO"
                3 curl http://127.0.0.1:11814/ -d "drop collectionspace&name=foo"
@expectation:   1/2/3 return errno:0
@modify list:
                2015-4-3 Ting YU init   2016-3-16 XiaoNi Huang init
****************************************************/
var csName = CHANGEDPREFIX + "_7185_7188";
var cs = "name=" + csName;


main( test );

function test ()
{
   commDropCS( db, csName, true, "drop cs in begin" );
   commDropCS( db, csName + 'FOO', true, "drop cs in begin" );
   createcsAndCheck();
   createcsByCapital();
   dropcsAndCheck();

}

function createcsAndCheck ()
{
   tryCatch( ["cmd=create collectionspace", cs], [0], "Failed to create cs by rest cmd!" );
   db.getCS( csName );
}

function createcsByCapital ()
{
   tryCatch( ["cmd=create collectionspace", "name=" + csName + "FOO"], [0], getFuncName() + "Error occurs" );
   db.getCS( csName + "FOO" );
   db.dropCS( csName + "FOO" );
}

function dropcsAndCheck ()
{
   tryCatch( ["cmd=drop collectionspace", cs], [0], "Fail to drop existed cs by rest cmd!" );
   try
   {
      db.getCS( csName );
      throw new Error( "need throw error" );
   }
   catch( e )
   {
      if( e.message != SDB_DMS_CS_NOTEXIST )
      {
         throw e;
      }
   }
}



