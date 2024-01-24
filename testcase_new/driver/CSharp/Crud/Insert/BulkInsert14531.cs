using System;
using System.Text;
using System.Linq;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using SequoiaDB;
using CSharp.TestCommon;
using SequoiaDB.Bson;
using System.Collections.Generic;

namespace CSharp.Crud.Insert
{
    /**
     * description: bulkInsert records
     *              test interface:  bulkInsert (List< BsonDocument > records, int flag)
     * testcase:    14531
     * author:      wuyan
     * date:        2018/3/10
    */
    [TestClass]
    public class BulkInsert14531
    {
        private Sequoiadb sdb = null;
        private CollectionSpace cs = null;
        private DBCollection cl = null;
        private string clName = "cl14531";

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
        public void TestBulkInsert14531()
        {
            TestBulkInsert();
            bulkInsertDuplicateKey0();
            bulkInsertDuplicateKey1();
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

        public void TestBulkInsert()
        {
            List<BsonDocument> insertor = new List<BsonDocument>();
            for (int i = 0; i < 2; i++)
            {
                BsonDocument obj = new BsonDocument();
                ObjectId id = ObjectId.GenerateNewId();
                BsonMinKey minKey = BsonMinKey.Value;
                string str = "32345.067891234567890123456789" + i;
                byte[] b = new byte[] {1,2,3};
                obj.Add("_id", id).
                            Add("operation", "BulkInsert").
                            Add("date", DateTime.Now.ToString()).
                            Add("timestamp", new BsonTimestamp(2000000000L)).
                            Add("float", 3.1456).
                            Add("a", -2147483648).
                            Add("long", 9223372036854775807L).
                            Add("decimal", new BsonDecimal(str, 23, 5)).
                            Add("bool", true).
                            Add("binary", new BsonBinaryData(b)).
                            Add("arr", new BsonArray() { "test", 123 }).
                            Add("null", null).
                            Add("obj", new BsonDocument("a", "testobj")).
                            Add("minKey", minKey).
                            Add("maxKey", BsonMaxKey.Value);

                insertor.Add(obj);
            }
            cl.BulkInsert(insertor, SDBConst.FLG_INSERT_CONTONDUP);

            //check the result
            DBQuery query = new DBQuery();
            DBCursor cursor = cl.Query(query);
            List<BsonDocument> querylist = new List<BsonDocument>();
            while (cursor.Next() != null)
            {
                BsonDocument actObj = cursor.Current();
                querylist.Add(actObj);
            }
            cursor.Close();

            insertor.Sort(delegate(BsonDocument x, BsonDocument y)
            {
                return x.CompareTo(y);
            });
            querylist.Sort(delegate(BsonDocument x, BsonDocument y)
            {
                return x.CompareTo(y);
            });
            Assert.AreEqual(insertor.ToJson(), querylist.ToJson());
        }

        //set the flag is 0
        public void bulkInsertDuplicateKey0()
        {
            try
            {
                cl.Delete(null);
                List<BsonDocument> insertor = generateDuplicateData();
                cl.BulkInsert(insertor, 0);
                Assert.Fail("bulkInsert will interrupt when Duplicate key exist");
            }
            catch (BaseException e)
            {
                //exist duplicate key ,the error is -38
                Assert.AreEqual(e.ErrorCode, -38, e.ErrorCode + e.Message);
            }

            //exist duplicatekey,only insert 1 records ,then continue insert
            long count = cl.GetCount(null);
            Assert.AreEqual(1, count);
            BsonDocument prepareDataCondition = new BsonDocument
            {
                {"testadd","testadd"}
            };
            long count1 = cl.GetCount(prepareDataCondition);
            Assert.AreEqual(0, count1);
        }

        //set the flag is SDBConst.FLG_INSERT_CONTONDUP
        public void bulkInsertDuplicateKey1()
        {
            cl.Delete(null);
            List<BsonDocument> insertor = generateDuplicateData();
            cl.BulkInsert(insertor, SDBConst.FLG_INSERT_CONTONDUP);

            //exist duplicatekey,only insert 1 records ,then unterrupt insert 
            long count = cl.GetCount(null);
            Assert.AreEqual(2, count);
            BsonDocument prepareDataCondition = new BsonDocument
            {
                {"testadd","testadd"}
            };
            long count1 = cl.GetCount(prepareDataCondition);
            Assert.AreEqual(1, count1);
        }


        public List<BsonDocument> generateDuplicateData()
        {
            List<BsonDocument> insertor = new List<BsonDocument>();
            for (int i = 0; i < 2; i++)
            {
                BsonDocument obj = new BsonDocument();
                obj.Add("_id", 1).
                            Add("operation", "BulkInsert").
                            Add("date", DateTime.Now.ToString()).
                            Add("timestamp", new BsonTimestamp(1000000000L));
                insertor.Add(obj);
            }

            BsonDocument obj1 = new BsonDocument();
            obj1.Add("testadd", "testadd");
            insertor.Add(obj1);
            return insertor;
        }
    }
}
