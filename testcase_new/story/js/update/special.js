/******************************************************************************
@Description : 1. Test update data . The data have complex structure.
@Modify list :
               2014-6-26  xiaojun Hu  Changed
******************************************************************************/
main( test );
function test ()
{
   var csName = COMMCSNAME;
   var clName = COMMCLNAME;
   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true, "drop cl in the beginning" );

   var cs = commCreateCS( db, csName, true, "create CS in the beginning" );
   var cl = cs.createCL( clName, { ReplSize: 0, Compressed: true } );

   for( var i = 1; i <= 100; i++ )
   {
      cl.insert( {
         id: "00" + i, province: "", city: [{
            id: "00" + i + "01",
            cityname: "", areaname: [{ id: "00" + i + "0101", name: "" },
            { id: "00" + i + "0102", name: "" }]
         }, {
            id: "00" + i + "02",
            cityname: "", areaname: [{ id: "00" + i + "0201", name: "" },
            { id: "00" + i + "0202", name: "" }]
         }]
      } );
   }

   // inspect the number of data
   var i = 0;
   do
   {
      var count = cl.count();
      if( 100 == count )
         break;
      ++i;
   } while( i < 20 );
   assert.equal( count, 100 );

   for( var i = 1; i <= 100; i++ )
   {
      cl.update( { $set: { province: "广东省_" + i } }, { id: "00" + i } );
      cl.update( { $set: { "city.$1.cityname": "广州市_" + i + "01" } }, { "city.$1.id": "00" + i + "01" } );
      cl.update( { $set: { "city.$1.areaname.$2.name": "番禺区_" + i + "0101" } }, { "city.$1.areaname.$2.id": "00" + i + "0101" } );
      cl.update( { $set: { "city.$1.areaname.$2.name": "黄浦区" + i + "0102" } }, { "city.$1.areaname.$2.id": "00" + i + "0102" } );
      cl.update( { $set: { "city.$1.cityname": "深圳市_" + i + "02" } }, { "city.$1.id": "00" + i + "02" } );
      cl.update( { $set: { "city.$1.areaname.$2.name": "宝山区_" + i + "0201" } }, { "city.$1.areaname.$2.id": "00" + i + "0201" } );
      cl.update( { $set: { "city.$1.areaname.$2.name": "萝岗区_" + i + "0202" } }, { "city.$1.areaname.$2.id": "00" + i + "0202" } );
   }

   for( i = 1; i <= 100; i++ )
   {
      var rc = cl.find( {
         $and: [{
            "city.$1.cityname": "广州市_" + i +
               "01", "city.$1.areaname.$1.name": "番禺区_" + i +
                  "0101", "city.$1.areaname.$2.name": "黄浦区" + i + "0102"
         },
         {
            "city.$2.cityname": "深圳市_" + i + "02",
            "city.$2.areaname.$1.name": "宝山区_" + i + "0201",
            "city.$2.areaname.$2.name": "萝岗区_" + i + "0202"
         }]
      } );
      assert.equal( rc.count(), 1 );
   }

   commDropCL( db, csName, clName, true, true );
}