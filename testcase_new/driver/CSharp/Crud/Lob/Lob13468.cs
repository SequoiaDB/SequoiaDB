using System;
using System.Text;
using System.Linq;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using SequoiaDB;
using CSharp.TestCommon;
using SequoiaDB.Bson;

namespace CSharp.Crud.Lob
{
    /**
    * description:   no seek ,then write lob 
    *                  test interface:  void Open(ObjectId id)
    * testcase:      13468 
    * author:        wuyan
    * date:          2018/3/16
   */
    [TestClass]
    public class Lob13468
    {
        private const string clName = "writeLob13468";
        private Sequoiadb sdb = null;
        private CollectionSpace cs = null;
        private DBCollection cl = null;
        byte[] testLobBuff = null;        

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
        public void TestLob13468()
        {
            int writeLobSize = 1024 * 10;                   
            testLobBuff = LobUtils.GetRandomBytes(writeLobSize);
            ObjectId oid = LobUtils.CreateAndWriteLob( cl, testLobBuff );

            int writeLobSize1 = 1024 * 3;            
		    byte[] rewriteBuff = LobUtils.GetRandomBytes(writeLobSize1);            
		    NoSeekAndRewriteLob( oid, rewriteBuff);			
		    ReadAndcheckResult( oid, rewriteBuff);		
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

        private ObjectId WriteLob(byte[] writeBuff)
        {		    
		    DBLob lob = cl.CreateLob() ;			
			lob.Write(writeBuff);		    
			ObjectId oid = lob.GetID();	
	        lob.Close();
            return oid;	
		}	

        private void NoSeekAndRewriteLob(ObjectId oid, byte[] writeBuff)
        {		
		    DBLob lob = cl.OpenLob(oid , DBLob.SDB_LOB_WRITE);				
			lob.Write(writeBuff);
            lob.Close();    
		}	
	
        private void ReadAndcheckResult( ObjectId oid, byte[] rewriteBuff) 
        {
            //check the all write lob     
		    testLobBuff =LobUtils.AppendBuff(testLobBuff, rewriteBuff, 0);          
		    DBLob lob = cl.OpenLob(oid, DBLob.SDB_LOB_READ);
            byte[] readLobBuff = new byte[lob.GetSize()];	
			lob.Read(readLobBuff);
            lob.Close();  			        
            LobUtils.AssertByteArrayEqual(readLobBuff, testLobBuff);
	
            //check the rewritelob
            byte[] reReadLobBuff = new byte[rewriteBuff.Length];
            DBLob rlob = cl.OpenLob(oid, DBLob.SDB_LOB_READ);
            rlob.Read(reReadLobBuff);
            rlob.Close();
            LobUtils.AssertByteArrayEqual(reReadLobBuff, rewriteBuff);
        }
        
    }
}
