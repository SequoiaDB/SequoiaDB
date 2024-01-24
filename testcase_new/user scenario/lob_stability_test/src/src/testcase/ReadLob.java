package testcase;

import java.io.File;
import java.util.List;
import java.util.Random;

import org.apache.jmeter.config.Arguments;
import org.apache.jmeter.protocol.java.sampler.AbstractJavaSamplerClient;
import org.apache.jmeter.protocol.java.sampler.JavaSamplerContext;
import org.apache.jmeter.samplers.SampleResult;
import org.bson.types.ObjectId;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;

import common.MyProps;
import common.Utils;

public class ReadLob extends AbstractJavaSamplerClient {

	private final String dbUrl     = MyProps.get("dbUrl");
	private final String dbUser    = MyProps.get("dbUser");
	private final String dbPasswd  = MyProps.get("dbPasswd");
	private final int bigNorRatio  = MyProps.getInt("bigNorRatio");
	
	private String downloadDir = null;
    
    public Arguments getDefaultParameters() {
        Arguments params = new Arguments();
        params.addArgument("UUID", "");
        return params;
    }
    
    public void setupTest(JavaSamplerContext arg0) {
    }
    
    @Override
    public SampleResult runTest(JavaSamplerContext arg0) {
        SampleResult results = new SampleResult();
        results.setSampleLabel(this.getClass().getName());
        results.setSuccessful(true);
        results.sampleStart();
        String UUID = arg0.getParameter("UUID");
        
        try {
            downloadDir = MyProps.get("downloadFileDir") + UUID + "/";
            new File(downloadDir).mkdir();
            
            Sequoiadb db = null;
            try {
                db = new Sequoiadb(dbUrl, dbUser, dbPasswd);
                DBCollection cl = Utils.getRandomCL(db);
                boolean isBig = (0 == new Random().nextInt(bigNorRatio));
                List<ObjectId> oidList =  Utils.selectLobOid(cl, isBig);
                if (!isBig) {
                    Utils.readNorLobs(cl, oidList, downloadDir);
                } else {
                    Utils.readBigLobs(cl, oidList, downloadDir);
                }
            } finally {
            	if (db != null) {
            		db.close();
            	}
            }
            
            Utils.deleteDir(downloadDir);
        } catch(Throwable e) {
            e.printStackTrace();
            results.setSuccessful(false);
        } finally {
            results.sampleEnd();
        }
        
        return results;
    }
    
    public void teardownTest(JavaSamplerContext arg0) {
    }
    
//    public static void main(String[] args) {
//        Arguments params = new Arguments();
//        params.addArgument("UUID", "321");
//        JavaSamplerContext arg0 = new JavaSamplerContext(params);
//        ReadOnAllSite bizUser2 = new ReadOnAllSite();
//        bizUser2.setupTest(arg0);
//        bizUser2.runTest(arg0);
//        bizUser2.teardownTest(arg0);
//    }
    
}
