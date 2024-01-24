/****************************************************
@description:   create/drop cs, abnormal case
                testlink cases: seqDB-7186 & 7187 & 7189
@expectation:   1 twoName(): "XXXX&nam=foo&name=foo1" expect: create foo, dont create foo1
                2 lackCmd(): expect: return -100
                3 lackName(): expect: return -6
                4 lackCmdAndName(): expect: return 0
                5 misspell(): cmd misspell expect: return -100
                  name misspell expect: return -6
                6 formatError1(): "XXXX&name=foo=foo" expect: create foo=foo
                7 formatError2(): "XXXX&name&b=foo" expect: return -6
                8 formatError3(): "XXXX&name==foo" expect: create =foo
                9 formatError4(): "XXXX&&&name=foo" expect: create foo
                10 lackNameDropCS(): expect: return -6
                   note: [1-9] cases are for create cs, [10] case is for drop cs.
@modify list:
                2015-3-30 Ting YU init   2016-3-16 XiaoNi Huang init
****************************************************/
var csName = CHANGEDPREFIX + "_7186";
var cs = "name=" + csName;

main( test );

function test ()
{
   commDropCS( db, csName, true, "drop cs in begin" );
   commDropCS( db, csName + '1', true, "drop cs1 in begin" );
   commDropCS( db, csName + '=' + cs, true, "drop cs in begin" );
   commDropCS( db, "name==" + cs, true, "drop cs1 in begin" );

   twoName();
   lackCmd();
   lackName();
   lackCmdAndName();
   misspell();
   formatError1();
   formatError2();
   formatError3();
   formatError4();
   lackNameDropCS();

   commDropCS( db, csName );
   commDropCS( db, csName + '1' );
   commDropCS( db, csName + '=' + cs );
   commDropCS( db, "name==" + cs );
}


function twoName ()
{
   //"XXXX&nam=foo&name=foo1" 
   var cs1Name = csName + '_1';
   var cs1 = "name=" + cs1Name;

   var word = "create collectionspace";
   tryCatch( ["cmd=" + word, cs, cs1], [0], getFuncName() + "Failed to run rest cmd=" + word );

   try
   {
      db.getCS( cs1Name );
      throw new Error( "need throw error" );
   }
   catch( e )
   {
      if( e.message != SDB_DMS_CS_NOTEXIST )
      {
         throw e;
      }
   }
   db.dropCS( csName );
}

function lackCmd ()
{
   tryCatch( [cs], [-100], getFuncName() + "Error occurs" );
}

function lackName ()
{
   tryCatch( ["cmd=create collectionspace"], [-6], getFuncName() + "Error occurs" );
}

function lackCmdAndName ()
{
   tryCatch( [""], [-100], getFuncName() + "Error occurs" );
}

function misspell ()
{
   var word = 'command=create collectionspace';
   tryCatch( [word, cs], [-100], "Error occurs in misspell function, " + word );

   word = 'cmd=create collectionspaces';
   tryCatch( [word, cs], [-100], "Error occurs in misspell function, " + word );

   word = 'cmd=create collectionspace';
   var csMisspell = "namename=" + csName;
   tryCatch( [word, csMisspell], [-6], "Error occurs in misspell function, " + csMisspell );
}

function formatError1 ()
{  //"XXXX&a=1=2&XXXX"   a value is 1=2
   var word = "create collectionspace";
   var csNameFormat1 = csName + '=' + csName;
   var csFormat1 = "name=" + csNameFormat1;
   tryCatch( ["cmd=" + word, csFormat1], [0], "Fai to create cs by rest " + csNameFormat1 );
   db.getCS( csNameFormat1 );
   db.dropCS( csNameFormat1 );
}

function formatError2 ()
{  //"XXXX&a&b=1"   can't get a value
   var word = "create collectionspace";
   tryCatch( ["cmd=" + word, 'name', 'b=' + csName], [-6], getFuncName() + "error occurs!!" );
}

function formatError3 ()
{  //"XXXX&a==1&XXXX"   a value is =1
   var word = "create collectionspace";
   var csNameFormat3 = '=' + csName;
   tryCatch( ["cmd=" + word, 'name=' + csNameFormat3], [0], getFuncName() + "error occurs!!" );
   db.getCS( csNameFormat3 );
   db.dropCS( csNameFormat3 );
}

function formatError4 ()
{  //"XXXX&&&a=1"   a value is 1
   var word = "create collectionspace";
   tryCatch( ["cmd=" + word, '', '', 'name=' + csName], [0], getFuncName() + "Fai to create cs by rest: &&&name=" + csName );
   db.getCS( csName );
   db.dropCS( csName );
}

function lackNameDropCS ()
{
   tryCatch( ["cmd=drop collectionspace"], [-6], "Error occurs in lackNameDropCL function" );
}



