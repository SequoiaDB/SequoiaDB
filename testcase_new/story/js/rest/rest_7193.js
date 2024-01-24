/****************************************************
@description:   create/drop cl, abnormal case
                testlink cases: seqDB-7193 & 7195
@expectation:   1 lackNameCreateCL(): lack [name] expect: return -6
                2 lackNameDropCL(): lack [name] expect: return -6
@modify list:
                2015-3-30 Ting YU init   2016-3-16 XiaoNi Huang init
****************************************************/
var csName = COMMCSNAME;

main( test );

function test ()
{
   lackNameCreateCL();
   lackNameDropCL();
}


function lackNameCreateCL ()
{
   tryCatch( ["cmd=create collection"], [-6], "Error occurs in " + getFuncName() );
}

function lackNameDropCL ()
{
   tryCatch( ["cmd=drop collection"], [-6], "Error occurs in " + getFuncName() );
}



