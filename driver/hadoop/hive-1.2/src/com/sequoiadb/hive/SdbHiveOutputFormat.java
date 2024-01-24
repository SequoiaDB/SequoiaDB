package com.sequoiadb.hive;

import java.io.IOException;
import java.util.Properties;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;
import org.apache.hadoop.fs.FileSystem;
import org.apache.hadoop.fs.Path;
import org.apache.hadoop.hive.ql.exec.FileSinkOperator.RecordWriter;
import org.apache.hadoop.hive.ql.io.HiveOutputFormat;
import org.apache.hadoop.io.BytesWritable;
import org.apache.hadoop.io.LongWritable;
import org.apache.hadoop.io.NullWritable;
import org.apache.hadoop.io.Writable;
import org.apache.hadoop.mapred.JobConf;
import org.apache.hadoop.mapred.OutputFormat;
import org.apache.hadoop.util.Progressable;

public class SdbHiveOutputFormat implements HiveOutputFormat<LongWritable, BSONWritable> {

	public static final Log LOG = LogFactory.getLog(SdbHiveOutputFormat.class.getName());
	
//	@Override
	public RecordWriter getHiveRecordWriter(JobConf jobConf, Path finalOutPath,
			Class<? extends Writable> valueClass, boolean isCompressed,
			Properties tableProperties, Progressable progress)
			throws IOException {

		LOG.debug("Entry SdbHiveOutputFormat::getHiveRecordWriter");
		
		String spaceName = null;
		String colName = null;
		String userName = ConfigurationUtil.getUserName (jobConf);
		String passwd = ConfigurationUtil.getPasswd (jobConf);
		
//		if( ConfigurationUtil.getCsName(jobConf) == null && ConfigurationUtil.getClName(jobConf) == null ){
//			spaceName = ConfigurationUtil.getHiveDataBaseName(jobConf);
//			colName = ConfigurationUtil.getHiveTableName(jobConf);
//		}else{
//			spaceName = ConfigurationUtil.getCsName(jobConf);
//			colName = ConfigurationUtil.getClName(jobConf);
//		}
		
		
		
		if (ConfigurationUtil.getCsName(jobConf) == null){
			spaceName = ConfigurationUtil.getHiveDataBaseName(jobConf);
		}else{
			spaceName = ConfigurationUtil.getCsName(jobConf);
		}
		if (ConfigurationUtil.getClName(jobConf) == null){
			colName = ConfigurationUtil.getHiveTableName(jobConf);
		}else{
			colName = ConfigurationUtil.getClName(jobConf);
		}
		
		return (RecordWriter) new SdbWriter(ConfigurationUtil.getDBAddr(jobConf),
				userName,
				passwd,
				spaceName,
				colName, 
				ConfigurationUtil.getBulkRecourdNum(jobConf));

	}

	@Override
	public void checkOutputSpecs(FileSystem arg0, JobConf arg1)
			throws IOException {
		// TODO Auto-generated method stub

	}

	@Override
	public org.apache.hadoop.mapred.RecordWriter<LongWritable, BSONWritable> getRecordWriter(
			FileSystem arg0, JobConf arg1, String arg2, Progressable arg3)
			throws IOException {
		throw new RuntimeException("Error: Hive should not invoke this method.");
	}

}
