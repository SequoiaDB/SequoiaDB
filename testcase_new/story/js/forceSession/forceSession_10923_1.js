/***********************************************************************
*@Description : test force session with invalid option
*               seqDB-10923:options参数非法校验
*@author      : Zhao xiaoni 
***********************************************************************/
testConf.skipStandAlone = true;

main( test );

function test ()
{
   var sessionID = db.list( 3, { Global: false } ).next().toObj().SessionID;
   var errKeys = ["GroupID", "GroupName", "NodeID", "HostName", "svcname", "NodeSelect", "Role"];
   var errVals = ["", "abcd", 123];
   var errno = [[-6, -6, -154], [-154, -154, -6], [-6, -6, -155], [-155, -155, -6], [-155, -155, -6], [-6, -6, -6], [-6, -6, -6]];

   var options = {};
   for( var i = 0; i < errKeys.length; i++ )
   {
      for( var j = 0; j < errVals.length; j++ )
      {
         var key = errKeys[i];
         var val = errVals[j];
         var err = errno[i][j];
         options[key] = val;
         assert.tryThrow( errno[i], function()
         {
            db.forceSession( sessionID, options );
         } );
      }
   }

}