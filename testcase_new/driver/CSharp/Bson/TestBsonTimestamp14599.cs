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
     *                  GetHashCode（）
     *                 1.不同格式的timestamp类型的数据
     *                 2.获取hashCode值，检查是否正确
     * testcase:    14599
     * author:      chensiqin
     * date:        2019/03/06
    */

    [TestClass]
    public class TestBsonTimestamp14599
    {
        private Sequoiadb sdb = null;
        private CollectionSpace cs = null;
        private DBCollection cl = null;
        private string clName = "cl14599";

        [TestInitialize()]
        public void SetUp()
        {
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " begin: " + this.GetType().ToString());
            sdb = new Sequoiadb(SdbTestBase.coordUrl);
            sdb.Connect();
            cs = sdb.GetCollectionSpace(SdbTestBase.csName);
        }

        [TestMethod]
        public void Test14599()
        {
            if (cs.IsCollectionExist(clName))
            {
                cs.DropCollection(clName);
            }
            cl = cs.CreateCollection(clName);

            BsonTimestamp timestamp1 = new BsonTimestamp(1482396787, 999999);
            BsonTimestamp timestamp2 = new BsonTimestamp(6366845719861477951);
            BsonTimestamp timestamp3 = new BsonTimestamp(1482396787, 888888);
            Assert.AreEqual(timestamp1.GetHashCode(), timestamp2.GetHashCode());
            Assert.AreNotEqual(timestamp1.GetHashCode(), timestamp3.GetHashCode());
            
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
