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
     * description: test type BsonBinaryData
     * testcase:    14587
     * author:      linsuqiang
     * date:        2018/3/12
     */

    [TestClass]
    public class BinData14587
    {
        private Sequoiadb sdb = null;
        private CollectionSpace cs = null;
        private DBCollection cl = null;
        private const string clName = "cl14587";
        private List<BsonDocument> validDocs = null;
        private List<BsonDocument> invalidDocs = null;

        [TestInitialize()]
        public void SetUp()
        {
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " begin: " + this.GetType().ToString());
            sdb = new Sequoiadb(SdbTestBase.coordUrl);
            sdb.Connect();
            cs = sdb.GetCollectionSpace(SdbTestBase.csName);
            cl = cs.CreateCollection(clName);

            // all BsonBinarySubType as below:
            // Binary, Function, OldBinary, UuidLegacy, UuidStandard, MD5, UserDefined
            //
            // in this test. we just consider type<Binary> and type<UserDefined>.
            // because other types are implemented by thirdpart, not our business.
            // function<BsonBinaryData (Guid , GuidRepresentation )> and some guid 
            // related parameter will not be tested for the same reason.
            //
            validDocs = new List<BsonDocument> 
            {
                new BsonDocument("zeroBin", new BsonBinaryData(new byte[0])).Add("a",1),
                new BsonDocument("bigBin", new BsonBinaryData(new byte[15 * 1024 * 1024], BsonBinarySubType.Binary)).Add("a",2),
                new BsonDocument("udfBin", new BsonBinaryData(new byte[16], BsonBinarySubType.UserDefined)).Add("a",3),
                new BsonDocument("zeroBson", new BsonDocument { { "$binary", "" }, { "$type", 1 } }).Add("a",4),
                new BsonDocument("normalBson", new BsonDocument { { "$binary", "aGVsbG8gd29ybGQ="}, { "$type", 1 } }).Add("a",5),
            };
            invalidDocs = new List<BsonDocument>
            {
                new BsonDocument("wrongType", new BsonDocument { { "$binary", "" }, { "$type", 1000 } }),
                new BsonDocument("wrongBinStr", new BsonDocument { { "$binary", "aGVsbG8gd29ybGQ%%%%%="}, { "$type", 1 } })
            };
        }
        
        [TestMethod()]
        public void TestBinData14587()
        {
            cl.BulkInsert(validDocs, 0);
            List<BsonDocument> insertedDocs = QueryAndReturnAll(cl);
            Assert.IsTrue(Common.IsEqual(validDocs, insertedDocs),
                "expect:" + validDocs.ToJson() + " actual:" + insertedDocs.ToJson());

            UpdateNormalBson(cl);

            cl.Delete(null);
            Assert.AreEqual(0, cl.GetCount(null));

            // TODO: fail for SEQUOIADBMAINSTREAM-3353
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

        public List<BsonDocument> QueryAndReturnAll(DBCollection cl)
        {
            List<BsonDocument> result = new List<BsonDocument>();
            DBCursor cursor = cl.Query(null,null,new BsonDocument("a",1),null);
            while (cursor.Next() != null)
            {
                BsonDocument doc = cursor.Current();
                result.Add(doc);
            }
            cursor.Close();
            return result;
        }

        public void UpdateNormalBson(DBCollection cl)
        {
            BsonDocument matcher = new BsonDocument("normalBson", new BsonDocument("$exists", 1));
            BsonDocument newNormalBson = new BsonDocument { { "$binary", "" }, { "$type", 1 } };
            BsonDocument modifier = new BsonDocument("$set", new BsonDocument("normalBson", newNormalBson));
            cl.Update(matcher, modifier, /*hint*/null);
            List<BsonDocument> updatedDocs = QueryAndReturnAll(cl);

            List<BsonDocument> expDocs = new List<BsonDocument>();
            expDocs.AddRange(validDocs);
            expDocs.Last().Set("normalBson", newNormalBson);
            Assert.IsTrue(Common.IsEqual(expDocs, updatedDocs),
                   "expect:" + expDocs.ToJson() + " actual:" + updatedDocs.ToJson()); 
        }
    }
}
