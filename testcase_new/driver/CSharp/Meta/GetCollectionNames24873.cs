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
     * @Description seqDB-24873:驱动支持cs.listCollections()
     * @Author liuli
     * @Date 2021.12.22
     * @UpdatreAuthor liuli
     * @UpdateDate 2021.12.22
     * @version 1.00
    */
    [TestClass]
    public class GetCollectionNames24873
    {
        private Sequoiadb sdb = null;
        private string csName = "cs24873";
        private string clName = "cl24873_";

        [TestInitialize()]
        public void SetUp()
        {
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " begin: " + this.GetType().ToString());
            sdb = new Sequoiadb(SdbTestBase.coordUrl);
            sdb.Connect();
        }

        [TestMethod()]
        public void Test24873()
        {
            if (Common.isStandalone(sdb))
            {
                return;
            }
            if (sdb.IsCollectionSpaceExist(csName))
            {
                sdb.DropCollectionSpace(csName);
            }
            CollectionSpace dbcs = sdb.CreateCollectionSpace(csName);

            List<string> clNames = new List<string>();
            clNames = dbcs.GetCollectionNames();
            List<string> expected = new List<string>();
            checkList(clNames, expected);

            dbcs.CreateCollection(clName + "0");
            expected.Add(csName + "." + clName + "0");
            clNames = dbcs.GetCollectionNames();
            checkList(clNames, expected);

            for (int i = 1; i < 50; i++)
            {
                dbcs.CreateCollection(clName + i);
                expected.Add(csName + "." + clName + i);
            }
            clNames = dbcs.GetCollectionNames();
            checkList(clNames, expected);
        }

        private void checkList(List<string> list1, List<string> list2)
        {
            list1.Sort();
            list2.Sort();
            int count1 = list1.Count();
            int count2 = list2.Count();
            Assert.AreEqual(count1, count2);
            for (int i = 0; i < count1; i++)
            {
                Assert.AreEqual(list1[i], list2[i]);
            }
        }

        [TestCleanup()]
        public void TearDown()
        {
            sdb.DropCollectionSpace(csName);
            if (sdb != null)
            {
                sdb.Disconnect();
            }
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " end  : " + this.GetType().ToString());
        }
    }
}
