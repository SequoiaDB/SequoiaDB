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
     *                  ToString ()
     *                  1.插入MaxKey类型的数据
     *                  2.将MaxKey类型数据转换为string类型
     * testcase:    14637
     * author:      chensiqin
     * date:        2019/03/07
    */

    [TestClass]
    public class TestBsonMaxKey14637
    {
        private Sequoiadb sdb = null;
        private CollectionSpace cs = null;
        private DBCollection cl = null;
        private string clName = "cl14637";

        [TestInitialize()]
        public void SetUp()
        {
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " begin: " + this.GetType().ToString());
            sdb = new Sequoiadb(SdbTestBase.coordUrl);
            sdb.Connect();
            cs = sdb.GetCollectionSpace(SdbTestBase.csName);
        }

        [TestMethod]
        public void Test14637()
        {
            if (cs.IsCollectionExist(clName))
            {
                cs.DropCollection(clName);
            }
            cl = cs.CreateCollection(clName);

            BsonDocument record = new BsonDocument();
            BsonMaxKey bsonMaxKey = BsonMaxKey.Value;
            BsonMinKey bsonMinKey = BsonMinKey.Value;
            record.Add("maxKey", bsonMaxKey);
            record.Add("minKey", bsonMinKey);
            cl.Insert(record);
            DBCursor cur = cl.Query();
            while (cur.Next() != null)
            {
                BsonDocument doc = cur.Current();

                Assert.AreEqual(bsonMaxKey, doc.GetElement("maxKey").Value);
                Assert.AreEqual(bsonMaxKey.ToString(), doc.GetElement("maxKey").Value.ToString());

                Assert.AreEqual(bsonMinKey, doc.GetElement("minKey").Value);
                Assert.AreEqual(bsonMinKey.ToString(), doc.GetElement("minKey").Value.ToString());
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
