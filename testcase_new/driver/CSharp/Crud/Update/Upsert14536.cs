using System;
using System.Text;
using System.Collections.Generic;
using System.Linq;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using SequoiaDB;
using CSharp.TestCommon;
using SequoiaDB.Bson;

namespace CSharp.Crud.Update
{
    /**
     * description: upsert with complex condition
     *              interface: Upsert (BsonDocument matcher, BsonDocument modifier, BsonDocument hint, BsonDocument setOnInsert)
     * testcase:    14536
     * author:      linsuqiang
     * date:        2018/3/12
     */

    [TestClass]
    public class Upsert14536
    {
        private Sequoiadb sdb = null;
        private CollectionSpace cs = null;
        private DBCollection cl = null;
        private const string clName = "cl14536";

        [TestInitialize()]
        public void SetUp()
        {
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " begin: " + this.GetType().ToString());
            sdb = new Sequoiadb(SdbTestBase.coordUrl);
            sdb.Connect();
            cs = sdb.GetCollectionSpace(SdbTestBase.csName);
            cl = cs.CreateCollection(clName);
        }
        
        [TestMethod()]
        public void TestUpsert14536()
        {
            InsertData(cl);
            UpsertData(cl);
            List<BsonDocument> actDocs = QueryAll(cl);
            List<BsonDocument> expDocs = GetExpDocs();
            Assert.IsTrue(Common.IsEqual(expDocs, actDocs),
                    "expDocs:" + expDocs.ToJson() + " actDocs:" + actDocs.ToJson());
        }

        [TestCleanup()]
        public void TearDown()
        {
            try
            {
                cs.DropCollection(clName);
            }
            finally
            {
                if (sdb != null)
                {
                    sdb.Disconnect();    
                }
                Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " end  : " + this.GetType().ToString());
            }
        }
        
        private void InsertData(DBCollection cl)
        {
            List<BsonDocument> docs = new List<BsonDocument>
            {
                new BsonDocument() { { "array", new BsonArray() { 1, 2, 3 } } },
                new BsonDocument() { { "obj", new BsonDocument("subObj", new BsonDocument("subSubObj", 1)) } },
                new BsonDocument() { { "string", "a string" }, { "int", 999 }, { "float", 0.985211 }, { "bool", true } },
                new BsonDocument() 
                { 
                    { 
                        "complexArray", new BsonArray() 
                        { 
                            new BsonDocument("subObj", 1), 
                            new BsonArray() { new BsonDocument("subSubObj", 1) }, 
                            3,
                            new BsonBinaryData(new byte[128]),
                            new BsonRegularExpression("^w")
                        }
                    } 
                }
            };
            cl.BulkInsert(docs, 0);
        }

        private void UpsertData(DBCollection cl)
        {
            BsonDocument matcher = new BsonDocument("array", new BsonDocument("$exists", 1));
            BsonDocument modifier = new BsonDocument("$addtoset", new BsonDocument("array", new BsonArray() { 1, 4, 5 }));
            BsonDocument hint = new BsonDocument("", null);
            BsonDocument setOnInsert = new BsonDocument("insertFlag", 1);
            cl.Upsert(matcher, modifier, hint, setOnInsert);

            matcher = new BsonDocument("obj", new BsonDocument("$exists", 1));
            modifier = new BsonDocument("$set", new BsonDocument("obj", new BsonDocument("subObj", 1)));
            cl.Upsert(matcher, modifier, hint, setOnInsert);

            matcher = BsonDocument.Parse("{ $and: [ { string: { $regex: '.*string.*', $options: '' }, int: { $lt: 1000 }, float: { $gt: 0.5 } } ] }");
            modifier = BsonDocument.Parse("{ $inc: { int: 1 }, $set: { bool: false } }");
            cl.Upsert(matcher, modifier, hint, setOnInsert);

            matcher = new BsonDocument("complexArray.$1", new BsonDocument("$et", new BsonBinaryData(new byte[128])));
            modifier = BsonDocument.Parse("{ $push_all: { 'complexArray.1': [ 1, 2 ] } }");
            cl.Upsert(matcher, modifier, hint, setOnInsert);

            matcher = new BsonDocument("newField", new BsonDocument("$exists", 1));
            modifier = new BsonDocument("$set", new BsonDocument("newField", 188L));
            cl.Upsert(matcher, modifier, hint, setOnInsert);
        }

        private List<BsonDocument> GetExpDocs()
        {
            List<BsonDocument> docs = new List<BsonDocument>
            {
                new BsonDocument() { { "array", new BsonArray() { 1, 2, 3, 4, 5 } } },
                new BsonDocument() { { "obj", new BsonDocument("subObj", 1) } },
                new BsonDocument() { { "string", "a string" }, { "int", 1000 }, { "float", 0.985211 }, { "bool", false } },
                new BsonDocument() 
                { 
                    { 
                        "complexArray", new BsonArray() 
                        { 
                            new BsonDocument("subObj", 1), 
                            new BsonArray() { new BsonDocument("subSubObj", 1), 1, 2 }, 
                            3,
                            new BsonBinaryData(new byte[128]),
                            new BsonRegularExpression("^w")
                        }
                    } 
                },
                new BsonDocument() { { "newField", 188L }, { "insertFlag", 1 } }
            };
            return docs;
        }

        private List<BsonDocument> QueryAll(DBCollection cl)
        {
            List<BsonDocument> docs = new List<BsonDocument>();
            DBCursor cursor = cl.Query();
            while (cursor.Next() != null)
            {
                BsonDocument doc = cursor.Current();
                doc.Remove("_id");
                docs.Add(doc);
            }
            cursor.Close();
            return docs;
        }
    }
}