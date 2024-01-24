package com.sequoiadb.hive;

import java.util.Map;
import java.util.Properties;
import java.util.Set;
import java.util.TreeSet;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;
import org.apache.hadoop.conf.Configuration;
// import org.apache.hadoop.hive.metastore.api.hive_metastoreConstants;
// import org.apache.hadoop.hive.metastore.api.Constants;
import org.apache.hadoop.mapred.JobConf;

public class ConfigurationUtil {
	public static final Log LOG = LogFactory.getLog(ConfigurationUtil.class.getName());
	
	//The parameter name of database name 
	public static final String SPACE_NAME = "db";
	//The parameter name of table name 
	public static final String COLLECTION_NAME = "name";
	public static final String COLUMN_MAPPING = "columns";
	
	//The space name which defined in table property.
	public static final String CS_NAME = "sdb.space";
	public static final String CL_NAME = "sdb.collection";
	public static final String DB_ADDR = "sdb.address";
	public static final String BULK_RECOURD_NUM = "sdb.bulk.record.num";

	public static final Set<String> ALL_PROPERTIES = new TreeSet<String>();

	static {
		ALL_PROPERTIES.add(SPACE_NAME);
		ALL_PROPERTIES.add(COLLECTION_NAME);
		ALL_PROPERTIES.add(DB_ADDR);
		ALL_PROPERTIES.add(COLUMN_MAPPING);
		ALL_PROPERTIES.add(BULK_RECOURD_NUM);
		ALL_PROPERTIES.add(CS_NAME);
		ALL_PROPERTIES.add(CL_NAME);
	}
	public static String getCsName(Configuration conf){
		String cs_name = conf.get(CS_NAME);
		if( cs_name == null ){
			return null;
		}else{
			return cs_name.toLowerCase();
		}
	}
	public static String getClName(Configuration conf){
		String cl_name = conf.get(CL_NAME);
		if( cl_name == null ){
			return null;
		}
		else{
			return cl_name.toLowerCase();
		}
	}
	public final static int getBulkRecourdNum(Configuration conf) {
		String bulk_record_num = conf.get(BULK_RECOURD_NUM);
		if (bulk_record_num == null) {
			return -1;
		} else {
			return Integer.parseInt(bulk_record_num);
		}
	}

	public final static String getSpaceName(Configuration conf) {
		String fullTableName = conf.get(COLLECTION_NAME); 
		if (fullTableName == null) {
			return null;
		}
		return fullTableName.substring(0, fullTableName.indexOf(".") ).toLowerCase();
	}

	public final static String getCollectionName(Configuration conf) {
		String fullTableName = conf.get(COLLECTION_NAME); 
		if (fullTableName == null) {
			return null;
		}
		return fullTableName.substring(fullTableName.indexOf(".") + 1).toLowerCase();
	}

	public final static String getDBAddr(Configuration conf) {
		return conf.get(DB_ADDR);
	}

	public final static String getColumnMapping(Configuration conf) {
		return conf.get(COLUMN_MAPPING);
	}

	public static void copyProperties(Properties from, Map<String, String> to) {
		
		for(String str: from.stringPropertyNames()) {
			LOG.info(str + ":" + from.getProperty(str));
		}
		
		for (String key : ALL_PROPERTIES) {
			String value = from.getProperty(key);
			if (value != null) {
				to.put(key, value);
			}
		}
	}
	
	public static void copyProperties(Properties from, JobConf to) {
		
		for(String str: from.stringPropertyNames()) {
			LOG.info(str + ":" + from.getProperty(str));
		}
		
		for (String key : ALL_PROPERTIES) {
			String value = from.getProperty(key);
			if (value != null) {
				to.set(key, value);
			}
		}
	}

	public static String[] getAllColumns(String columnMappingString) {
		String columnswithspace[] = columnMappingString.split(",");

		String columnsWithOutSpace[] = new String[columnswithspace.length];
		int i = 0;
		for (String column : columnswithspace) {
			columnsWithOutSpace[i++] = column.trim();
		}

		return columnsWithOutSpace;
	}

	public static SdbConnAddr[] getAddrList(String addrString) {
		String addrStrList[] = addrString.split(",");

		SdbConnAddr[] SdbConnAddr = new SdbConnAddr[addrStrList.length];

		int i = 0;
		for (String addr : addrStrList) {
			SdbConnAddr[i++] = new SdbConnAddr(addr);
		}

		return SdbConnAddr;
	}

}
