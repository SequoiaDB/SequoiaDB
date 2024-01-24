/****************************************************
@description:   insert, normal case
                testlink cases: seqDB-7198
@input:         run CURL command: 
                curl http://127.0.0.1:11814/ -d "cmd=insert&name=local_test_cs.local_test_cl&insertor={age:11,name:"Tom",male:true,parents:null,tel:[123456,654321],addr:{city:"Shenzhen"}}"
@expectation:   1 varCL.count(): 1
                2 check _id field: exist
                3 varCL.find({age:11,name:"Tom",male:true,parents:null,tel:[123456,654321],addr:{city:"Shenzhen"}})
                return only 1 record
@modify list:
                2015-4-3 Ting YU init   2016-3-16 XiaoNi Huang init
****************************************************/

var csName = COMMCSNAME;
var clName = COMMCLNAME + "_7198";


main( test );

function test ()
{
   commDropCL( db, csName, clName, true, true, "drop cl in begin" );
   var varCL = commCreateCL( db, csName, clName, {}, true, false, "create cl in begin" );
   insertAndCheck( varCL );
   commDropCL( db, csName, clName, false, false, "drop cl in clean" );
}


function insertAndCheck ( varCL )
{
   var num = 1;
   var word = "insert";
   tryCatch( ["cmd=" + word, "name=" + csName + '.' + clName, 'insertor={age:11,name:"Tom",male:true,parents:null,tel:[123456,654321],addr:{city:"Shenzhen"}}'], [0], "insertAndCheck: fail to run rest cmd=" + word );

   var size = varCL.count();
   if( num != size )
   {
      throw new Error( "num: " + num + "\nsize: " + size );
   }

   var size = 0;
   var fieldNum = 0;
   var rc = varCL.find( { age: 11, name: "Tom", male: true, parents: null, tel: [123456, 654321], addr: { city: "Shenzhen" } } );
   while( rc.next() )
   {
      fieldNum = 0;
      if( rc.current().toObj()["_id"] == undefined )
      {
         throw new Error( "rc.current().toObj()[\"_id\"]: " + rc.current().toObj()["_id"] );
      }
      for( var x in rc.current().toObj() )
      {
         fieldNum++;
      }
      if( fieldNum != 7 )
      {
         throw new Error( "fieldNum: " + fieldNum );
      }
      size++;
   }
   if( num != size )
   {
      throw new Error( "num: " + num + "\nsize: " + size );
   }
}


