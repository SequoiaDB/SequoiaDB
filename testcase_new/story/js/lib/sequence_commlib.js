import( "../lib/main.js" );


/************************************
*@Description: 删除序列
*@author:      zhaoyu
*@createDate:  20121.1.8
**************************************/
function dropSequence ( db, name )
{
   try
   {
      db.dropSequence( name );
   } catch( e )
   {
      commCompareErrorCode( e, SDB_SEQUENCE_NOT_EXIST );
   }
}

/*******************************************************************************
@Description : 获取coord组内所有节点名
@return: array,例如：["localhost:11810","localhost:11820"]
@Modify  2018-10-15 zhaoyu init
*******************************************************************************/
function getCoordNodeNames ( db )
{
   var nodeNames = new Array();
   if( commIsStandalone( db ) )
   {
      return nodeNames;
   }
   var rg = db.getCoordRG();

   var details = rg.getDetail();
   while( details.next() )
   {
      var groups = details.current().toObj().Group;
      for( var i = 0; i < groups.length; i++ )
      {
         var hostName = groups[i].HostName;
         var service = groups[i].Service;
         for( var j = 0; j < service.length; j++ )
         {
            if( service[j].Type === 0 )
            {
               var serviceName = service[j].Name;
               break;
            }
         }
         nodeNames.push( hostName + ":" + serviceName );
      }
   }
   return nodeNames;
}

/*******************************************************************************
@Description : 获取sequence属性
@return: 
@Modify list : 2021-1-9 zhaoyu init
*******************************************************************************/
function getSequenceAttr ( db, sequenceName, selector )
{
   var sequenceObj = db.snapshot( SDB_SNAP_SEQUENCES, { Name: sequenceName }, selector ).next().toObj();
   return sequenceObj;
}

