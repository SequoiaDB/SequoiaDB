/* *****************************************************************************
@Description: seqDB-438:新增数据组名重复
@author:      Zhao Xiaoni
***************************************************************************** */
testConf.skipStandAlone = true;

main( test );

function test()
{
   var groupName = db.listReplicaGroups().current().toObj().GroupName;
   try
   {
      var dataRG = db.createRG( groupName );
      throw "Create group should be failed!";
   }
   catch( e )
   {  
      if( e !== -153 && e !== -6 )
      {  
         throw new Error( e );
      }
   }
}
