/************************************
*@Description: insert records in capped cl, check count/find and the size of data file
*@author:      liuxiaoxuan
*@createdate:  2017.10.17
*@testlinkCase: seqDB-13018
**************************************/
main( test );
function test ()
{
   var clName = COMMCAPPEDCLNAME + "_13018";
   var clOption = { Capped: true, Size: 1024, AutoIndexId: false };
   var dbcl = commCreateCL( db, COMMCAPPEDCSNAME, clName, clOption, true, true );

   //insert 4w records 
   var insertNums = 40000;
   var stringLength = 968;
   var string = 'a';
   insertFixedLengthDatas( dbcl, insertNums, stringLength, string );

   //check count 
   var expectCount = 40000;
   checkCount( dbcl, null, expectCount );

   //check find
   var limitConf = 3;
   var skipConf = 39997;

   var expectIDs1 = [getOneLogicalID( stringLength, 1 ), getOneLogicalID( stringLength, 0 ), 0];
   var sortConf1 = { _id: -1 };

   var expectIDs2 = [getOneLogicalID( stringLength, 39997 ),
   getOneLogicalID( stringLength, 39998 ),
   getOneLogicalID( stringLength, 39999 )];
   var sortConf2 = { _id: 1 };

   //checkLogicalID( dbcl, null, null, sortConf1, limitConf, skipConf, expectIDs1);
   checkLogicalID( dbcl, null, null, sortConf2, limitConf, skipConf, expectIDs2 );

   //check data file size
   //if(!commIsStandalone(db))
   //{
   //	var dbpath = getDataFilePath(csName , clName);
   //   var expectFileSize = 149 * 1024 * 1024;//init CL size is 149M
   //   checkDataFileSize(dbpath, expectFileSize);
   //}

   //insert records again
   insertNums = 60000;
   insertFixedLengthDatas( dbcl, insertNums, stringLength, string );

   //check count 
   expectCount = 100000;
   checkCount( dbcl, null, expectCount );

   //check find
   var limitConf = 3;
   var skipConf = 99997;

   var expectIDs1 = [getOneLogicalID( stringLength, 1 ), getOneLogicalID( stringLength, 0 ), 0];
   var sortConf1 = { _id: -1 };

   var expectIDs2 = [getOneLogicalID( stringLength, 99997 ),
   getOneLogicalID( stringLength, 99998 ),
   getOneLogicalID( stringLength, 99999 )];
   var sortConf2 = { _id: 1 };

   //checkLogicalID( dbcl, null, null, sortConf1, limitConf, skipConf, expectIDs1);
   checkLogicalID( dbcl, null, null, sortConf2, limitConf, skipConf, expectIDs2 );

   commDropCL( db, COMMCAPPEDCSNAME, clName, true, true, "drop CL in the end" );
}

function getOneLogicalID ( stringLength, skipNum )
{
   var logicalID = 0;
   var blockCounts = 1;
   var block_max_32 = 33554396;

   var recordLength = stringLength + recordHeader;
   if( recordLength % 4 !== 0 )
   {
      recordLength = recordLength - recordLength % 4 + 4;
   }

   for( var i = 0; i < skipNum; ++i )
   {
      logicalID = logicalID + recordLength;
   }
   return logicalID;
}

function getDataFilePath ( csName, clName )
{
   var dbpath = null;
   // get CL group name
   var cl_full_name = csName + "." + clName;
   var clGroupArray = commGetCLGroups( db, cl_full_name );
   var clGroupName = clGroupArray[0];

   //get data file path for CL
   var dataGroupArrays = commGetGroups( db );
   for( var i = 0; i < dataGroupArrays.length; i++ )
   {
      var dataGroupItem = dataGroupArrays[i];
      var dataGroupName = dataGroupItem[0].GroupName;

      var primaryPos = dataGroupItem[0].PrimaryPos;
      if( clGroupName == dataGroupItem[0].GroupName )
      {
         dbpath = dataGroupItem[primaryPos].dbpath + '/' + csName + '.1.data';
         break;
      }
   }

   return dbpath;
}

function getDataFileSize ( dbpath )
{
   var fileSize = -1;
   var file;
   file = new File( dbpath )
   fileSize = file.getSize( dbpath );

   return fileSize;
}

function checkDataFileSize ( dbpath, expectFileSize )
{
   if( dbpath != null )
   {
      var actFileSize = getDataFileSize( dbpath );
      if( actFileSize == -1 || actFileSize > expectFileSize )
      {
         throw new Error( "checkDataFileSize() check data file size" + expectFileSize + actFileSize );
      }
   }
}
