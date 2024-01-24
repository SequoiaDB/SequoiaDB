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
     *                BsonObjectId (ObjectId value)
     *                BsonObjectId (byte[] bytes)
     *                BsonObjectId (DateTime timestamp, int machine, short pid, int increment)
     *                BsonObjectId (int timestamp, int machine, short pid, int increment)
     *                BsonObjectId (string value)
     * testcase:     14613
     * author:       chensiqin
     * date:         2019/03/12
    */

    [TestClass]
    public class TestBsonObjectId14613
    {
        private Sequoiadb sdb = null;
        private CollectionSpace cs = null;
        private DBCollection cl = null;
        private string clName = "cl14613";

        [TestInitialize()]
        public void SetUp()
        {
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " begin: " + this.GetType().ToString());
            sdb = new Sequoiadb(SdbTestBase.coordUrl);
            sdb.Connect();
        }

        [TestMethod]
        public void Test14613()
        {
            cs = sdb.GetCollectionSpace(SdbTestBase.csName);
            if (cs.IsCollectionExist(clName))
            {
                cs.DropCollection(clName);
            }
            cl = cs.CreateCollection(clName);

            ObjectId oid = new ObjectId("123456789012345678901234");
            BsonObjectId boid1 = new BsonObjectId(oid);
            byte[] bytes = System.Text.Encoding.Default.GetBytes("hi sequoiadb");
            BsonObjectId boid2 = new BsonObjectId(bytes);

            DateTime date = new DateTime(2016, 9, 1);
            BsonObjectId boid3 = new BsonObjectId(date, 123, 2233, 5);

            BsonObjectId boid4 = new BsonObjectId(1482396787, 123, 2233, 5);

            BsonObjectId boid5 = new BsonObjectId("123456789012345678901233");
            
            BsonDocument record = new BsonDocument();
            record.Add("boid1", boid1);
            record.Add("boid2", boid2);
            record.Add("boid3", boid3);
            record.Add("boid4", boid4);
            record.Add("boid5", boid5);
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
