// To run this example, type ./sdb -f example.js

// connect to database
var db = new Sdb('localhost', 11810);

// try-catch block just to ensure that you can run this example multiple times
try {
   db.dropCS('foo');
} catch ( e ) {
}

db.createCS("foo");
db.foo.createCL("bar");

// insert document {a:1} into collection bar on collection space foo
// if foo or bar is not present, it will be automatically created
db.foo.bar.insert({a:1})

// bulk insert
var docs = []
for (var i = 0; i < 10; i++) {
   docs.push({a:i, b:i+100, c:2*i+10});
}
db.foo.bar.insert(docs);

// list all documents(1)
println();
println('----list all documents, approach 1, using next()----');
var result = db.foo.bar.find();
while (true) {
   var doc = result.next();
   if ( ! doc )
      break;
   println(doc);
}

// list all documents(2)
println();
println('----list all documents, approach 2, using toArray()----');
println(db.foo.bar.find().toArray().join('\n'));

// list all documents(3)
println();
println('----list all documents, approach 3, using array index----');
var result = db.foo.bar.find();
for ( var i = 0; i < result.size(); i++ ) {
   println(result[i]);
}

// list documents where a > 5 and sort result in non-descending order in field a,
// selecting only fields a and b.
println();
println('----a > 5, select a, b, sort on a----');
var result = db.foo.bar.find({a:{$gt:5}},{a:1,b:1}).sort({a:1});
println(result.toArray().join('\n'));

// update documents where a = 5 to a = 23
println();
println('----update a=5 to a=23----');
println('before:');
println(db.foo.bar.find({a:5}).toArray().join('\n'));
db.foo.bar.update({$set:{a:23}},{a:5});
println('after:');
println(db.foo.bar.find({a:23}).toArray().join('\n'));

// delete documents where 3 < a < 7
println();
println('----delete 3 < a < 7----');
println('before');
println(db.foo.bar.find().toArray().join('\n'));
db.foo.bar.remove({a:{$gt:3,$lt:7}})
println('after');
println(db.foo.bar.find().toArray().join('\n'));

