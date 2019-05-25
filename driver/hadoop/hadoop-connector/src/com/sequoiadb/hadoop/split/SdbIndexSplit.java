package com.sequoiadb.hadoop.split;

import java.io.IOException;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;
import org.apache.hadoop.mapreduce.InputSplit;
/**
 * 
 * 
 * @className：SdbIndexSplit
 *
 * @author： gaoshengjie
 *
 * @createtime:2013年12月11日 下午3:51:59
 *
 * @changetime:TODO
 *
 * @version 1.0.0 
 *
 */
public class SdbIndexSplit  extends InputSplit{
	private static final Log log = LogFactory.getLog(SdbIndexSplit.class );
	
	
	@Override
	public long getLength() throws IOException, InterruptedException {
		return 0;
	}

	@Override
	public String[] getLocations() throws IOException, InterruptedException {
		return null;
	}

}
