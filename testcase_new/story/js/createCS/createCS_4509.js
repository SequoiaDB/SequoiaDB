/****************************************************
@description: seqDB-4509:createCS������name��Ч�ַ��ͱ߽�
@author:
2019-6-3 wuyan init
****************************************************/
main( test );
function test ()
{

   var csNameLen = 1;
   createCSAndCheckResult( csNameLen );

   var csNameLen = 127;
   createCSAndCheckResult( csNameLen );
}

function createCSAndCheckResult ( csNameLen )
{
   var csName = getRandomString( csNameLen );
   commDropCS( db, csName, true, "clear cs in the beginning." )

   //create cs; 
   db.createCS( csName );

   //check cs is exist; 
   var clName = "cl4509";
   var dbcs = db.getCS( csName );
   dbcs.createCL( clName );
   dbcs.getCL( clName )

   commDropCS( db, csName, false, "clear cs in the ending." );
}

function getRandomString ( len )
{
   var chars = "1234567890abcdefghijklmnABCDEFGHIJKLMNOPQRSTUVWXYZ-���ġ�~!@#%^&()_ + ~_";
   var str = "";
   var strLen = chars.length;

   var clPrefix = "4509cs_";
   if( len > clPrefix.length )
   {
      len = len - clPrefix.length;
   }

   for( var i = 0; i < len; i++ )
   {
      str += chars.charAt( Math.floor( Math.random() * strLen ) );
   }

   if( len > clPrefix.length )
   {
      str = clPrefix + str;
   }

   return str;
}
