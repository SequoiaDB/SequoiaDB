using System;
using System.Text;
using System.Collections.Generic;
using System.Linq;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using SequoiaDB.Bson;
using SequoiaDB;
using CSharp.TestCommon;

namespace CSharp.Transaction
{
    /**
     * @Description seqDB-25359:事务中指定SDB_FLG_QUERY_FOR_SHARE查询
     * @Description seqDB-25360:不开起事务使用SDB_FLG_QUERY_FOR_SHARE读数据
     * @Author liuli
     * @Date 2022.02.12
     * @UpdateAuthor liuli
     * @UpdateDate 2022.02.12
     * @version 1.10
     */
    [TestClass]
    public class TestTransaction25359
    {
        private Sequoiadb sdb = null;
        private CollectionSpace dbcs = null;
        private DBCollection dbcl = null;
        private string clName = "cl_25359";
        private string csName = "cs_25359";
        private string IndexName = "index_25359";
        private List<BsonDocument> insertDocs = new List<BsonDocument>();

        [TestInitialize()]
        public void SetUp()
        {
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " begin: " + this.GetType().ToString());
            sdb = new Sequoiadb(SdbTestBase.coordUrl);
            sdb.Connect();
        }

        [TestMethod]
        public void Test25359()
        {
            if (sdb.IsCollectionSpaceExist(csName))
            {
                sdb.DropCollectionSpace(csName);
            }

            dbcs = sdb.CreateCollectionSpace(csName);
            dbcl = dbcs.CreateCollection(clName);
            const int recordNum = 20;
            for (int i = 0; i < recordNum; i++)
            {
                insertDocs.Add(new BsonDocument { { "a", i }, { "b", "hello" } });
            }
            dbcl.BulkInsert(insertDocs, 0);
            dbcl.CreateIndex(IndexName, new BsonDocument("a",1), null);
            DBCursor cursor;
            int lockCount = 0;

            // 修改会话属性
            BsonDocument sessionAttr = new BsonDocument();
            sessionAttr.Add("TransIsolation", 0);
            sessionAttr.Add("TransMaxLockNum", 10);
            sdb.SetSessionAttr(sessionAttr);

            // 开启事务后查询10条数据
            sdb.TransactionBegin();
            BsonDocument hint = new BsonDocument();
            hint.Add("",IndexName);
            BsonDocument query = new BsonDocument();
            query.Add("a", new BsonDocument("$lt", 10));
            cursor = dbcl.Query(query, null, null, hint);
            while (cursor.Next() != null) { }
            cursor.Close();
            // 校验没有记录锁为S锁
            lockCount = GetCLLockCount(sdb,"S");
            Assert.AreEqual(lockCount,0);

            // 指定flags为SDB_FLG_QUERY_FOR_SHARE查询10条数据
            cursor = dbcl.Query(query, null, null, hint, 0, -1, DBQuery.FLG_QUERY_FOR_SHARE);
            while (cursor.Next() != null) { }
            cursor.Close();
            // 校验记录锁中S锁数量为10，集合锁为IS，没有发生锁升级
            lockCount = GetCLLockCount(sdb,"S");
            Assert.AreEqual(lockCount,10);
            CheckIsLockEscalated(sdb,false);
            CheckCLLockType(sdb,"IS");

            // 不指定flags查询后10条数据
            query.Clear();
            query.Add("a",new BsonDocument("$gte",10));
            cursor = dbcl.Query(query, null, null, hint);
            while (cursor.Next() != null) { }
            cursor.Close();
            // 记录锁数量不变，集合锁不变，没有发生锁升级
            lockCount = GetCLLockCount(sdb,"S");
            Assert.AreEqual(lockCount,10);
            CheckIsLockEscalated(sdb,false);
            CheckCLLockType(sdb,"IS");

            // 指定flags为SDB_FLG_QUERY_FOR_SHARE查询后10条数据
            cursor = dbcl.Query(query, null, null, hint, 0, -1, DBQuery.FLG_QUERY_FOR_SHARE);
            while (cursor.Next() != null) { }
            cursor.Close();
            // 发生锁升级，集合锁为S锁
            CheckIsLockEscalated(sdb,true);
            CheckCLLockType(sdb,"S");

            // 事务中指定flags为SDB_FLG_QUERY_FOR_SHARE读取数据并校验
            cursor = dbcl.Query(null, null, new BsonDocument("a", 1), null, 0, -1, DBQuery.FLG_QUERY_FOR_SHARE);
            CheckRecords(insertDocs,cursor);

            // 提交事务
            sdb.TransactionCommit();

            // seqDB-25360:不开起事务使用SDB_FLG_QUERY_FOR_SHARE读数据
            cursor = dbcl.Query(null, null, new BsonDocument("a", 1), null, 0, -1, DBQuery.FLG_QUERY_FOR_SHARE);
            CheckRecords(insertDocs, cursor);
        }

        [TestCleanup()]
        public void TearDown()
        {
            sdb.DropCollectionSpace(csName);
            sdb.Disconnect();
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " end  : " + this.GetType().ToString());
        }

        private int GetCLLockCount(Sequoiadb sdb,String lockType)
        {
            int lockCount = 0;
            DBCursor cursor = sdb.GetSnapshot(SDBConst.SDB_SNAP_TRANSACTIONS_CURRENT,null,null,null);
            BsonDocument record = cursor.Next();
            cursor.Close();
            Assert.AreNotEqual(record,null);
            BsonArray lockList = (BsonArray)record.GetValue("GotLocks");
            foreach (BsonDocument obj in lockList )
            {
                int csID = (int)obj.GetValue("CSID");
                int clID = (int)obj.GetValue("CLID");
                int extentID = (int)obj.GetValue("ExtentID");
                int offset = (int)obj.GetValue("Offset");
                String mode = (String)obj.GetValue("Mode");
                if(csID >= 0 && clID >= 0 && extentID >= 0 && offset >= 0 )
                {
                    if (lockType.Equals(mode))
                    {
                        lockCount++;
                    }
                }
            }
            return lockCount;
        }

        private void CheckIsLockEscalated(Sequoiadb sdb, Boolean isLockEscalated)
        {
            DBCursor cursor = sdb.GetSnapshot(SDBConst.SDB_SNAP_TRANSACTIONS_CURRENT, null, null, null);
            BsonDocument record = cursor.Next();
            cursor.Close();
            Assert.AreNotEqual(record, null);
            Boolean actLockEscalated = (Boolean)record.GetValue("IsLockEscalated");
            Assert.AreEqual(actLockEscalated,isLockEscalated);
        }

        private void CheckCLLockType(Sequoiadb sdb, String lockMode)
        {
            DBCursor cursor = sdb.GetSnapshot(SDBConst.SDB_SNAP_TRANSACTIONS_CURRENT, null, null, null);
            BsonDocument record = cursor.Next();
            cursor.Close();
            Assert.AreNotEqual(record, null);
            BsonArray lockList = (BsonArray)record.GetValue("GotLocks");
            foreach (BsonDocument obj in lockList)
            {
                int csID = (int)obj.GetValue("CSID");
                int clID = (int)obj.GetValue("CLID");
                int extentID = (int)obj.GetValue("ExtentID");
                int offset = (int)obj.GetValue("Offset");
                String mode = (String)obj.GetValue("Mode");
                if (csID >= 0 && clID >= 0 && clID != 65535 && extentID == -1 && offset == -1)
                {
                    Assert.AreEqual(mode,lockMode);
                }
            }
        }

        private void CheckRecords(List<BsonDocument> expRecord, DBCursor cursor)
        {
            List<BsonDocument> result = new List<BsonDocument>();
            while (cursor.Next() != null)
            {
                result.Add(cursor.Current());
            }
            cursor.Close();
            Assert.IsTrue(Common.IsEqual(expRecord, result),
                    "expect:" + expRecord.ToJson() + " actual:" + result.ToJson());
        }
    }
}
