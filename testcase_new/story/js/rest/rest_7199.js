/****************************************************
@description:   insert/delete/update/query, abnormal case
                testlink cases: seqDB-7199 & 7202 & 7205 & 7214
@expectation:   1 lackInInsert(): lack [name/insertor] expect: return -6
                2 lackInUpdate(): lack [name/updator] expect: return -6
                3 lackInQuery(): lack [cmd/name] expect: return -100
                4 lackInDelete(): lack [name] expect: return -6
@modify list:
                2015-4-2 Ting YU init   2016-3-16 XiaoNi Huang init
****************************************************/
var csName = COMMCSNAME;
var clName = COMMCLNAME + "_7199_7202_7205_7214";
var cl = "name=" + csName + '.' + clName;

main( test );

function test ()
{
   commDropCL( db, csName, clName, true, true, "drop cl in begin" );
   var opt = { ReplSize: 0 };
   var varCL = commCreateCL( db, csName, clName, opt, true, false, "create cl in begin" );

   lackInInsert();
   lackInUpdate();
   lackInQuery();
   lackInDelete();

   commDropCL( db, csName, clName, false, false, "drop cl in clean" )
}

function lackInInsert ()
{
   var word = 'insert';
   //lack of name
   tryCatch( ["cmd=" + word, 'filter={"name":"Tom"}'], [-6], "Error occurs in lackInInsert function; lack of name" );
   //lack of insertor
   tryCatch( ["cmd=" + word, cl], [-6], "Error occurs in lackInInsert function; lack of insertor" );
   //invaild parameter
   tryCatch( ["cmd=" + word, cl, 'filter={"name":"Tom"},'], [-6], "Error,Failed to insert, invaild parameter" );
}

function lackInUpdate ()
{
   var word = 'update';
   //lack of name
   tryCatch( ["cmd=" + word, 'filter={"name":"Tom"}'], [-6], "Error occurs in lackInUpdate function; lack of name" );
   //lack of updator
   tryCatch( ["cmd=" + word, cl, 'filter={"name":"Tom"}'], [-6], "Error occurs in lackInUpdate function; lack of updator" );
}

function lackInQuery ()
{
   var word = 'query';
   //lack of cmd
   tryCatch( [cl, 'skip=1'], [-100], "Error occurs in lackInQuery function; lack of cmd" );
   //lack of name
   tryCatch( ["cmd=" + word], [-6], "Error occurs in lackInQuery function; lack of name" );
}

function lackInDelete ()
{
   var word = 'delete';
   //lack of name   
   tryCatch( ["cmd=" + word, 'deletor={"age":100}'], [-6], "Error occurs in lackInDelete function; lack of name" );
}




