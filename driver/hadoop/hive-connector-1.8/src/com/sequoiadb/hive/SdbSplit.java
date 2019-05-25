package com.sequoiadb.hive;

import java.io.DataInput;
import java.io.DataOutput;
import java.io.IOException;
import java.util.LinkedList;
import java.util.List;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;
import org.apache.hadoop.fs.Path;
import org.apache.hadoop.mapred.FileInputFormat;
import org.apache.hadoop.mapred.FileSplit;
import org.apache.hadoop.mapred.InputSplit;
import org.apache.hadoop.mapred.JobConf;
import org.bson.BSONObject;
import org.bson.types.BasicBSONList;

import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Node;
import com.sequoiadb.base.ReplicaGroup;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;

public class SdbSplit extends FileSplit implements InputSplit {

	public static final Log LOG = LogFactory.getLog(SdbSerDe.class.getName());

	private static final String[] EMPTY_ARRAY = new String[] {};
	private SdbConnAddr sdbAddr;

	public SdbSplit() {
		super((Path) null, 0, 0, EMPTY_ARRAY);

	}

	public SdbSplit(String host, int port, Path dummyPath) {
		super(dummyPath, 0, 0, EMPTY_ARRAY);
		this.sdbAddr = new SdbConnAddr(host, port);
	}

	@Override
	public void readFields(final DataInput input) throws IOException {
		super.readFields(input);
		this.sdbAddr = new SdbConnAddr();
		sdbAddr.setHost(input.readUTF());
		sdbAddr.setPort(input.readInt());

	}

	@Override
	public void write(final DataOutput output) throws IOException {
		super.write(output);
		output.writeUTF(this.sdbAddr.getHost());
		output.writeInt(this.sdbAddr.getPort());
	}

	public SdbConnAddr getSdbAddr() {
		return this.sdbAddr;
	}

	@Override
	public long getLength() {
		return super.getLength();
	}

	/* Data is remote for all nodes. */
	@Override
	public String[] getLocations() throws IOException {
		return new String[] { sdbAddr.getHost() };
	}

	@Override
	public String toString() {

		return String.format("SdbSplit(sdbaddr=%s)", sdbAddr == null ? "null"
				: sdbAddr.toString());
	}

	public static InputSplit[] getSplits(JobConf conf, int numSplits) {
		LOG.debug("Entry getSplits function");

		SdbConnAddr[] sdbAddrList = ConfigurationUtil
				.getAddrList(ConfigurationUtil.getDBAddr(conf));

		if (sdbAddrList == null || sdbAddrList.length == 0) {
			throw new IllegalArgumentException("The argument "
					+ ConfigurationUtil.DB_ADDR + " must be set.");
		}

		final Path[] tablePaths = FileInputFormat.getInputPaths(conf);

		Sequoiadb sdb = null;
		BaseException lastException = null;

		SdbConnAddr curCoordAddr = null;
		for (SdbConnAddr addr : sdbAddrList) {
			try {
				sdb = new Sequoiadb(addr.getHost(), addr.getPort(), null, null);
				curCoordAddr = addr;
				break;
			} catch (BaseException e) {
				LOG.info("Connect coords error:" + addr);

				lastException = e;
				continue;
			}
		}
		if (sdb == null) {
			LOG.info("Connect coords error:" + lastException);
			throw lastException;
		}

		String spaceName = null;
		String clName = null;
		if (ConfigurationUtil.getCsName(conf) == null
				&& ConfigurationUtil.getClName(conf) == null) {
			spaceName = ConfigurationUtil.getSpaceName(conf);
			clName = ConfigurationUtil.getCollectionName(conf);
		} else {
			spaceName = ConfigurationUtil.getCsName(conf);
			clName = ConfigurationUtil.getClName(conf);
		}
		StringBuilder snapCondBuilder = new StringBuilder();
		String snapCond = snapCondBuilder.append("{Name:\"").append(spaceName)
				.append('.').append(clName).append("\"}").toString();

		List<InputSplit> splits = new LinkedList<InputSplit>();
		List<Integer> groupIDList = new LinkedList<Integer>();
		try {
			DBCursor cursor = sdb.getSnapshot(8, snapCond, null, null);
			while (cursor.hasNext()) {
				BSONObject obj = cursor.getNext();
				LOG.info("Groups record:" + obj.toString());
				BasicBSONList cataInfoList = (BasicBSONList) obj
						.get("CataInfo");
				for (int i = 0; i < cataInfoList.size(); i++) {

					BSONObject cataInfo = (BSONObject) cataInfoList.get(i);
					Integer groupId = (Integer) cataInfo.get("GroupID");

					if (groupIDList.contains(groupId)) {
						continue;
					}
					groupIDList.add(groupId);

					ReplicaGroup group = sdb.getReplicaGroup(groupId);

					Node node = group.getSlave();

					String hostName = node.getHostName();
					Integer port = node.getPort();
					
					splits.add(new SdbSplit(hostName, port, tablePaths[0]));
				}
			}
		} catch (BaseException e) {

			splits.add(new SdbSplit(curCoordAddr.getHost(), curCoordAddr
					.getPort(), tablePaths[0]));
		}

		LOG.info("Exit SdbScanNode::getScanRangeLocations");
		sdb.disconnect();

		return splits.toArray(new InputSplit[splits.size()]);
	}
}
