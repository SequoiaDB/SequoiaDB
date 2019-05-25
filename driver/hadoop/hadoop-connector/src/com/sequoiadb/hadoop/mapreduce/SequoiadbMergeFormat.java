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
		return this.conf;
	}

	@Override
	public void setConf(Configuration configuration) {
		this.conf = configuration;
		this.collectionName = SequoiadbConfigUtil.getOutCollectionName(conf);
		this.collectionSpaceName = SequoiadbConfigUtil
				.getOutCollectionSpaceName(conf);
		
		this.user = SequoiadbConfigUtil.getOutputUser(conf);
		this.passwd = SequoiadbConfigUtil.getOutputPasswd(conf);
		
		
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
		
		InetAddress localAddr = null;
		try {
			localAddr = InetAddress.getLocalHost();
			log.debug(localAddr.getHostAddress());
		} catch (UnknownHostException e) {
			log.error(e.getMessage());
		}

		ArrayList<SdbConnAddr> localAddrList = new ArrayList<SdbConnAddr>();
		for (int i = 0; i < sdbConnAddr.length; i++) {
			if (sdbConnAddr[i].getHost().equals(localAddr.getHostAddress())
					|| sdbConnAddr[i].getHost().equals(localAddr.getHostName())) {
				localAddrList.add(sdbConnAddr[i]);
			}
		}

		if (localAddrList.isEmpty()) {
			for (int i = 0; i < sdbConnAddr.length; i++) {
				localAddrList.add(sdbConnAddr[i]);
			}
		}

		int i = 0;
		if (localAddrList.size() > 1) {
			Random rand = new Random();
			i = rand.nextInt(localAddrList.size());
		}

		log.debug("Select coord address:" + localAddrList.get(i).toString());

		return new SequoiadbWriter(collectionSpaceName, collectionName,
				localAddrList.get(i), user, passwd, "upsert");
	}

}
