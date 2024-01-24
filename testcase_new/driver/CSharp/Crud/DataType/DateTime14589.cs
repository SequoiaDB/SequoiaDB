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
     * description: test type BsonDateTime
     * testcase:    14586
     * author:      linsuqiang
     * date:        2018/3/12
     */

    [TestClass]
    public class DataTime14589
    {
        private Sequoiadb sdb = null;
        private CollectionSpace cs = null;
        private DBCollection cl = null;
        private const string clName = "cl14589";
        private BsonDocument validDoc = null;
        private List<BsonDocument> invalidDocs = null;

        private const long MILLISEC_PER_DAY = 24 * 60 * 60 * 1000;
        private const long MILLISEC_PER_YEAR = 365 * MILLISEC_PER_DAY;

        [TestInitialize()]
        public void SetUp()
        {
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " begin: " + this.GetType().ToString());
            sdb = new Sequoiadb(SdbTestBase.coordUrl);
            sdb.Connect();
            cs = sdb.GetCollectionSpace(SdbTestBase.csName);
            cl = cs.CreateCollection(clName);

            long maxMilliSec = DateTimeToMilliSec(new DateTime(9999, 12, 31));
            long minMilliSec = DateTimeToMilliSec(new DateTime(0001, 1, 1)) - MILLISEC_PER_YEAR - MILLISEC_PER_DAY;
            validDoc = new BsonDocument
            {
                { "maxDate", new BsonDateTime(new DateTime(9999, 12, 31)) },
                { "minDate", new BsonDateTime(new DateTime(0001, 1, 1)) }, // Note: 0000-1-1 makes compile error
                { "maxBson", new BsonDocument("$date", "9999-12-31") },
                { "minBson", new BsonDocument("$date", "0000-01-01") },
                { "maxMsec", new BsonDateTime(maxMilliSec) },
                { "minMsec", new BsonDateTime(minMilliSec) }
            };
            invalidDocs = new List<BsonDocument> {
                // invalid DateTime will make compile error, so ignore them.
                new BsonDocument("notFormatBson", new BsonDocument("$date", "10000-1-1")),
                new BsonDocument("notExistTimeBson", new BsonDocument("$date", "0000-12-32")),
                new BsonDocument("overMax", new BsonDateTime(maxMilliSec + MILLISEC_PER_DAY)),
                new BsonDocument("overMin", new BsonDateTime(minMilliSec - MILLISEC_PER_DAY))
            };
        }
        
        [TestMethod()]
        public void TestDataTime14589()
        {
            cl.Insert(validDoc);
            BsonDocument insertedDoc = QueryAndReturn(cl);
            Assert.IsTrue(Common.IsEqual(validDoc, insertedDoc),
                   "expect:" + validDoc.ToJson() + " actual:" + insertedDoc.ToJson()); 
            
            BsonDocument normalDate = new BsonDocument("normalDate", new BsonDateTime(new DateTime()));
            BsonDocument modifier = new BsonDocument("$set", normalDate);
            cl.Update(/*matcher*/null, modifier, /*hint*/null);
            BsonDocument updatedDoc = QueryAndReturn(cl);

            BsonDocument expDoc = (BsonDocument) validDoc.DeepClone();
            expDoc.Add(normalDate);
            Assert.IsTrue(Common.IsEqual(expDoc, updatedDoc),
                   "expect:" + expDoc.ToJson() + " actual:" + updatedDoc.ToJson()); 

            cl.Delete(null);
            Assert.AreEqual(0, cl.GetCount(null));

            // TODO: fail for SEQUOIADBMAINSTREAM-3350
            // for (int i = 0; i < invalidDocs.Count; ++i)
            // {
            //     try 
            //     {
            //         BsonDocument doc = invalidDocs.ElementAt(i);
            //         cl.Insert(doc);
            //         Assert.Fail(doc.ToJson() + " shouldn't be inserted successfully");
            //     }
            //     catch (BaseException e)
            //     {
            //         if (e.ErrorCode != -6) // SDB_INVALID_ARG
            //             throw e;
            //     }
            // }
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

        private long DateTimeToMilliSec(DateTime dt)
        {
            DateTime dtOrigin = new DateTime(1970, 1, 1);
            TimeSpan span = dt - dtOrigin;
            return (long) span.TotalMilliseconds;
        }

        // query and return the only one document
        public BsonDocument QueryAndReturn(DBCollection cl)
        {
            DBCursor cursor = cl.Query();
            BsonDocument res = cursor.Next();
            Assert.IsNull(cursor.Next());
            cursor.Close();
            return res;
        }
    }
}
