package com.sequoiadb.hive;

import java.util.ArrayList;
import java.util.List;
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


/**
 * 
 * 
 * @className：BSONWritable
 *
 * @author： chenzichuan
 *
 * @createtime:2013年12月13日 下午4:41:11
 *
 * @changetime:20150630
 *
 * @version 1.0.0 
 *
 */
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
	public static final String SDB_USER = "sdb.user";
	public static final String SDB_PASSWD = "sdb.passwd";

	public static final Set<String> ALL_PROPERTIES = new TreeSet<String>();

	static {
		ALL_PROPERTIES.add(SPACE_NAME);
		ALL_PROPERTIES.add(COLLECTION_NAME);
		ALL_PROPERTIES.add(DB_ADDR);
		ALL_PROPERTIES.add(COLUMN_MAPPING);
		ALL_PROPERTIES.add(BULK_RECOURD_NUM);
		ALL_PROPERTIES.add(CS_NAME);
		ALL_PROPERTIES.add(CL_NAME);
		ALL_PROPERTIES.add(SDB_USER);
		ALL_PROPERTIES.add(SDB_PASSWD);
	}
	
	public static String getUserName (Configuration conf){
		String userName = conf.get(SDB_USER);
		if (userName == null){
			return "";
		}else{
			return userName;
		}
	}
	public static String getPasswd (Configuration conf){
		String passwd = conf.get(SDB_PASSWD);
		if (passwd == null){
			return "";
		}else{
			return passwd;
		}
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

	public final static String getHiveDataBaseName(Configuration conf) {
		String fullTableName = conf.get(COLLECTION_NAME); 
		if (fullTableName == null) {
			return null;
		}
		return fullTableName.substring(0, fullTableName.indexOf(".") ).toLowerCase();
	}

	public final static String getHiveTableName(Configuration conf) {
		String fullTableName = conf.get(COLLECTION_NAME); 
		if (fullTableName == null) {
			return null;
		}
		return fullTableName.substring(fullTableName.indexOf(".") + 1).toLowerCase();
	}

	public final static String getDBAddr(Configuration conf) {
		return conf.get(DB_ADDR);
	}
	public final static List<String> getDBAddrs (String connects){
		List<String> addrList = new ArrayList<String>();
		String[] connArray = null;
		if (connects != null){
			connArray = connects.split(",");
		}
		for(int i=0; i<connArray.length; ++i){
			addrList.add(connArray[i].trim());
		}
		return addrList;
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

//	public static SdbConnAddr[] getAddrList(String addrString) {
//		String addrStrList[] = addrString.split(",");
//
//		SdbConnAddr[] SdbConnAddr = new SdbConnAddr[addrStrList.length];
//
//		int i = 0;
//		for (String addr : addrStrList) {
//			SdbConnAddr[i++] = new SdbConnAddr(addr);
//		}
//
//		return SdbConnAddr;
//	}

}
