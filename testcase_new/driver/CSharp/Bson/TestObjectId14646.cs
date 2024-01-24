using System;
using System.Text;
using System.Collections.Generic;
using System.Linq;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using SequoiaDB.Bson;
using SequoiaDB;
using CSharp.TestCommon;

namespace CSharp.Bson
{
    /**
     * description:  
     *               ObjectId (ObjectId value)
     *               ObjectId (byte[] bytes)
     *               ObjectId (DateTime timestamp, int machine, short pid, int increment)
     *               ObjectId (int timestamp, int machine, short pid, int increment)
     *               ObjectId (string value)
     * testcase:     14646
     * author:       chensiqin
     * date:         2019/03/11
    */

    [TestClass]
    public class TestObjectId14646
    {
        private Sequoiadb sdb = null;
        private CollectionSpace cs = null;
        private DBCollection cl = null;
        private string clName = "cl14646";

        [TestInitialize()]
        public void SetUp()
        {
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " begin: " + this.GetType().ToString());
            sdb = new Sequoiadb(SdbTestBase.coordUrl);
            sdb.Connect();
        }

        [TestMethod]
        public void Test14646()
        {
            cs = sdb.GetCollectionSpace(SdbTestBase.csName);
            if (cs.IsCollectionExist(clName))
            {
                cs.DropCollection(clName);
            }
            cl = cs.CreateCollection(clName);

            ObjectId oid1 = new ObjectId("123456789012345678901234");

            byte[] bytes = System.Text.Encoding.Default.GetBytes("hi sequoiadb");
            ObjectId oid2 = new ObjectId(bytes);

            DateTime date = new DateTime(2016, 9, 1);
            ObjectId oid3 = new ObjectId (date, 123, 2233, 5);

            ObjectId oid4 = new ObjectId(1482396787, 123, 2233, 5);

            BsonDocument record = new BsonDocument();
            record.Add("oid1", oid1);
            record.Add("oid2", oid2);
            record.Add("oid3", oid3);
            record.Add("oid4", oid4);
            cl.Insert(record);

            DBCursor cur = cl.Query();
            while (cur.Next() != null)
            {
                BsonDocument doc = cur.Current();
                Assert.AreEqual(record.ToString(), doc.ToString());
            }
            cur.Close();
        }

        [TestCleanup()]
        public void TearDown()
        {
            if (cs.IsCollectionExist(clName))
            {
                cs.DropCollection(clName);
            }
            if (sdb != null)
            {
                sdb.Disconnect();
            }
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " end  : " + this.GetType().ToString());
        }
    }
}
