/* *****************************************************************************
@discretion: operators basic: $or
@modify list:
                     2018-09-18 csq  Init
***************************************************************************** */

main( test );

function test ()
{
   if( commIsStandalone( db ) )
   {
      return;
   }

   if( 2 > commGetGroupsNum( db ) )
   {
      return;
   }
   var clName = CHANGEDPREFIX + "macthe15084";
   commDropCL( db, COMMCSNAME, clName, true, true );

   //create CL
   var groups = commGetGroups( db );
   var srcGroupName = groups[0][0].GroupName;
   var destGroupName = groups[1][0].GroupName;
   var varCL = commCreateCL( db, COMMCSNAME, clName, { ShardingKey: { a: 1 }, ShardingType: "hash", Group: srcGroupName }, true, false, "create cl in the beginning" )
   insertData( varCL, srcGroupName, destGroupName );
   checkResult( varCL )
   commDropCL( db, COMMCSNAME, clName, true, true );
}

function insertData ( varCL, srcGroupName, destGroupName )
{

   varCL.split( srcGroupName, destGroupName, 50 );
   varCL.insert( { a: 0, b: 1, c: 0 } );
   varCL.insert( { a: 1, b: 0, c: 0 } );
   for( var i = 2; i < 50; i++ )
   {
      varCL.insert( { a: i, b: -i, c: 0 } );
   }
}

function checkResult ( varCL )
{

   var cur = varCL.find( { "$or": [{ "a": 1 }, { "b": 1 }] } ).sort( { b: -1 } );
   var expFindResult = [{ a: 0, b: 1, c: 0 }, { a: 1, b: 0, c: 0 }];
   checkRec( cur, expFindResult );

   var cur = varCL.find( { "$or": [{ a: { $gte: 30 } }, { b: { $gte: -30 } }] } ).sort( { a: 1 } );
   var expFindResult = [{ a: 0, b: 1, c: 0 }, { a: 1, b: 0, c: 0 }, { a: 2, b: -2, c: 0 }, { a: 3, b: -3, c: 0 }, { a: 4, b: -4, c: 0 },
   { a: 5, b: -5, c: 0 }, { a: 6, b: -6, c: 0 }, { a: 7, b: -7, c: 0 }, { a: 8, b: -8, c: 0 }, { a: 9, b: -9, c: 0 },
   { a: 10, b: -10, c: 0 }, { a: 11, b: -11, c: 0 }, { a: 12, b: -12, c: 0 }, { a: 13, b: -13, c: 0 }, { a: 14, b: -14, c: 0 },
   { a: 15, b: -15, c: 0 }, { a: 16, b: -16, c: 0 }, { a: 17, b: -17, c: 0 }, { a: 18, b: -18, c: 0 }, { a: 19, b: -19, c: 0 },
   { a: 20, b: -20, c: 0 }, { a: 21, b: -21, c: 0 }, { a: 22, b: -22, c: 0 }, { a: 23, b: -23, c: 0 }, { a: 24, b: -24, c: 0 },
   { a: 25, b: -25, c: 0 }, { a: 26, b: -26, c: 0 }, { a: 27, b: -27, c: 0 }, { a: 28, b: -28, c: 0 }, { a: 29, b: -29, c: 0 },
   { a: 30, b: -30, c: 0 }, { a: 31, b: -31, c: 0 }, { a: 32, b: -32, c: 0 }, { a: 33, b: -33, c: 0 }, { a: 34, b: -34, c: 0 },
   { a: 35, b: -35, c: 0 }, { a: 36, b: -36, c: 0 }, { a: 37, b: -37, c: 0 }, { a: 38, b: -38, c: 0 }, { a: 39, b: -39, c: 0 },
   { a: 40, b: -40, c: 0 }, { a: 41, b: -41, c: 0 }, { a: 42, b: -42, c: 0 }, { a: 43, b: -43, c: 0 }, { a: 44, b: -44, c: 0 },
   { a: 45, b: -45, c: 0 }, { a: 46, b: -46, c: 0 }, { a: 47, b: -47, c: 0 }, { a: 48, b: -48, c: 0 }, { a: 49, b: -49, c: 0 },];
   checkRec( cur, expFindResult );
}






