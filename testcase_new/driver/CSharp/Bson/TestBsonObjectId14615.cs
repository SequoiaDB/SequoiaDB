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
     *                   ToByteArray ()
     *                   ToString ()
     *                   1.构造objectId类型的数据（覆盖多个不同格式）
     *                   2.将该类型数据转换为string类型
     *                   3、将该类型数据转换为byteArry
     * testcase:     seqDB-14615
     * author:       chensiqin
     * date:         2019/03/15
    */

    [TestClass]
    public class TestBsonObjectId14615
    {
        private Sequoiadb sdb = null;
        private CollectionSpace cs = null;
        private DBCollection cl = null;
        private string clName = "cl14615";

        [TestInitialize()]
        public void SetUp()
        {
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " begin: " + this.GetType().ToString());
            sdb = new Sequoiadb(SdbTestBase.coordUrl);
            sdb.Connect();
        }

        [TestMethod]
        public void Test14615()
        {
            cs = sdb.GetCollectionSpace(SdbTestBase.csName);
            if (cs.IsCollectionExist(clName))
            {
                cs.DropCollection(clName);
            }
            cl = cs.CreateCollection(clName);

            BsonObjectId boid1 = new BsonObjectId("123456789111123456789111");
            byte[] bytes = System.Text.Encoding.Default.GetBytes("123456789111");
            BsonObjectId boid2 = new BsonObjectId(bytes);
            BsonDocument record = new BsonDocument();
            record.Add("boid1", boid1.ToString());
            record.Add("boid2", boid2.ToByteArray());
            record.Add("boid3", boid1.ToString());
            record.Add("boid4", boid2.ToByteArray());
            cl.Insert(record);

            DBCursor cur = cl.Query();
            int count = 0;
            while (cur.Next() != null)
            {
                count++;
                BsonDocument doc = cur.Current();
                //doc.Remove("_id");
                Assert.AreEqual(record, doc);
            }
            cur.Close();
            Assert.AreEqual(1, count);

            string expected = "{ \"boid1\" : \"string\", \"boid2\" : \"bindata\", \"boid3\" : \"string\", \"boid4\" : \"bindata\" }";
            DBQuery query = new DBQuery();
            BsonDocument selector = new BsonDocument();
            selector.Add("boid1", new BsonDocument("$type", 2));
            selector.Add("boid2", new BsonDocument("$type", 2));
            selector.Add("boid3", new BsonDocument("$type", 2));
            selector.Add("boid4", new BsonDocument("$type", 2));
            query.Selector = selector;
            cur = cl.Query(query);
            count = 0;
            while (cur.Next() != null)
            {
                count++;
                BsonDocument doc = cur.Current();
                doc.Remove("_id");
                Assert.AreEqual(expected, doc.ToString());
            }
            cur.Close();
            Assert.AreEqual(1, count);
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
