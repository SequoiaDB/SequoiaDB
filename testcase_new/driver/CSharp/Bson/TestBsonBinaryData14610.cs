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
     *                  BsonBinaryData (byte[] bytes)
     *                  BsonBinaryData (byte[] bytes, BsonBinarySubType subType)
     *                  BsonBinaryData (byte[] bytes, BsonBinarySubType subType, GuidRepresentation guidRepresentation)
     *                  BsonBinaryData (Guid guid)
     *                  BsonBinaryData (Guid guid, GuidRepresentation guidRepresentation)
     *                  1.创建二进制对象，通过C#端插入该二进制对象数据，分别通过不同方式创建：
     *                    a、使用默认二进制类型
     *                    b、指定subtype参数
     *                    c、指定subType、guidRepresentation参数
     *                    d、指定Guid参数
     *                    e、指定guid、guidRepresentation参数
     *                  2.查询插入数据是否正确
     * testcase:    14610
     * author:      chensiqin
     * date:        2019/03/11
    */

    [TestClass]
    public class TestBsonBinaryData14610
    {
        private Sequoiadb sdb = null;
        private CollectionSpace cs = null;
        private DBCollection cl = null;
        private string clName = "cl14610";

        [TestInitialize()]
        public void SetUp()
        {
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " begin: " + this.GetType().ToString());
            sdb = new Sequoiadb(SdbTestBase.coordUrl);
            sdb.Connect();
        }

        [TestMethod]
        public void Test14610()
        {
            cs = sdb.GetCollectionSpace(SdbTestBase.csName);
            if (cs.IsCollectionExist(clName))
            {
                cs.DropCollection(clName);
            }
            cl = cs.CreateCollection(clName);
            byte[] bytes = System.Text.Encoding.Default.GetBytes("hello sequoiadb!");
            Guid guid = Guid.NewGuid();
            BsonDocument record = new BsonDocument();
            record.Add("_id", 1);
            record.Add("binary1", new BsonBinaryData(bytes));
            record.Add("binary2", new BsonBinaryData(bytes, BsonBinarySubType.Binary));
            record.Add("binary3", new BsonBinaryData(bytes, BsonBinarySubType.UuidLegacy, GuidRepresentation.CSharpLegacy));
            record.Add("binary4", new BsonBinaryData(guid));
            record.Add("binary5", new BsonBinaryData(guid, GuidRepresentation.CSharpLegacy));
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
