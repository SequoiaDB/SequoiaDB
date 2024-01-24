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
    * description:   seek and write lob 
    *                  test interface:  seek (long offset, long length) ；void Open(ObjectId id, int mode)
    * testcase:      13466  
    * author:        wuyan
    * date:          2018/3/15
   */
    [TestClass]
    public class Lob13466
    {
        private const string clName = "writeLob13466";
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
        public void TestLob13466()
        {
            int writeLobSize = 1024 * 200;
            int offset = 1024 * 2;         
            testLobBuff = LobUtils.GetRandomBytes(writeLobSize);
            ObjectId oid = SeekAndWriteLob(testLobBuff, offset);

            int rewriteOffset = 1024 * 3;            
		    byte[] rewriteBuff = LobUtils.GetRandomBytes(writeLobSize);           
            	    
		    SeekAndRewriteLob(oid, rewriteBuff, rewriteOffset);			
		    ReadAndcheckResult( oid, rewriteBuff, offset, rewriteOffset);		
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

        private ObjectId SeekAndWriteLob(byte[] writeBuff,int offset)
        {		    
		    DBLob lob = cl.CreateLob() ;			
			lob.Seek(offset, DBLob.SDB_LOB_SEEK_SET);
			lob.Write(writeBuff);		    
			ObjectId oid = lob.GetID();	
	        lob.Close();
            return oid;	
		}	

        private void SeekAndRewriteLob(ObjectId oid, byte[] writeBuff,int offset)
        {		
		    DBLob lob = cl.OpenLob(oid ,DBLob.SDB_LOB_WRITE);		
			lob.Seek(offset, DBLob.SDB_LOB_SEEK_SET);
			lob.Write(writeBuff);
            lob.Close();    
		}	
	
        private void ReadAndcheckResult( ObjectId oid, byte[] rewriteBuff,int offset,int rewriteoffset) 
        {
		    testLobBuff =LobUtils.AppendBuff(testLobBuff, rewriteBuff, rewriteoffset-offset);            
		    byte[] readLobBuff = new byte[testLobBuff.Length];		
		    DBLob lob = cl.OpenLob(oid, DBLob.SDB_LOB_READ);
			lob.Seek(offset, DBLob.SDB_LOB_SEEK_SET);
			lob.Read(readLobBuff);
            lob.Close();            
               
			//check the all write lob             
            LobUtils.AssertByteArrayEqual(readLobBuff, testLobBuff);	
        }
        
    }
}
