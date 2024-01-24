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
     * description: test type BsonRegularExpression
     * testcase:    14586
     * author:      linsuqiang
     * date:        2018/3/12
     */

    [TestClass]
    public class Regex14586
    {
        private Sequoiadb sdb       = null;
        private CollectionSpace cs  = null;
        private DBCollection cl     = null;
        private const string clName = "cl14586";

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
        public void TestRegex14586()
        {
            InsertData(cl);

            BsonRegularExpression regexObj = new BsonRegularExpression("^w", "i");
            BsonDocument objMatcher = new BsonDocument("a", regexObj);
            TestQuery(cl, objMatcher);

            BsonDocument regexJson = new BsonDocument{ { "$regex", "^w" }, { "$options", "i" } };
            BsonDocument jsonMatcher = new BsonDocument("a", regexJson);
            TestUpdate(cl, jsonMatcher);

            Regex regex = new Regex("^w", RegexOptions.IgnoreCase);
            BsonDocument regexMatcher = new BsonDocument("a", regex);
            TestDelete(cl, regexMatcher);
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
            List<BsonDocument> insertDocs = new List<BsonDocument>
            {
                new BsonDocument("a", "White"),
                new BsonDocument("a", "white"),
                new BsonDocument("a", "Black"),
                new BsonDocument("a", new BsonRegularExpression("^a"))
            };
            cl.BulkInsert(insertDocs, 0);
        }

        private void TestQuery(DBCollection cl, BsonDocument matcher)
        {
            List<BsonDocument> expDocs = new List<BsonDocument>()
            {
                new BsonDocument("a", "White"),
                new BsonDocument("a", "white")
            };
            List<BsonDocument> actDocs = QueryAndReturn(cl, matcher);
            Assert.IsTrue(Common.IsEqual(expDocs, actDocs), 
                "expect:" + expDocs.ToJson() + " actual:" + actDocs.ToJson());
        }

        private void TestUpdate(DBCollection cl, BsonDocument matcher)
        {
            BsonDocument modifier = new BsonDocument("$set", new BsonDocument("b", 1));
            BsonDocument hint = null;
            cl.Update(matcher, modifier, hint);

            List<BsonDocument> expDocs = new List<BsonDocument>()
            {
                new BsonDocument{ { "a", "White" }, { "b", 1 } },
                new BsonDocument{ { "a", "white" }, { "b", 1 } },
                new BsonDocument{ { "a", "Black" } },
                new BsonDocument("a", new BsonRegularExpression("^a"))
            };
            List<BsonDocument> actDocs = QueryAndReturn(cl, null);
            Assert.IsTrue(Common.IsEqual(expDocs, actDocs), 
                "expect:" + expDocs.ToJson() + " actual:" + actDocs.ToJson());
        }

        private void TestDelete(DBCollection cl, BsonDocument matcher)
        {
            cl.Delete(matcher);

            long count = cl.GetCount(matcher);
            Assert.AreEqual(0, count);
            count = cl.GetCount(null);
            Assert.AreEqual(2, count);
        }

        private List<BsonDocument> QueryAndReturn(DBCollection cl, BsonDocument matcher)
        {
            BsonDocument selector       = null;
            BsonDocument orderBy        = null;
            BsonDocument hint           = null;
            List<BsonDocument> result   = new List<BsonDocument>();
            
            DBCursor cursor = cl.Query(matcher, selector, orderBy, hint);
            while (cursor.Next() != null)
            {
                BsonDocument doc = cursor.Current();
                doc.Remove("_id");
                result.Add(doc);
            }
            cursor.Close();
            return result;
        }

    }
}
