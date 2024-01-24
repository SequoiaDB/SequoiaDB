/******************************************************************************
 * @Description   :
 * @Author        : liuli
 * @CreateTime    : 2021.08.17
 * @LastEditTime  : 2022.08.15
 * @LastEditors   : liuli
 ******************************************************************************/
import( "../lib/main.js" );

/*******************************************************************************
@Description : 删除 $regex:"originName*" 的所有回收站项目
@param : 
@Modify list : 2021-04-06 liuli
*******************************************************************************/
function cleanRecycleBin ( sdb, originName )
{
   // 先删除CS项目，删除CS项目会自动删除CL项目
   var option = { OriginName: { "$regex": originName + "+" }, Type: "CollectionSpace" };
   var rc = sdb.getRecycleBin().list( option, { RecycleName: "" } );
   while( rc.next() )
   {
      var recycleName = rc.current().toObj().RecycleName;
      try
      {
         sdb.getRecycleBin().dropItem( recycleName, true );
      }
      catch( e )
      {
         if( e != SDB_RECYCLE_ITEM_NOTEXIST )
         {
            throw new Error( e );
         }
      }
   }
   rc.close();

   // 再删除CL项目
   var rc = sdb.getRecycleBin().list( { OriginName: { "$regex": originName + "+" } }, { RecycleName: "" } );
   while( rc.next() )
   {
      var recycleName = rc.current().toObj().RecycleName;
      try
      {
         sdb.getRecycleBin().dropItem( recycleName );
      }
      catch( e )
      {
         if( e != SDB_RECYCLE_ITEM_NOTEXIST )
         {
            throw new Error( e );
         }
      }
   }
   rc.close();
}

/*******************************************************************************
@Description : 获取originName相同，opType的回收站项目，按recycleName排序返回
@param : 
@Modify list : 2021-04-08 liuli
*******************************************************************************/
function getRecycleName ( sdb, originName, opType )
{
   var option;
   if( opType == undefined ) 
   {
      option = { "OriginName": originName };
   }
   else
   {
      option = { "OriginName": originName, "OpType": opType };
   }
   var recycleNames = [];
   var rc = sdb.getRecycleBin().list( option );
   while( rc.next() )
   {
      var recycleName = rc.current().toObj().RecycleName;
      recycleNames.push( recycleName );
   }
   rc.close();
   recycleNames.sort();
   return recycleNames;
}

/*******************************************************************************
@Description : 获取originName，opType的回收站项目，有多个时按recycleName排序返回第一个回收站项目
@param : 
@Modify list : 2021-04-08 liuli
*******************************************************************************/
function getOneRecycleName ( sdb, originName, opType )
{
   return getRecycleName( sdb, originName, opType )[0];
}

function checkRecycleItem ( recycleName )
{
   var option = { "RecycleName": recycleName };
   var recyclebinItem = db.getRecycleBin().count( option );
   if( recyclebinItem != 0 )
   {
      throw new Error( "Item not cleared" );
   }
}

/*******************************************************************************
@Description : 检查主子表对应关系及分区范围
@param : subCLName，eg : [subCLName1,subCLName2]
@param : shardRange，eg : [0,1000,2000]
@Modify list : 2021-03-29 liuli
*******************************************************************************/
function checkSubCL ( csName, mainCLName, subCLNames, shardRanges )
{
   var message = "The relationship between mainCL and subCL is inconsistent";
   var cataInfo = db.snapshot( SDB_SNAP_CATALOG, { "Name": csName + "." + mainCLName } ).current().toObj().CataInfo;

   if( subCLNames.length != cataInfo.length )
   {
      throw new Error( "subCLName and cataInfo are not the same length" );
   }

   for( var i = 0; i < cataInfo.length; i++ )
   {
      var actSubCLName = cataInfo[i].SubCLName;
      if( subCLNames.indexOf( actSubCLName ) < 0 )
      {
         throw new Error( message + " expect : " + subCLNames + " actual : " + actSubCLName );
      }
   }

   for( var i = 0; i < cataInfo.length; i++ )
   {
      for( var key in cataInfo[i].LowBound )
      {
         if( shardRanges.indexOf( cataInfo[i].LowBound[key] ) < 0 )
         {
            throw new Error( message + " expect bound range : " + shardRanges + " actual bound range : " + cataInfo[i].LowBound[key] );
         }
      }

      for( var key in cataInfo[i].UpBound )
      {
         if( shardRanges.indexOf( cataInfo[i].UpBound[key] ) < 0 )
         {
            throw new Error( message + " expect bound range : " + shardRanges + " actual bound range : " + cataInfo[i].UpBound[key] );
         }
      }
   }
}

/*******************************************************************************
@Description : 获取RecycleID，opType的回收站项目，有多个时按recycleName排序返回第一个回收站项目
@param : 
@Modify list : 2021-04-08 liuli
*******************************************************************************/
function getRecycleID ( sdb, recycleName )
{
   var recycleID = "";
   var rc = sdb.getRecycleBin().list( { RecycleName: recycleName } );
   while( rc.next() )
   {
      var recycleID = rc.current().toObj().RecycleID;
   }
   rc.close();
   return recycleID;
}

/*******************************************************************************
@Description : 判断环境是否开启回收站
@param : 
@Modify list : 2022-08-15 liuli
*******************************************************************************/
function isRecycleBinOpen ( sdb )
{
   var isRecycleBinOpen = true;
   var recycleBinConf = sdb.getRecycleBin().getDetail().toObj();
   if( recycleBinConf["Enable"] == true && recycleBinConf["ExpireTime"] != 0
      && recycleBinConf["MaxItemNum"] != 0 && recycleBinConf["MaxVersionNum"] != 0 )
   {
      isRecycleBinOpen = true;
   }
   else
   {
      isRecycleBinOpen = false;
   }
   return isRecycleBinOpen;
}
