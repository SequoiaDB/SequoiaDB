/****************************************************
@description:   query, normal case
                testlink cases: seqDB-7204
@input:         insert 6 recs
                queryAndCheck(): use only the required parameters
@expectation:   1 "cmd="+word,"name="+csName+'.'+clName,'sort={"age":1}'
                2 varCL.find({male:true}).skip(1).limit(3).sort({"age":1})
                3 step 1 result == step 2 result
@modify list:
                2015-4-3 Ting YU init   2016-3-16 XiaoNi Huang init
****************************************************/
var csName = COMMCSNAME;
var clName = COMMCLNAME + "_7204";

main( test );

function test ()
{
   commDropCL( db, csName, clName, true, true, "drop cl in begin" );
   var varCL = commCreateCL( db, csName, clName, {}, true, false, "create cl in begin" );
   varCL.insert( [
      { age: 12, name: "Tom", male: true },
      { age: 13, name: "Anna", male: false },
      { age: 15, name: "Jack", male: true },
      { age: 11, name: "Harry", male: true },
      { age: 19, name: "Bob", male: true },
      { age: 9, name: "Jobs", male: true }
   ] );
   queryAndCheck( varCL );
   commDropCL( db, csName, clName, false, false, "drop cl in clean" );
}

function queryAndCheck ( varCL )
{
   var word = "query";
   tryCatch( ["cmd=" + word, "name=" + csName + '.' + clName, 'sort={"age":1}'], [0], "queryAndCheck: fail to run rest command: " + word );

   var arr = infoSplit;
   var rc = varCL.find().sort( { "age": 1 } );
   var i = 1;
   while( rc.next() )
   {
      if( rc.current().toObj()["name"] !== JSON.parse( arr[i] ).name )
      {
         throw new Error( "rc.current().toObj()[\"name\"]: " + rc.current().toObj()["name"] + "\nJSON.parse( arr[i] ).name: " + JSON.parse( arr[i] ).name );
      }
      i++;
   }
}




