/*******************************************************************************
*@Description : test query when using regex option.
*               [ Revision: 15549, JIRA: SEQUOIADBMAINSTREAM-332]
*@Modify list :
*               2014-6-20  xiaojun Hu  Init
                2020-08-13 Zixian Yan  Modify
*******************************************************************************/
testConf.clName = COMMCLNAME + "_13765";
main( test );

function test ( testPara )
{
   var cl = testPara.testCL;
   //insert data
   var queryNum = 0;
   var begVal = "a";
   var endVal = "z";
   var midVal = "m";
   for( var i = begVal.charCodeAt( 0 ); i <= endVal.charCodeAt( 0 ); ++i )
   {
      var urlVal = String.fromCharCode( i );
      if( i < midVal.charCodeAt( 0 ) )
      {
         cl.insert( { "url": "http://test." + urlVal + ".com" } );
         queryNum++;
      }
      else
         cl.insert( { "url": "http://test." + urlVal + ".cn" } );
   }
   assert.equal( 26, cl.count() );

   // query by use regex
   var regCount1 = cl.count( {
      "url": {
         "$options": "",
         "$regex": "http://test.*.com*"
      }
   } );
   var regCount2 = cl.count( {
      "url": {
         "$regex": "http://test.*.com*",
         "$options": ""
      }
   } );
   assert.equal( regCount1, regCount2 );
   assert.equal( queryNum, regCount1 );

}
