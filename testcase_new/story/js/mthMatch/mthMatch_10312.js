/******************************************************************************
 * @Description   : seqDB-10312:find condition include function and matches,format is illegal;
 * @Author        : zhaoyu
 * @CreateTime    : 2016.10.24
 * @LastEditTime  : 2021.02.04
 * @LastEditors   : Lai Xuan
 ******************************************************************************/
main( test );
function test ()
{
   //clean environment before test
   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true, "drop CL in the beginning" );

   //create cl
   var dbcl = commCreateCL( db, COMMCSNAME, COMMCLNAME );

   //insert data
   var doc = [{ No: 1, a: [1, 2, 3, 4, 5, 6] },
   { No: 2, a: { 0: 1, 1: 2, 2: 3, 3: 4, 4: 5, 5: 6 } },
   { No: 3, a: [4, 5, 6, 7, 8] },
   { No: 4, a: { 0: 4, 1: 5, 2: 6, 3: 7, 4: 8 } },
   { No: 5, a: 1 },
   { No: 6, a: [1, 2, 3, 4, 5, 6, 7, 8, 9] },
   { No: 7, a: [1, 2, 3, 7, 8, 9] },
   { No: 8, a: [11, 12, 13, 17, 18, 19] }];
   dbcl.insert( doc );

   matches = [{ $et: 1 },
   { $gt: 1 },
   { $gte: 1 },
   { $lt: 1 },
   { $lte: 1 },
   { $ne: 1 },
   { $mod: [2, 1] },
   { $in: [1] },
   { $nin: [1] },
   { $all: [1] },
   { $exists: 1 },
   { $isnull: 1 },
   { $elemMatch: { a: 1 } },
   { $regex: '', $options: 'i' },
   { $field: "b" }];
   functions = [{ $abs: 1 },
   { $floor: 1 },
   { $ceiling: 1 },
   { $subtract: 1 },
   { $add: 1 },
   { $multiply: 1 },
   { $divide: 1 },
   { $mod: 1 },
   { $substr: 1 },
   { $strlen: 1 },
   { $upper: 1 },
   { $lower: 1 },
   { $trim: 1 },
   { $ltrim: 1 },
   { $rtrim: 1 },
   { $cast: 1 },
   { $type: 1 },
   { $size: 1 },
   { $slice: 1 }];

   //check result loop for loopNum
   var loopNum = 100;
   for( var i = 0; i < loopNum; i++ )
   {
      var findCond1 = getRdmFuncsAndMatchsFromArr( matches, functions );
      var combinationFind1 = { a: findCond1 };
      InvalidArgCheck( dbcl, combinationFind1, null, -6 );

      var findCond2 = getRdmFuncsAndMatchsFromArr1( functions, matches );
      var combinationFind2 = { a: findCond2 };
      InvalidArgCheck( dbcl, combinationFind2, null, -6 );
   }

}


/************************************
*@Description: unset the order for arr elements.
*@author:      zhaoyu 
*@createDate:  2016/10/24
*@parameters:               
**************************************/
function getRdmFuncsAndMatchsFromArr ( functionsArr, matchesArr )
{
   var functions = parseInt( Math.random() * functionsArr.length );
   var matches = parseInt( Math.random() * matchesArr.length );
   var obj = mergeObj( functionsArr[functions], matchesArr[matches] );
   return obj;
}

function getRdmFuncsAndMatchsFromArr1 ( functionsArr, matchesArr )
{
   var matches = parseInt( Math.random() * matchesArr.length );
   var functions = parseInt( Math.random() * functionsArr.length );
   var functions1 = parseInt( Math.random() * functionsArr.length );
   while( functions == functions1 )
   {
      functions1 = parseInt( Math.random() * functionsArr.length );
   }
   var obj1 = mergeObj( functionsArr[functions], matchesArr[matches] );
   var obj2 = mergeObj( obj1, functionsArr[functions1] );
   return obj2;
}