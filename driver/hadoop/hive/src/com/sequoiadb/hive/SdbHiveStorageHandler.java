package com.sequoiadb.hive;

import java.util.Map;
import java.util.Properties;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;
import org.apache.hadoop.conf.Configuration;
import org.apache.hadoop.hive.metastore.HiveMetaHook;
import org.apache.hadoop.hive.metastore.MetaStoreUtils;
import org.apache.hadoop.hive.ql.metadata.HiveException;
import org.apache.hadoop.hive.ql.metadata.HiveStorageHandler;
import org.apache.hadoop.hive.ql.plan.TableDesc;
import org.apache.hadoop.hive.ql.security.authorization.DefaultHiveAuthorizationProvider;
import org.apache.hadoop.hive.ql.security.authorization.HiveAuthorizationProvider;
import org.apache.hadoop.hive.serde2.SerDe;
import org.apache.hadoop.mapred.InputFormat;
import org.apache.hadoop.mapred.JobConf;
import org.apache.hadoop.mapred.OutputFormat;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.Sequoiadb;

public class SdbHiveStorageHandler implements HiveStorageHandler {
	public static final Log LOG = LogFactory.getLog(SdbSerDe.class.getName());
	private Configuration mConf = null;

	@Override
	public void configureInputJobProperties(TableDesc tableDesc,
			Map<String, String> jobProperties) {
		Properties properties = tableDesc.getProperties();
		
		ConfigurationUtil.copyProperties(properties, jobProperties);
	}

	@Override
	public void configureOutputJobProperties(TableDesc tableDesc,
			Map<String, String> jobProperties) {
		Properties properties = tableDesc.getProperties();
		ConfigurationUtil.copyProperties(properties, jobProperties);

	}

	@Override
	@Deprecated
	public void configureTableJobProperties(TableDesc tableDesc,
			Map<String, String> jobProperties) {
		Properties properties = tableDesc.getProperties();
		ConfigurationUtil.copyProperties(properties, jobProperties);
	}

	@Override
	public HiveAuthorizationProvider getAuthorizationProvider()
			throws HiveException {
		return new DefaultHiveAuthorizationProvider();
	}

	@Override
	public Class<? extends InputFormat> getInputFormatClass() {
		return SdbHiveInputFormat.class;
	}

	@Override
	public HiveMetaHook getMetaHook() {
		return new DummyMetaHook();
	}

	@Override
	public Class<? extends OutputFormat> getOutputFormatClass() {
		return SdbHiveOutputFormat.class;
	}

	@Override
	public Class<? extends SerDe> getSerDeClass() {
		return SdbSerDe.class;
	}

	@Override
	public Configuration getConf() {
		return mConf;
	}

	@Override
	public void setConf(Configuration conf) {
		mConf = conf;
	}

	private class DummyMetaHook implements HiveMetaHook {
		

		@Override
		public void commitCreateTable(
				org.apache.hadoop.hive.metastore.api.Table tbl)
				throws org.apache.hadoop.hive.metastore.api.MetaException {
		}

		@Override
		public void commitDropTable(
				org.apache.hadoop.hive.metastore.api.Table tbl,
				boolean deleteData)
				throws org.apache.hadoop.hive.metastore.api.MetaException {
			boolean isExternal = MetaStoreUtils.isExternalTable(tbl);
			
			if( deleteData && !isExternal ){
				
				String dbAddr = tbl.getParameters().get(
						ConfigurationUtil.DB_ADDR);
				String dbCsName = tbl.getParameters().get(ConfigurationUtil.CS_NAME);
				String dbClName = tbl.getParameters().get(ConfigurationUtil.CL_NAME);
				String spaceName = null;
				String dbCollection = null;

				if( dbCsName == null && dbClName == null ){
					spaceName = tbl.getDbName();
					dbCollection = tbl.getTableName();
				}else{
					spaceName = dbCsName;
					dbCollection = dbClName;
				}
				LOG.debug("isExternal is "+isExternal+" and will be droped space's name is "+ spaceName);
				SdbConnAddr[] sdbAddr = ConfigurationUtil.getAddrList(dbAddr);

				Sequoiadb sdb = new Sequoiadb(sdbAddr[0].getHost(),
						sdbAddr[0].getPort(), null, null);
				
				if( sdb.isCollectionSpaceExist(spaceName) ){
					sdb.dropCollectionSpace(spaceName);
				}
				sdb.disconnect();
				
			}

		}

		@Override
		public void preCreateTable(
				org.apache.hadoop.hive.metastore.api.Table tbl)
				throws org.apache.hadoop.hive.metastore.api.MetaException {
			boolean isExternal = MetaStoreUtils.isExternalTable(tbl);
			String dbAddr = tbl.getParameters().get(ConfigurationUtil.DB_ADDR);
			String dbCsName = tbl.getParameters().get(ConfigurationUtil.CS_NAME);
			String dbClName = tbl.getParameters().get(ConfigurationUtil.CL_NAME);
			
			String spaceName = null;
			String dbCollection = null;
			SdbConnAddr[] sdbAddr = ConfigurationUtil.getAddrList(dbAddr);
			
			Sequoiadb sdb = new Sequoiadb(sdbAddr[0].getHost(),
					sdbAddr[0].getPort(), null, null);
			
			if( dbCsName == null && dbClName == null ){
				spaceName = tbl.getDbName();
				dbCollection = tbl.getTableName();
			}else{
				spaceName = dbCsName;
				dbCollection = dbClName;
			}
			

			CollectionSpace space = null;
			if (!sdb.isCollectionSpaceExist(spaceName)) {
				space = sdb.createCollectionSpace(spaceName);
			} else {
				space = sdb.getCollectionSpace(spaceName);
			}

			if (!space.isCollectionExist(dbCollection)) {
				space.createCollection(dbCollection);
			}

			sdb.disconnect();
		}

		@Override
		public void preDropTable(org.apache.hadoop.hive.metastore.api.Table arg0)
				throws org.apache.hadoop.hive.metastore.api.MetaException {

		}

		@Override
		public void rollbackCreateTable(
				org.apache.hadoop.hive.metastore.api.Table arg0)
				throws org.apache.hadoop.hive.metastore.api.MetaException {

		}

		@Override
		public void rollbackDropTable(
				org.apache.hadoop.hive.metastore.api.Table arg0)
				throws org.apache.hadoop.hive.metastore.api.MetaException {

		}

	}

	@Override
	public void configureJobConf(TableDesc tableDesc, JobConf config) {
		Properties properties = tableDesc.getProperties();
		ConfigurationUtil.copyProperties(properties, config);
	}

}
