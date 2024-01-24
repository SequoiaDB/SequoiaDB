package com.sequoiadb.hadoop.mapreduce;

import java.io.IOException;
import java.net.InetAddress;
import java.net.UnknownHostException;
import java.util.ArrayList;
import java.util.Random;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;
import org.apache.hadoop.conf.Configurable;
import org.apache.hadoop.conf.Configuration;
import org.apache.hadoop.mapreduce.*;
import com.sequoiadb.hadoop.io.SequoiadbWriter;
import com.sequoiadb.hadoop.util.SdbConnAddr;
import com.sequoiadb.hadoop.util.SequoiadbConfigUtil;

public class SequoiadbOutputFormat extends OutputFormat implements Configurable {
	private static final Log log = LogFactory
			.getLog(SequoiadbInputFormat.class);
	private static final int BULKINSERTNUMBER = 512;

	private String collectionSpaceName;
	private String collectionName;
	private int bulkNum;
	private SdbConnAddr[] sdbConnAddr;
	private String user;
	private String passwd;

	public SequoiadbOutputFormat() {

	}

	@Override
	public void checkOutputSpecs(JobContext jobContext) {

		// The performance is bad?
		// Sequoiadb sequoiadb = new Sequoiadb(sdbConnAddr.getHost(),
		// sdbConnAddr.getPort(), null, null);
		// if (!sequoiadb.isCollectionSpaceExist(collectionSpaceName)) {
		// CollectionSpace cs = sequoiadb
		// .createCollectionSpace(collectionSpaceName);
		// cs.createCollection(collectionName);
		// } else {
		// CollectionSpace cs = sequoiadb
		// .getCollectionSpace(collectionSpaceName);
		// if (!cs.isCollectionExist(collectionName)) {
		// cs.createCollection(collectionName);
		// }
		// }
		// sequoiadb.disconnect();
	}

	@Override
	public OutputCommitter getOutputCommitter(
			TaskAttemptContext taskAttemptContext) throws IOException,
			InterruptedException {
		System.out.println("OutputCommitter");
		return new SequoiadbOutputCommitter();
	}

	@Override
	public RecordWriter getRecordWriter(TaskAttemptContext taskAttemptContext)
			throws IOException, InterruptedException {

		// Find the local coord address from coord adress list.
		// If cann't found, then random select a address.
		InetAddress localAddr = null;
		try {
			localAddr = InetAddress.getLocalHost();
			log.debug(localAddr.getHostAddress());
		} catch (UnknownHostException e) {
			// TODO Auto-generated catch block
			log.error(e.getMessage());
		}

		// Get all location address.
		ArrayList<SdbConnAddr> localAddrList = new ArrayList<SdbConnAddr>();
		for (int i = 0; i < sdbConnAddr.length; i++) {
			if (sdbConnAddr[i].getHost().equals(localAddr.getHostAddress())
					|| sdbConnAddr[i].getHost().equals(localAddr.getHostName())) {
				localAddrList.add(sdbConnAddr[i]);
			}
		}

		// If not any local address, and all address to localAddrList
		if (localAddrList.isEmpty()) {
			for (int i = 0; i < sdbConnAddr.length; i++) {
				localAddrList.add(sdbConnAddr[i]);
			}
		}

		int i = 0;
		// if local address list size is more than one.
		if (localAddrList.size() > 1) {
			// Then genarate random number, for select any one coord
			Random rand = new Random();
			i = rand.nextInt(localAddrList.size());
		}

		log.debug("Select coord address:" + localAddrList.get(i).toString());

		return new SequoiadbWriter(collectionSpaceName, collectionName,
				localAddrList.get(i), user, passwd, bulkNum, "bulkinsert");
	}

	private Configuration conf;

	@Override
	public Configuration getConf() {
		// TODO Auto-generated method stub
		return conf;
	}

	@Override
	public void setConf(Configuration configuration) {
		this.conf = configuration;
		this.collectionName = SequoiadbConfigUtil.getOutCollectionName(conf);
		this.collectionSpaceName = SequoiadbConfigUtil
				.getOutCollectionSpaceName(conf);
		String bulkNumStr = SequoiadbConfigUtil.getOutputBulknum(conf);
		this.user = SequoiadbConfigUtil.getOutputUser(conf);
		this.passwd = SequoiadbConfigUtil.getOutputPasswd(conf);
		
		if (bulkNumStr != null) {
			try{
				this.bulkNum = Integer.valueOf(bulkNumStr);
			}catch( Exception e ){
				log.warn(e.toString());
				log.warn("bulkNum use " + BULKINSERTNUMBER);
				this.bulkNum = BULKINSERTNUMBER;
			}
		}else{
			this.bulkNum = BULKINSERTNUMBER;
		}

		// Process coord url string;
		// The string format is ip:port,ip:port,ip:port
		String urlStr = SequoiadbConfigUtil.getOutputURL(conf);
		if (urlStr == null) {
			throw new IllegalArgumentException("The argument "
					+ SequoiadbConfigUtil.JOB_OUTPUT_URL + " must be set.");
		}
		
		sdbConnAddr = SequoiadbConfigUtil.getAddrList(urlStr);
		if (sdbConnAddr == null || sdbConnAddr.length == 0) {
			throw new IllegalArgumentException("The argument "
					+ SequoiadbConfigUtil.JOB_OUTPUT_URL + " must be set.");
		}
	}

}
