using System;
using System.Text;
using System.Linq;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using SequoiaDB;
using CSharp.TestCommon;
using SequoiaDB.Bson;

namespace CSharp.Crud.Insert
{
    /**
     * description:   specify oid insert record,and check the insert retrun oid     
     *                  test interface:  insert (BsonDocument record)
     * testcase:      14529  
     * author:        wuyan
     * date:          2018/3/15
    */
    [TestClass]
    public class Insert14529
    {
        private Sequoiadb sdb = null;
        private CollectionSpace cs = null;
        private DBCollection cl = null;
        private string clName = "cl14529";

        [TestInitialize()]
        public void SetUp()
        {      
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " begin: " + this.GetType().ToString());  
            sdb = new Sequoiadb(SdbTestBase.coordUrl);
            sdb.Connect();
            cs = sdb.GetCollectionSpace(SdbTestBase.csName);
            cl = cs.CreateCollection(clName);              
        }
        
        [TestMethod()]
        public void TestInsert14529()
        {
            BsonDocument insertor = new BsonDocument();            
            ObjectId id = new ObjectId("53bb5667c5d061d6f579d0bb");
            insertor.Add("operation", "insert");           
            insertor.Add("_id", id);
            insertor.Add("no", 1);
            ObjectId oid = (ObjectId)cl.Insert(insertor);

            //check the insert result
            DBQuery query = new DBQuery();            
            DBCursor cursor = cl.Query(query);
            BsonDocument actObj = null;            
            while (cursor.Next() != null)
            {
                actObj = cursor.Current();                
            }
            cursor.Close();
            Assert.AreEqual(actObj, insertor);

            //check insert() return value 
            Assert.AreEqual(oid, id); 
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
