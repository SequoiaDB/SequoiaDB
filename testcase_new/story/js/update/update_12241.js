/******************************************************************************
 * @Description   : seqDB-12241:set更新嵌套对象
 * @Author        : Hu Xiaojun
 * @CreateTime    : 2014.06.26
 * @LastEditTime  : 2021.08.11
 * @LastEditors   : Zhang Yanan
 ******************************************************************************/
testConf.clName = COMMCLNAME + "_12241";

main( test );

function test ( args )
{
   var varCL = args.testCL;
   // 插入多条包含嵌入对象数据
   for( var i = 1; i <= 100; i++ )
   {
      varCL.insert( {
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
   // set指定修改
   for( var i = 1; i <= 100; i++ )
   {
      varCL.update( { $set: { province: "广东省_" + i } }, { id: "00" + i } );
      varCL.update( { $set: { "city.$1.cityname": "广州市_" + i + "01" } }, { "city.$1.id": "00" + i + "01" } );
      varCL.update( { $set: { "city.$1.areaname.$2.name": "番禺区_" + i + "0101" } }, { "city.$1.areaname.$2.id": "00" + i + "0101" } );
      varCL.update( { $set: { "city.$1.areaname.$2.name": "黄浦区" + i + "0102" } }, { "city.$1.areaname.$2.id": "00" + i + "0102" } );
      varCL.update( { $set: { "city.$1.cityname": "深圳市_" + i + "02" } }, { "city.$1.id": "00" + i + "02" } );
      varCL.update( { $set: { "city.$1.areaname.$2.name": "宝山区_" + i + "0201" } }, { "city.$1.areaname.$2.id": "00" + i + "0201" } );
      varCL.update( { $set: { "city.$1.areaname.$2.name": "萝岗区_" + i + "0202" } }, { "city.$1.areaname.$2.id": "00" + i + "0202" } );
   }

   // 查询结果
   for( i = 1; i <= 100; i++ )
   {
      var rc = varCL.find( {
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
      assert.equal( rc.count(), 1, rc );
   }
}