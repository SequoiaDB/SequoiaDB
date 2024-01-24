using System;
using System.Text;
using System.Collections.Generic;
using System.Linq;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using SequoiaDB;
using CSharp.TestCommon;
using SequoiaDB.Bson;
using System.Text.RegularExpressions;

namespace CSharp.Crud.DataType
{
    /**
     * description: test type BsonDecimal
     * testcase:    14592
     * author:      linsuqiang
     * date:        2018/3/12
     */

    [TestClass]
    public class BsonDecimal14592
    {
        private Sequoiadb sdb = null;
        private CollectionSpace cs = null;
        private DBCollection cl = null;
        private const string clName = "cl14592";
        private List<BsonDocument> validDocs = null;
        // private List<BsonDocument> invalidDocs = null;
        private List<BsonDocument> specialDocs = null;

        [TestInitialize()]
        public void SetUp()
        {
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " begin: " + this.GetType().ToString());
            sdb = new Sequoiadb(SdbTestBase.coordUrl);
            sdb.Connect();
            cs = sdb.GetCollectionSpace(SdbTestBase.csName);
            cl = cs.CreateCollection(clName);

            validDocs = new List<BsonDocument> 
            {
                new BsonDocument("minPrecision", new BsonDecimal("1", 1, 0)),
                new BsonDocument("maxPrecision", new BsonDecimal("1", 1000, 999)),
                new BsonDocument("posInt", new BsonDecimal("+100")),
                new BsonDocument("negInt", new BsonDecimal("-100")),
                new BsonDocument("posFloat", new BsonDecimal("+1.88")),
                new BsonDocument("negFloat", new BsonDecimal("-1.88")),
                new BsonDocument("zero", new BsonDocument { { "$decimal", "0" } }),
                new BsonDocument("fromDecimal", new BsonDecimal(new Decimal(1)))
            };
            // invalidDocs = new List<BsonDocument>
            // {
                // new BsonDocument("overPrecision", new BsonDecimal("11", 1, 0)),      // compile error
                // new BsonDocument("overPrecision", new BsonDecimal("1", 1001, 0)),    // compile error
                // new BsonDocument("overPrecision", new BsonDecimal("1", 0, 0)),       // compile error
                // new BsonDocument("notDigital", new BsonDecimal("abc", 100, 0)),      // compile error
            // };
            specialDocs = new List<BsonDocument>
            {
                new BsonDocument("MAX", new BsonDecimal("MAX")),
                new BsonDocument("MAX", new BsonDecimal("max")),
                new BsonDocument("MAX", new BsonDecimal("INF")),
                new BsonDocument("MAX", new BsonDecimal("inf")),
                new BsonDocument("MIN", new BsonDecimal("MIN")),
                new BsonDocument("MIN", new BsonDecimal("min")),
                new BsonDocument("MIN", new BsonDecimal("-INF")),
                new BsonDocument("MIN", new BsonDecimal("-inf")),
                new BsonDocument("NaN", new BsonDecimal("NAN")),
                new BsonDocument("NaN", new BsonDecimal("nan"))
            };
        }
        
        [TestMethod()]
        public void TestDecimal14592()
        {
            cl.BulkInsert(validDocs, 0);
            List<BsonDocument> insertedDocs = QueryAndReturnAll(cl);
            Assert.IsTrue(Common.IsEqual(validDocs, insertedDocs),
                "expect:" + validDocs.ToJson() + " actual:" + insertedDocs.ToJson());

            UpdateDecimal(cl);

            cl.Delete(null);
            Assert.AreEqual(0, cl.GetCount(null));

            cl.BulkInsert(specialDocs, 0);
            List<BsonDocument> readSpDocs = QueryAndReturnAll(cl);
            CheckSpecialDoc(readSpDocs);
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

        public List<BsonDocument> QueryAndReturnAll(DBCollection cl)
        {
            List<BsonDocument> result = new List<BsonDocument>();
            DBCursor cursor = cl.Query();
            while (cursor.Next() != null)
            {
                BsonDocument doc = cursor.Current();
                result.Add(doc);
            }
            cursor.Close();
            return result;
        }

        public void UpdateDecimal(DBCollection cl)
        {
            BsonDocument matcher = new BsonDocument("fromDecimal", new BsonDocument("$exists", 1));
            BsonDecimal dec = new BsonDecimal(new Decimal(2));
            BsonDocument modifier = new BsonDocument("$set", new BsonDocument("fromDecimal", dec));
            cl.Update(matcher, modifier, /*hint*/null);
            List<BsonDocument> updatedDocs = QueryAndReturnAll(cl);

            List<BsonDocument> expDocs = new List<BsonDocument>();
            expDocs.AddRange(validDocs);
            expDocs.Last().Set("fromDecimal", dec);
            Assert.IsTrue(Common.IsEqual(expDocs, updatedDocs),
                   "expect:" + expDocs.ToJson() + " actual:" + updatedDocs.ToJson()); 
        }

        public void CheckSpecialDoc(List<BsonDocument> spDocs)
        {
            for (int i = 0; i < spDocs.Count; ++i)
            {
                BsonDocument spDoc = spDocs.ElementAt(i);
                spDoc.Remove("_id");
                Assert.AreEqual(1, spDoc.ElementCount);
                BsonElement elem = spDoc.First();
                BsonDocument expDoc = new BsonDocument(elem.Name, new BsonDecimal(elem.Name));
                Assert.AreEqual(expDoc, spDoc);
            }
        }
    }
}
