using System;
using System.Text;
using System.Collections.Generic;
using System.Linq;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using SequoiaDB.Bson;
using SequoiaDB;
using CSharp.TestCommon;

namespace CSharp.Index
{
    /**
     * @Description seqDB-24548:getIndexStat获取指定索引的统计信息
     * @Author liuli
     * @Date 2021.11.03
     * @UpdatreAuthor liuli
     * @UpdateDate 2021.11.03
     * @version 1.00
    */

    [TestClass]
    public class TestIndex24549
    {
        private Sequoiadb sdb = null;
        private string cs_Name = "cs_24549";
        private string cl_Name = "cl_24549";
        private string index_Name = "index_24549";

        [TestInitialize()]
        public void SetUp()
        {
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " begin: " + this.GetType().ToString());
            sdb = new Sequoiadb(SdbTestBase.coordUrl);
            sdb.Connect();
        }

        [TestMethod]
        public void Test24549()
        {
            CollectionSpace cs = sdb.CreateCollectionSpace(cs_Name);
            DBCollection cl = cs.CreateCollection(cl_Name);
            for (int i = 0; i < 200; i++)
            {
                BsonDocument insertor = new BsonDocument();
                insertor.Add("a", i);
                insertor.Add("b", "test"+i);
                cl.Insert(insertor);
            }
            cl.CreateIndex(index_Name, new BsonDocument("a",1), null);

            BsonDocument options = new BsonDocument();
            options.Add("Collection", cs_Name+"."+cl_Name);
            options.Add("Index", index_Name);
            sdb.Analyze(options);

            BsonDocument actResult = cl.GetIndexStat(index_Name);
            actResult.Remove("StatTimestamp");
            actResult.Remove("DistinctValNum");
          
            BsonDocument expResult = new BsonDocument();
            expResult.Add("Collection", cs_Name+"."+cl_Name);
            expResult.Add("Index", index_Name);
            expResult.Add("Unique", false);
            expResult.Add("KeyPattern", new BsonDocument("a", 1));
            expResult.Add("TotalIndexLevels", 1);
            expResult.Add("TotalIndexPages", 1);
            expResult.Add("MinValue", new BsonDocument("a", 0));
            expResult.Add("MaxValue", new BsonDocument("a", 199));
            expResult.Add("NullFrac", 0);
            expResult.Add("UndefFrac", 0);
            expResult.Add("SampleRecords", 200);
            expResult.Add("TotalRecords", 200);
            
            Assert.AreEqual(expResult, actResult);
        }

        [TestCleanup()]
        public void TearDown()
        {
            try
            {
                sdb.DropCollectionSpace(cs_Name);
            }
            finally
            {
                if (sdb != null)
                {
                    sdb.Disconnect();
                }
            }
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " end  : " + this.GetType().ToString());
        }
    }
}
