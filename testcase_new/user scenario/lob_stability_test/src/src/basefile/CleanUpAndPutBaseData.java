package basefile;


import java.util.ArrayList;
import java.util.List;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;

import common.MyProps;
import common.Utils;

public class CleanUpAndPutBaseData {
    private final String dbUrl          = MyProps.get("dbUrl");
    private final String dbUser         = MyProps.get("dbUser");
    private final String dbPasswd       = MyProps.get("dbPasswd");
    
    private final int norBaseFileNum    = MyProps.getInt("norBaseFileNum");
    private final int bigBaseFileNum    = MyProps.getInt("bigBaseFileNum");
    
    private List<String> csNameList     = null;
    private List<String> clNameList     = null;
    
	public static void main(String[] args) {
	    boolean shouldCleanUp = false;
	    boolean shouldPutData = false;
	    if (args.length == 0) {
	        System.out.println("--clean [-c], clean up data");
	        System.out.println("--put [-p], put base files");
	        System.out.println("--all [-a], clean up and then put base files");
	        System.exit(-1);
	    }
	    if (args[0].equals("--clean") || args[0].equals("-c")) {
	        shouldCleanUp = true;
	        shouldPutData = false;
	    } else if (args[0].equals("--put") || args[0].equals("-p")) {
	        shouldCleanUp = false;
	        shouldPutData = true;
	    } else if (args[0].equals("--all") || args[0].equals("-a")) {
	        shouldCleanUp = true;
	        shouldPutData = true;
	    } else {
	        System.out.println("unknown option");
	        System.exit(-1);
	    }

		CleanUpAndPutBaseData c = new CleanUpAndPutBaseData();
		if (shouldCleanUp) {
		    System.out.println("---begin to clear up");
		    c.cleanUp();
		}
		if (shouldPutData) {
		    System.out.println("---begin to put base files");
    		try {
    			c.putBaseData();
    		} catch (Exception e) {
    			e.printStackTrace();
    		}
		}
		System.out.println("---end");
	}
	
	public CleanUpAndPutBaseData() {
	    String csNames = MyProps.get("csNames");
	    csNameList = Utils.getListFromString(csNames);
	    String clNames = MyProps.get("clNames");
	    clNameList = Utils.getListFromString(clNames);
	}
	
	public void cleanUp() {
		Sequoiadb db = null;
		try {
			db = new Sequoiadb(dbUrl, dbUser, dbPasswd);
		    for (String csName : csNameList) {
		        for (String clName : clNameList) {
		            db.getCollectionSpace(csName).getCollection(clName).truncate();
		        }
		    }
		} finally {
			if (db != null) {
				db.close();
			}
		}
	}
	
	public void putBaseData() throws InterruptedException {
		List<PutLobThread> threadList = new ArrayList<PutLobThread>();
		
		for (String csName : csNameList) {
		    for (String clName : clNameList) {
		        threadList.add(new PutLobThread(csName, clName));
		    }
		}
		
		for (PutLobThread thd : threadList) {
			thd.start();
		}
		
		for (PutLobThread thd : threadList) {
			thd.join();
		}
	}
	
	public class PutLobThread extends Thread {
		private String csName = null;
		private String clName = null;
		
		public PutLobThread(String csName, String clName) {
			this.csName = csName;
			this.clName = clName;
		}
		
		@Override
		public void run() {
		    Sequoiadb db = null;
			try {
			    db = new Sequoiadb(dbUrl, dbUser, dbPasswd);
			    DBCollection cl = db.getCollectionSpace(csName).getCollection(clName);
				
				List<String> filePathList = Utils.getUploadFilePathList(false);
				final int totalClNum = csNameList.size() * clNameList.size();
				double norFileNumPerCl = Math.ceil((double) norBaseFileNum / totalClNum);
				double writeTimes = Math.ceil(norFileNumPerCl / filePathList.size());
		        for (int i = 0; i < writeTimes; ++i) {
		        	Utils.putNorLobs(cl, filePathList);
		        }
		        
		        double bigFileNumPerCl = Math.ceil((double) bigBaseFileNum / totalClNum);
		        List<String> bigFilePathList = Utils.getUploadFilePathList(true);
		        for (int i = 0; i < bigFileNumPerCl; ++i) {
    		        Utils.putBigLob(cl, bigFilePathList);
		        }
		        
			} catch (Exception e) {
				e.printStackTrace();
			} finally {
				if (null != db) {
					db.close();
				}
			}
		}
	    
	}
	
}
