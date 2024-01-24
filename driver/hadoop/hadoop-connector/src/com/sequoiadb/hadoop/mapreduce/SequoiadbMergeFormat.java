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
import org.apache.hadoop.mapreduce.JobContext;
import org.apache.hadoop.mapreduce.OutputCommitter;
import org.apache.hadoop.mapreduce.OutputFormat;
import org.apache.hadoop.mapreduce.RecordWriter;
import org.apache.hadoop.mapreduce.TaskAttemptContext;

import com.sequoiadb.hadoop.io.SequoiadbWriter;
import com.sequoiadb.hadoop.util.SdbConnAddr;
import com.sequoiadb.hadoop.util.SequoiadbConfigUtil;

public class SequoiadbMergeFormat extends OutputFormat implements Configurable {
	private static final Log log = LogFactory
			.getLog(SequoiadbMergeFormat.class);
	private Configuration conf;
	private String collectionSpaceName;
	private String collectionName;
	private SdbConnAddr[] sdbConnAddr;
	private String user;
	private String passwd;
	
	@Override
	public Configuration getConf() {
		// TODO Auto-generated method stub
		return this.conf;
	}

	@Override
	public void setConf(Configuration configuration) {
		// TODO Auto-generated method stub
		this.conf = configuration;
		this.collectionName = SequoiadbConfigUtil.getOutCollectionName(conf);
		this.collectionSpaceName = SequoiadbConfigUtil
				.getOutCollectionSpaceName(conf);
		
		this.user = SequoiadbConfigUtil.getOutputUser(conf);
		this.passwd = SequoiadbConfigUtil.getOutputPasswd(conf);
		
		
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

	@Override
	public void checkOutputSpecs(JobContext jobConf) throws IOException,
			InterruptedException {
		// TODO Auto-generated method stub
		
	}

	@Override
	public OutputCommitter getOutputCommitter(TaskAttemptContext arg0)
			throws IOException, InterruptedException {
		
		System.out.println("OutputCommitter");
		return new SequoiadbOutputCommitter();
	}

	@Override
	public RecordWriter getRecordWriter(TaskAttemptContext arg0)
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
				localAddrList.get(i), user, passwd, "upsert");
	}

}
