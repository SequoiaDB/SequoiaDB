/*******************************************************************************
*@Description : [jira_330][seqDB-22587] Query data by $regex.
               1. The prefixion of $regex condition has '\\A' or '^',and $options has contain 'i'
               2. The prefixion of $regex condition has '\\A' or '^',and $options has not contain 'i'
               3. The $regex condition doesn't have prefixion
               4. The query condition consisting by $regex with each one of $or & $and
*@Modifier :
*               2014-11-10  wuyida  Init
                2020-08-08  Zixian Yan
*******************************************************************************/
testConf.clName = COMMCLNAME + "_22587";
testConf.skipStandAlone = true;
main( test );

function test ( testPara )
{
   var cl = testPara.testCL;
   var indexName = "jira_330";
   var data = [{ a: 'abcdefgh', b: 0 },
   { a: 'ABCDefg', b: 1 },
   { a: ['123', '321', '213'], b: 2 },
   { a: 'zuilkj', b: -1 },
   { a: 4, b: 4 },
   { a: 'abcYouAreTheBest', b: 666 }];

   cl.insert( data );
   cl.createIndex( indexName, { a: 1 } );

   var findCondArray = [{ a: { $regex: '^abc', $options: 'xi' } },
   { a: { $regex: '^abc', $options: 'x' } },
   { a: { $regex: '\\ABC', $options: 'xi' } },
   { a: { $regex: '\\ABC', $options: 'x' } },
   { a: { $regex: '^abc' } },
   { a: { $regex: '\\Aabc' } },
   { a: { $regex: 'abc' } },
   { $or: [{ a: { $regex: '^abc' } }, { a: { $gt: 'abcd' } }] },
   { $and: [{ a: { $regex: '^abc' } }, { a: { $gt: 'abcd' } }] },
   { $or: [{ a: { $regex: '\\Aabc' } }, { a: { $gt: 'abcd' } }] },
   { $and: [{ a: { $regex: '\\Aabc' } }, { a: { $gt: 'abcd' } }] }];

   var expectArray = ['tbscan', 'ixscan', 'tbscan', 'ixscan',
      'ixscan', 'ixscan', 'tbscan',
      'tbscan', 'ixscan', 'tbscan', 'ixscan'];

   for( var i in findCondArray )
   {
      var findCondition = findCondArray[i];
      var expectation = expectArray[i];
      var result = cl.find( findCondition ).explain().current().toObj()['ScanType'];
      if( result != expectation )
      {
         var errMsg = "\n\nFind Condition: " + JSON.stringify( findCondition ) +
            "\nActually Scan Type: " + result +
            "\nAnticipant of  Scan Type: " + expectation;
         throw new Error( errMsg );
      }
   }

}
