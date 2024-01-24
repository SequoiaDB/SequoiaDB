/******************************************************************************
 * @Description   :
 * @Author        : Yao Kang
 * @CreateTime    : 2021.12.28
 * @LastEditTime  : 2022.02.16
 * @LastEditors   : liuli
 ******************************************************************************/
function insertData ( dbcl, number )
{
   if( undefined == number ) { number = 1000; }
   var docs = [];
   var arr = new Array( 1024 );
   var test = arr.join( "a" );
   for( var i = 0; i < number; ++i )
   {
      var no = i;
      var a = i;
      var user = test + i;
      var phone = 13700000000 + i;
      var time = new Date().getTime();
      var doc = { no: no, a: a, customerName: user, phone: phone, openDate: time };
      //data example: {"no":5, customerName:"test5", "phone":13700000005, "openDate":1402990912105

      docs.push( doc );
   }
   dbcl.insert( docs );
   return docs;
}
