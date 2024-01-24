/****************************************************
@description:     test analyze inexistent cl
@testlink cases:  seqDB-14233
@modify list:
2018-07-30        linsuqiang init
****************************************************/

main( test );

function test ()
{
   var clFullName = COMMCSNAME + "." + COMMCLNAME + "_14233";
   tryCatch( ["cmd=analyze", "options={Collection:\"" + clFullName + "\"}"], [-23], "wrong error code when analyze inexistent cl" );
}
