package com.sequoiadb.hadoop.util;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;
import org.apache.hadoop.conf.Configuration;
import org.apache.hadoop.mapreduce.InputFormat;
import org.apache.hadoop.mapreduce.Mapper;
import org.apache.hadoop.mapreduce.OutputFormat;
import org.apache.hadoop.mapreduce.Reducer;

/**
 * 
 * 
 * @className锛歋equoiadbConfigUtil
 * 
 * @author锛�gaoshengjie
 * 
 * @createtime:2013骞�2鏈�0鏃�涓嬪崍2:51:46
 * 
 * @changetime:TODO
 * 
 * @version 1.0.0
 * 
 */
public class SequoiadbConfigUtil {
	private static final Log log = LogFactory.getLog(SequoiadbConfigUtil.class);

	public static final String JOB_VERBOSE = "sequoiadb.job.verbose";
	public static final String JOB_BACKGROUND = "sequoiadb.job.background";

	public static final String JOB_MAPPER = "sequoiadb.job.mapper";
	public static final String JOB_COMBINER = "sequoiadb.job.combiner";
	public static final String JOB_PARTITIONER = "sequoiadb.job.partitioner";
	public static final String JOB_REDUCER = "sequoiadb.job.reducer";
	public static final String JOB_SORT_COMPARATOR = "sequoiadb.job.sort_comparator";

	public static final String JOB_INPUT_FORMAT = "sequoiadb.job.input.format";
	public static final String JOB_OUTPUT_FORMAT = "sequoiadb.job.output.format";

	public static final String JOB_OUTPUT_KEY = "sequoiadb.job.output.key";
	public static final String JOB_OUTPUT_VALUE = "sequoiadb.job.output.value";

	public static final String JOB_MAPPER_OUTPUT_KEY = "sequoiadb.job.mapper.output.key";
	public static final String JOB_MAPPER_OUTPUT_VALUE = "sequoiadb.job.mapper.output.value";

	public static final String JOB_INPUT_URL = "sequoiadb.input.url";
	public static final String JOB_INPUT_USER = "sequoiadb.input.user";
	public static final String JOB_INPUT_PASSWD = "sequoiadb.input.passwd";
	public static final String JOB_OUTPUT_URL = "sequoiadb.output.url";
	public static final String JOB_OUTPUT_USER = "sequoiadb.output.user";
	public static final String JOB_OUTPUT_PASSWD = "sequoiadb.output.passwd";
	
	public static final String JOB_IN_COLLECTIONSPACE = "sequoiadb.in.collectionspace";
	public static final String JOB_IN_COLLECTION = "sequoiadb.in.collection";

	public static final String JOB_OUT_COLLECTIONSPACE = "sequoiadb.out.collectionspace";
	public static final String JOB_OUT_COLLECTION = "sequoiadb.out.collection";
	                                               
	public static final String JOB_QUERY_STRING = "sequoiadb.query.json";
	public static final String JOB_SELECTOR_STRING = "sequoiadb.selector.json";
	public static final String JOB_PREFEREDINSTANCE="sequoiadb.preferedinstance";
	public static final String JOB_OUT_BULKNUM = "sequoiadb.out.bulknum";

	public static Class<? extends Mapper> getMapper(Configuration conf) {
		return conf.getClass(JOB_MAPPER, null, Mapper.class);
	}

	public static Class<? extends Reducer> getCombiner(Configuration conf) {
		return conf.getClass(JOB_COMBINER, null, Reducer.class);
	}

	public static Class<? extends Reducer> getReducer(Configuration conf) {
		return conf.getClass(JOB_REDUCER, null, Reducer.class);
	}

	public static Class<? extends OutputFormat> getOutputFormat(
			Configuration conf) {
		return conf.getClass(JOB_OUTPUT_FORMAT, null, OutputFormat.class);
	}

	public static Class<?> getOutputKey(Configuration conf) {
		return conf.getClass(JOB_OUTPUT_KEY, null);
	}

	public static Class<?> getOutputValue(Configuration conf) {
		return conf.getClass(JOB_OUTPUT_VALUE, null);
	}

	public static Class<? extends InputFormat> getInputFormat(Configuration conf) {
		return conf.getClass(JOB_INPUT_FORMAT, null, InputFormat.class);
	}

	public static Class getMapperOutputKey(Configuration conf) {
		return conf.getClass(JOB_MAPPER_OUTPUT_KEY, null);
	}

	public static Class getMapperOutputValue(Configuration conf) {
		return conf.getClass(JOB_MAPPER_OUTPUT_VALUE, null);
	}

	public static boolean isJobVerbose(Configuration conf) {
		return conf.getBoolean(JOB_VERBOSE, false);
	}

	public static boolean isJobBackground(Configuration conf) {
		return conf.getBoolean(JOB_BACKGROUND, false);
	}

	public static String getInputURL(Configuration conf) {
		return conf.get(JOB_INPUT_URL, null);
	}

	public static String getOutputURL(Configuration conf) {
		return conf.get(JOB_OUTPUT_URL, null);
	}

	public static String getOutputBulknum(Configuration conf) {
		return conf.get(JOB_OUT_BULKNUM, null);
	}
	
	public static SdbConnAddr[] getAddrList(String urls) {
		if (urls == null || urls.trim().length() == 0) {
			throw new IllegalArgumentException("the urls is wrong");
		}
		String[] urllist = urls.split(",");
		SdbConnAddr[] addrs = new SdbConnAddr[urllist.length];
		int i = 0;
		for (String url : urllist) {
			addrs[i++] = new SdbConnAddr(url);
		}
		return addrs;
	}

	public static String getInCollectionSpaceName(Configuration conf) {
		return conf.get(JOB_IN_COLLECTIONSPACE, null);
	}

	public static String getInCollectionName(Configuration conf) {
		return conf.get(JOB_IN_COLLECTION, null);
	}

	public static String getOutCollectionSpaceName(Configuration conf) {
		return conf.get(JOB_OUT_COLLECTIONSPACE, null);
	}

	public static String getOutCollectionName(Configuration conf) {
		return conf.get(JOB_OUT_COLLECTION, null);
	}
	
	public static String getQueryString( Configuration conf ){
		return conf.get(JOB_QUERY_STRING, null);
	}
	
	public static String getSelectorString( Configuration conf ){
		return conf.get(JOB_SELECTOR_STRING, null);
	}
	
	public static String  getPreferenceInstance( Configuration conf ){
		return conf.get(JOB_PREFEREDINSTANCE,"anyone");
	}
	public static String getInputUser(Configuration conf){
		return conf.get(JOB_INPUT_USER,null);
	}
	public static String getInputPasswd(Configuration conf){
		return conf.get(JOB_INPUT_PASSWD,null);
	}
	public static String getOutputUser(Configuration conf){
		return conf.get(JOB_OUTPUT_USER,null);
	}
	public static String getOutputPasswd(Configuration conf){
		return conf.get(JOB_OUTPUT_PASSWD,null);
	}
}
