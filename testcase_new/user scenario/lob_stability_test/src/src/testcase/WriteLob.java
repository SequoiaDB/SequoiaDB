package testcase;

import java.util.List;
import java.util.Random;

import org.apache.jmeter.config.Arguments;
import org.apache.jmeter.protocol.java.sampler.AbstractJavaSamplerClient;
import org.apache.jmeter.protocol.java.sampler.JavaSamplerContext;
import org.apache.jmeter.samplers.SampleResult;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;

import common.MyProps;
import common.Utils;

public class WriteLob extends AbstractJavaSamplerClient {
    
    private final String dbUrl     = MyProps.get("dbUrl");
    private final String dbUser    = MyProps.get("dbUser");
    private final String dbPasswd  = MyProps.get("dbPasswd");
    private final int bigNorRatio  = MyProps.getInt("bigNorRatio");
    
    public Arguments getDefaultParameters() {
        Arguments params = new Arguments();
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

        try {
            Sequoiadb db = null;
            try {
	            db = new Sequoiadb(dbUrl, dbUser, dbPasswd);
	            DBCollection cl = Utils.getRandomCL(db);
	            boolean isBig = (0 == new Random().nextInt(bigNorRatio));
	            List<String> filePathList = Utils.getUploadFilePathList(isBig);
	            if (!isBig) {
	                Utils.putNorLobs(cl, filePathList);
	            } else {
	                Utils.putBigLob(cl, filePathList);
	            }
            } finally {
            	if (db != null) { 
            	    db.close(); 
            	}
            }
            
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
//        params.addArgument("UUID", "6213121");
//        params.addArgument("wsName", "ws4");
//        JavaSamplerContext arg0 = new JavaSamplerContext(params);
//        WriteOnAllSite bizUser1 = new WriteOnAllSite();
//        bizUser1.setupTest(arg0);
//        bizUser1.runTest(arg0);
//        bizUser1.teardownTest(arg0);
//    }

}