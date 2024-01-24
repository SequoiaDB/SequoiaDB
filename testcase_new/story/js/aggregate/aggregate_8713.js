/*******************************************************************************
*@Description : Test aggregate method using complex combination of argument
*               options.
*@Modify list :
*               2014-10-10  xiaojun Hu  change
*               2016-03-18  wenjing wang modify
*******************************************************************************/

main( test );
function test ()
{
   var cl = new collection( db, COMMCSNAME, COMMCLNAME );
   cl.create();
   var docNumber = loadData( cl );
   var cursor = cl.execAggregate( { $match: { $and: [{ no: { $gte: 10 } }, { group: { $in: [1, 2, 3] } }, { no: { $mod: [5, 3] } }] } },
      { $sort: { group: 1, no: 1 } },
      { $match: { $or: [{ price: { $lt: 1500, $gt: 1000 } }, { price: { $lt: 4000, $gt: 3500 } }] } },
      { $skip: 5 }, { $limit: 100 },
      {
         $group: {
            _id: "$group", avg_price: { $avg: "$price" },
            sum_price: { $sum: "$price" }, max_price: { $max: "$price" },
            min_price: { $min: "$price" }, group: { $first: "$group" },
            push_price: { $push: "$price" }
         }
      }
   );

   var expectResult = [{ "avg_price": 2780, "sum_price": 13900, "max_price": 3980, "min_price": 1180, "group": 2, "push_price": [1180, 1380, 3580, 3780, 3980] },
   { "avg_price": 2230, "sum_price": 11150, "max_price": 3830, "min_price": 1030, "group": 3, "push_price": [1030, 1230, 1430, 3630, 3830] }];
   var parameters = "{$match:{$and:[{no:{$gte:10}}, {group:{$in:[1, 2, 3]}}, {no:{$mod:[5, 3]}}]}}, "
   parameters += "{$sort:{group:1, no:1}}, ";
   parameters += "{$match:{$or:[{price:{$lt:1500, $gt:1000}}, {price:{$lt:4000, $gt:3500}}]}}, "
   parameters += "{$skip:5}, {$limit:100}, ";
   parameters += "{$group: {_id:'$group', avg_price:{$avg:'$price'}, " +
      "sum_price:{$sum:'$price'}, max_price:{$max:'$price'}, " +
      "min_price:{$min:'$price'}, group:{$first:'$group'}, " +
      "push_price:{$push:'$price'} }}";
   checkResult( cursor, expectResult, parameters );
   cl.drop();
}

function loadData ( cl )
{
   var docs = [];
   for( var i = 0; i < 1000; i++ )
   {
      docs.push( { no: i, price: i * 10, group: i % 4 } )

   }
   return cl.bulkInsert( docs );
}