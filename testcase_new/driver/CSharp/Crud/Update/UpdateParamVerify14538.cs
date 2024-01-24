using System;
using System.Text;
using System.Collections.Generic;
using System.Linq;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using SequoiaDB;
using CSharp.TestCommon;
using SequoiaDB.Bson;

namespace CSharp.Crud.Update
{
    /**
     * description: parameter verification of Update(), Upsert() and QueryAndUpdate()
     *              interface: Update(...), Upsert(...), QueryAndUpdate(...) 
     * testcase:    14538
     * author:      linsuqiang
     * date:        2018/3/12
     */

    [TestClass]
    public class UpdateParamVerify14538
    {
        private Sequoiadb sdb       = null;
        private CollectionSpace cs  = null;
        private DBCollection cl     = null;
        private const string clName = "cl14538";

        [TestInitialize()]
        public void SetUp()
        {
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " begin: " + this.GetType().ToString());
            sdb = new Sequoiadb(SdbTestBase.coordUrl);
            sdb.Connect();
            cs = sdb.GetCollectionSpace(SdbTestBase.csName);
            BsonDocument options = new BsonDocument("ShardingKey", new BsonDocument("a", 1));
            cl = cs.CreateCollection(clName, options);
        }
        
        [TestMethod()]
        public void TestUpdateParam14538()
        {
            // modifier null
            DBQuery query = new DBQuery();
            query.Matcher = null;
            query.Hint = null;
            query.Modifier = null;
            query.Flag = 0;
            try
            {
                cl.Update(query);
                Assert.Fail("update shouldn't succeed when modifier is null");
            }
            catch (BaseException e)
            {
                if (e.ErrorCode != -6) // SDB_INVALID_ARG
                    throw e;
            }

            try
            {
                cl.Update(/*DBQuery*/null);
                Assert.Fail("update shouldn't succeed when DBQuery is null");
            }
            catch (BaseException e)
            {
                if (e.ErrorCode != -6) // SDB_INVALID_ARG
                    throw e;
            }
        }

        [TestMethod()]
        public void TestUpsertParam14538()
        {
            // modifier null
            try
            {
                cl.Upsert(/*matcher*/null, /*modifier*/null, /*hint*/null, /*setOnInsert*/null, /*flag*/0);
                Assert.Fail("upsert shouldn't succeed when modifier is null");
            }
            catch (BaseException e)
            {
                if (e.ErrorCode != -6) // SDB_INVALID_ARG
                    throw e;
            }
        }

        [TestMethod()]
        public void TestQueryAndUpdateParam14538()
        {
            // update null
            try
            {
                cl.QueryAndUpdate(/*matcher*/null, /*selector*/null, /*orderBy*/null, /*hint*/null, /*update*/null,
                    /*skipRows*/0, /*returnRows*/0, /*flag*/0, /*returnNew*/false);
                Assert.Fail("QueryAndUpdate shouldn't succeed when update is null");
            }
            catch (BaseException e)
            {
                if (e.ErrorCode != -6) // SDB_INVALID_ARG
                    throw e;
            }
        }

        [TestCleanup()]
        public void TearDown()
        {
            try
            {
                cs.DropCollection(clName);
            }
            finally
            {
                if (sdb != null)
                {
                    sdb.Disconnect();    
                }
                Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " end  : " + this.GetType().ToString());
            }
        }
    }
}
