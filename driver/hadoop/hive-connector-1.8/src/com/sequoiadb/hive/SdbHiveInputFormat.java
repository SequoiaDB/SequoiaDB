package com.sequoiadb.hive;

import com.sequoiadb.hive.SdbReader;

import java.io.BufferedWriter;
import java.io.File;
import java.io.FileWriter;
import java.io.IOException;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.util.List;

import org.apache.commons.lang.StringUtils;
import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;
import org.apache.hadoop.conf.Configuration;
import org.apache.hadoop.hive.ql.exec.Utilities;
import org.apache.hadoop.hive.ql.io.HiveInputFormat;
import org.apache.hadoop.hive.ql.plan.ExprNodeDesc;
import org.apache.hadoop.hive.ql.plan.TableScanDesc;
import org.apache.hadoop.hive.serde2.ColumnProjectionUtils;
import org.apache.hadoop.io.BytesWritable;
import org.apache.hadoop.io.LongWritable;
import org.apache.hadoop.mapred.InputSplit;
import org.apache.hadoop.mapred.JobConf;
import org.apache.hadoop.mapred.RecordReader;
import org.apache.hadoop.mapred.Reporter;


public class SdbHiveInputFormat extends
		HiveInputFormat<LongWritable, BytesWritable> {
    private static Log log= LogFactory.getLog(SdbHiveInputFormat.class);
	@Override
	public RecordReader<LongWritable, BytesWritable> getRecordReader(InputSplit inputSplit, JobConf jobConf,
			Reporter Reporter) throws IOException {
		
		Configuration conf = new Configuration( jobConf );
//		List<Integer> readColIDs = ColumnProjectionUtils.getReadColumnIDs(jobConf);
		List<Integer> readColIDs = ColumnProjectionUtils.getReadColumnIDs( conf );
		
		String columnString = jobConf.get(ConfigurationUtil.COLUMN_MAPPING);

		if (StringUtils.isBlank(columnString)) {
			throw new IOException("no column mapping found!");
		}

		String[] columns = ConfigurationUtil.getAllColumns(columnString);
		
		if (readColIDs.size() > columns.length) {
			throw new IOException(
					"read column count larger than that in column mapping string!");
		}

		
		String filterExprSerialized = jobConf
				.get(TableScanDesc.FILTER_EXPR_CONF_STR);
		String filterTextSerialized = jobConf
				.get(TableScanDesc.FILTER_TEXT_CONF_STR);

		ExprNodeDesc filterExpr = null;
		if (filterTextSerialized != null) {

			filterTextSerialized = filterTextSerialized.replaceAll("\'", "\"");

			LOG.debug(TableScanDesc.FILTER_TEXT_CONF_STR + "=" + filterTextSerialized);
			System.out.println(TableScanDesc.FILTER_TEXT_CONF_STR + "=" + filterTextSerialized);

			int useHiveApi_byUtilities = chooseHiveApi_byUtilities();

			switch( useHiveApi_byUtilities ){
			case 0 :
                Class[] parameterTypes_openSourceHive = new Class[2];
                parameterTypes_openSourceHive[0] = String.class;
                parameterTypes_openSourceHive[1] = Configuration.class;


                try {
                    Method method  = Utilities.class.getDeclaredMethod("deserializeExpression", parameterTypes_openSourceHive);
                    filterExpr= (ExprNodeDesc) method.invoke(null,filterExprSerialized, jobConf);
                } catch (NoSuchMethodException e) {
                   e.printStackTrace();
                } catch (InvocationTargetException e) {
                    e.printStackTrace();
                } catch (IllegalAccessException e) {
                    e.printStackTrace();
                }
                break;
			case 1 :
                Class[] parameterTypes_cdh5_0_0beta2Hive = new Class[1];
                parameterTypes_cdh5_0_0beta2Hive[0] = String.class;
                try {
                    Method method = Utilities.class.getDeclaredMethod("deserializeExpression", parameterTypes_cdh5_0_0beta2Hive);
                    filterExpr= (ExprNodeDesc) method.invoke(null,filterExprSerialized);
                } catch (NoSuchMethodException e) {
                    e.printStackTrace();
                } catch (InvocationTargetException e) {
                   e.printStackTrace();
                } catch (IllegalAccessException e) {
                    e.printStackTrace();
                }
				break;
			default :
				break;
			}

		}
		String spaceName = null;
		String colName = null;
		if( ConfigurationUtil.getCsName(jobConf) == null && ConfigurationUtil.getClName(jobConf) == null ){
			spaceName = ConfigurationUtil.getSpaceName(jobConf);
			colName = ConfigurationUtil.getCollectionName(jobConf);
		}else{
			spaceName = ConfigurationUtil.getCsName(jobConf);
			colName = ConfigurationUtil.getClName(jobConf);
		}

        //RecordReader<LongWritable, BytesWritable> sdbr = new SdbReader(spaceName,colName, inputSplit,	columns, readColIDs, filterExpr);
		
		return new SdbReader(spaceName,colName, inputSplit,	columns, readColIDs, filterExpr);
		
	}

	@Override
	public InputSplit[] getSplits(JobConf jobConf, int numSplits) throws IOException {
		return SdbSplit.getSplits(jobConf,numSplits);
	}
	
	/*
	 * If use open source Hive 0.12 , then the Hive Api like this 
	 *        Utilities.deserializeExpression(
					String, Configuration);
	 * If use Cloudera CDH5.0.0 beta2 Hive 0.12 , then the Hive Api like this 
	 *        filterExpr = Utilities.deserializeExpression(
					String);
	 *
	 * The function is check Running in which Hive version
	 * 
	 * If running in open source Hive 0.12 , then return 0
	 * If running in Cloudera CDH5.0.0 beta2 version , then return 1
	 * If running in an unknow Hive version , then return -1
	 */
	private int chooseHiveApi_byUtilities(){
		Class[] parameterTypes_openSourceHive = new Class[2];
		parameterTypes_openSourceHive[0] = String.class;
		parameterTypes_openSourceHive[1] = Configuration.class;
		
		Class[] parameterTypes_cdh5_0_0beta2Hive = new Class[1];
		parameterTypes_cdh5_0_0beta2Hive[0] = String.class;
		
		
		boolean findHiveVersion = false;
		int returnNum = -1;
		
		try {
			Method method = Utilities.class.getDeclaredMethod("deserializeExpression", parameterTypes_openSourceHive);
			findHiveVersion = true;
			returnNum = 0;
		} catch (SecurityException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		} catch (NoSuchMethodException e) {
			// TODO Auto-generated catch block
//			e.printStackTrace();
			try {
				Method method = Utilities.class.getDeclaredMethod("deserializeExpression", parameterTypes_cdh5_0_0beta2Hive);
				findHiveVersion = true;
				returnNum = 1;
			} catch (SecurityException e1) {
				// TODO Auto-generated catch block
				e1.printStackTrace();
			} catch (NoSuchMethodException e1) {
				// TODO Auto-generated catch block
				e1.printStackTrace();
			}
			
		}
		if( findHiveVersion )
			return returnNum;
		
		return returnNum;
	} 

}

