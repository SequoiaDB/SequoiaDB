using System;
using System.Text;
using System.Collections.Generic;
using System.Linq;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using SequoiaDB.Bson;
using SequoiaDB;
using CSharp.TestCommon;

namespace CSharp.Meta
{
    /**
     * description: 
     *              createCollection (String collectionName) 
     *              GetCollectionSpace (String csName) 
     *              isCollectionSpaceExist (String csName) 
     *              isCollectionExist (String clName) 
     *              ListCollections ()
     *              GetCollection（String collectinName） 
     *              已创建CS,已分别在CS下创建多个CL
     *              1、获取CSNames，检查返回信息 
     *              2、判断当前CS是否存在 
     *              3、查看cs下所有cl（listcl） 
     *              4、指定cl，判断该cl是否存在 
     * testcase:    14562
     * author:      chensiqin
     * date:        2018/05/03
    */
    [TestClass]
    public class TestCSCL14562
    {
        private Sequoiadb sdb = null;
        private string csName1 = "csName14562_1";
        private string csName2 = "csName14562_2";
        private string csName3 = "csName14562_3";
        private string clName1 = "clName14562_1";
        private string clName2 = "clName14562_2";
        private string clName3 = "clName14562_3";

        [TestInitialize()]
        public void SetUp()
        {
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " begin: " + this.GetType().ToString());
            sdb = new Sequoiadb(SdbTestBase.coordUrl);
            sdb.Connect();
        }

        [TestMethod]
        public void Test14562()
        {
            CollectionSpace cs1 = sdb.CreateCollectionSpace(csName1);
            CollectionSpace cs2 = sdb.CreateCollectionSpace(csName2);
            CollectionSpace cs3 = sdb.CreateCollectionSpace(csName3);

            cs1.CreateCollection(clName1);
            cs1.CreateCollection(clName2);
            cs1.CreateCollection(clName3);

            cs2.CreateCollection(clName1);
            cs2.CreateCollection(clName2);
            cs2.CreateCollection(clName3);

            cs3.CreateCollection(clName1);
            cs3.CreateCollection(clName2);
            cs3.CreateCollection(clName3);

            //获取CSNames，检查返回信息 
            cs1 = sdb.GetCollectionSpace(csName1);
            cs2 = sdb.GetCollectionSpace(csName2);
            cs3 = sdb.GetCollectionSpace(csName3);
            Assert.AreEqual(csName1, cs1.Name);
            Assert.AreEqual(csName2, cs2.Name);
            Assert.AreEqual(csName3, cs3.Name);

            //cl存在
            Assert.IsTrue(cs1.IsCollectionExist(clName1));
            Assert.IsTrue(cs1.IsCollectionExist(clName2));
            Assert.IsTrue(cs1.IsCollectionExist(clName3));

            //判断当前CS是否存在
            Assert.IsTrue(sdb.IsCollectionSpaceExist(csName1));
            Assert.IsTrue(sdb.IsCollectionSpaceExist(csName2));
            Assert.IsTrue(sdb.IsCollectionSpaceExist(csName3));

            //查看cs下所有cl
            DBCursor cur = sdb.ListCollections();
            List<string> actual = new List<string>();

            while (cur.Next() != null)
            {
                BsonDocument doc = cur.Current();
                actual.Add(doc.GetElement("Name").Value.ToString());
            }
            cur.Close();
            Assert.IsTrue(actual.Contains(csName1 + "." + clName1));
            Assert.IsTrue(actual.Contains(csName1 + "." + clName2));
            Assert.IsTrue(actual.Contains(csName1 + "." + clName3));

            Assert.IsTrue(actual.Contains(csName2 + "." + clName1));
            Assert.IsTrue(actual.Contains(csName2 + "." + clName2));
            Assert.IsTrue(actual.Contains(csName2 + "." + clName3));

            Assert.IsTrue(actual.Contains(csName3 + "." + clName1));
            Assert.IsTrue(actual.Contains(csName3 + "." + clName2));
            Assert.IsTrue(actual.Contains(csName3 + "." + clName3));

            sdb.DropCollectionSpace(csName1);
            sdb.DropCollectionSpace(csName2);
            sdb.DropCollectionSpace(csName3);
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
    }
}
