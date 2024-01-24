using System;
using System.Text;
using System.Collections.Generic;
using System.Linq;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using SequoiaDB;
using CSharp.TestCommon;
using SequoiaDB.Bson;


namespace CSharp.Crud.Insert
{
    /**
     * TestCase : seqDB-16622
     * test interface:   public BsonDocument Insert(BsonDocument record, int flags)
     *                   public BsonDocument Insert(List<BsonDocument> recordList, int flags)
     * author:  chensiqin
     * date:    2018/11/19
     * version: 1.0
    */

    [TestClass]
    public class Insert16622
    {
        private Sequoiadb sdb = null;
        private CollectionSpace cs = null;
        private DBCollection cl1 = null;
        private DBCollection cl2 = null;

        private string clName1 = "cl16622_1";
        private string clName2 = "cl16622_2";

        [TestInitialize()]
        public void SetUp()
        {
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " begin: " + this.GetType().ToString());
            sdb = new Sequoiadb(SdbTestBase.coordUrl);
            sdb.Connect();
            cs = sdb.GetCollectionSpace(SdbTestBase.csName);
        }

        [TestMethod]
        public void Test16622()
        {
            TestInsert16622_1();
            TestInsert16622_2();
        }

        [TestCleanup()]
        public void TearDown()
        {
            if (sdb != null)
            {
                sdb.Disconnect();
            }
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " end  : " + this.GetType().ToString());
        }


        //BsonDocument Insert(BsonDocument record, int flags)
        public void TestInsert16622_1()
        {
            cl1 = cs.CreateCollection(clName1);
            BsonDocument record = new BsonDocument();
            record.Add("_id", 1);
            record.Add("num", 1);
            cl1.Insert(record);//插入第一条记录
            try
            {
                cl1.Insert(record, 0);//重复插入record
                Assert.Fail("expected insert throw exception ");
            }
            catch (BaseException e)
            {
                Assert.AreEqual(-38, e.ErrorCode);
            }

            cl1.Insert(record, SDBConst.FLG_INSERT_CONTONDUP);
            Assert.AreEqual(1, cl1.GetCount(null));

            record = new BsonDocument();
            record.Add("_id", 2);
            record.Add("num", 2);
            BsonDocument doc = cl1.Insert(record, SDBConst.FLG_INSERT_RETURN_OID);
            Assert.AreEqual(2, cl1.GetCount(null));
            Assert.AreEqual("{ \"_id\" : 2 }", doc.ToString());

            //other flag sunch as -2
            cl1.Truncate();
            try
            {
                cl1.Insert(record, -2);
            }
            catch (BaseException e)
            {
                Assert.AreEqual(e.ErrorCode, -6, e.ErrorCode + e.Message);
            }
            Assert.AreEqual("{ \"_id\" : 2 }", doc.ToString());

            cs.DropCollection(clName1);
        }
        // BsonDocument Insert(List<BsonDocument> recordList, int flags)
        public void TestInsert16622_2()
        {
            cl2 = cs.CreateCollection(clName2);
            InsertDuplicateKey0();
            InsertDuplicateKey1();
            InsertReturnOID();
            InsertWithOtherFlag();
            cs.DropCollection(clName2);
        }

        //set the flag is 0
        public void InsertDuplicateKey0()
        {
            try
            {
                cl2.Delete(null);
                List<BsonDocument> insertor = GenerateDuplicateData();
                cl2.Insert(insertor, 0);
                Assert.Fail("bulkInsert will interrupt when Duplicate key exist");
            }
            catch (BaseException e)
            {
                Assert.AreEqual(e.ErrorCode, -38, e.ErrorCode + e.Message);
            }
            Assert.AreEqual(1, cl2.GetCount(null));
            //flag为0时失败，报-38之后记录不会再插入
            Assert.AreEqual(0, cl2.GetCount(new BsonDocument { { "test16622", "test16622" } }));
        }

        //set the flag is SDBConst.FLG_INSERT_CONTONDUP
        public void InsertDuplicateKey1()
        {
            cl2.Truncate();
            try
            {
                List<BsonDocument> insertor = GenerateDuplicateData();
                cl2.Insert(insertor, SDBConst.FLG_INSERT_CONTONDUP);
            }
            catch (BaseException e)
            {
                Assert.Fail("Failed to bulkinsert:", e.ErrorCode + e.Message);
            }
            Assert.AreEqual(2, cl2.GetCount(null));
            Assert.AreEqual(1, cl2.GetCount(new BsonDocument { { "test16622", "test16622" } }));
        }

        //set the flag is SDBConst.FLG_INSERT_RETURN_OID
        public void InsertReturnOID()
        {
            cl2.Truncate();
            List<BsonDocument> insertor = new List<BsonDocument>();
            for (int i = 0; i < 2; i++)
            {
                BsonDocument obj = new BsonDocument();
                obj.Add("_id", i).
                            Add("operation", "Insert").
                            Add("name", "zhangsan" + i);
                insertor.Add(obj);
            }
            BsonDocument doc = cl2.Insert(insertor, SDBConst.FLG_INSERT_RETURN_OID);
            Assert.AreEqual("{ \"_id\" : [0, 1] }", doc.ToString());
        }

        public List<BsonDocument> GenerateDuplicateData()
        {
            List<BsonDocument> insertor = new List<BsonDocument>();
            for (int i = 0; i < 2; i++)
            {
                BsonDocument obj = new BsonDocument();
                obj.Add("_id", 1).
                            Add("operation", "Insert").
                            Add("name", "zhangsan" + i);
                insertor.Add(obj);
            }

            BsonDocument doc = new BsonDocument();
            doc.Add("test16622", "test16622");
            insertor.Add(doc);
            return insertor;
        }

        public void InsertWithOtherFlag()
        {
            cl2.Truncate();
            List<BsonDocument> insertor = new List<BsonDocument>();
            for (int i = 0; i < 2; i++)
            {
                BsonDocument obj = new BsonDocument();
                obj.Add("_id", i).
                            Add("operation", "Insert").
                            Add("name", "zhangsan" + i);
                insertor.Add(obj);
            }
            try
            {
                cl2.Insert(insertor, -2);
            }
            catch (BaseException e)
            {
                Assert.AreEqual(e.ErrorCode, -6, e.ErrorCode + e.Message);
            }
            Assert.AreEqual(0, cl2.GetCount(null));
        }
    
    }
}
