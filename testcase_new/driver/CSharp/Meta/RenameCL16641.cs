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
     * description:  seqDB-16641 renamCL
     * testcase:     16641
     * author:       chensiqin
     * date:         2018/11/20
    */
    [TestClass]
    public class RenameCL16641
    {
        private Sequoiadb sdb = null;
        private CollectionSpace cs = null;
        private string localCLName = "cl16641";
        private string newCLName = "newcl16641";
        
        [TestInitialize()]
        public void SetUp()
        {
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " begin: " + this.GetType().ToString());
            sdb = new Sequoiadb(SdbTestBase.coordUrl);
            sdb.Connect();
            cs = sdb.GetCollectionSpace(SdbTestBase.csName);
        }

        [TestMethod()]
        public void Test16641() 
        {
            cs.CreateCollection(localCLName);
            cs.RenameCollection(localCLName, newCLName);
            DBCollection cl = cs.GetCollection(newCLName);

            BsonDocument doc = new BsonDocument();
            doc.Add("num",1);
            cl.Insert(doc);
            doc = new BsonDocument();
            doc.Add("num",12);
            cl.Insert(doc);
            BsonDocument modifier = new BsonDocument();
            modifier.Add("$inc",new BsonDocument{ { "num",12 } });
            cl.Update(doc, modifier,null, SDBConst.FLG_INSERT_CONTONDUP);
            cl.Delete(new BsonDocument { { "num", 1 } }, null);
            Assert.AreEqual(1, cl.GetCount(new BsonDocument { { "num", 24 } }));
            Assert.AreEqual(false, cs.IsCollectionExist(localCLName));

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
