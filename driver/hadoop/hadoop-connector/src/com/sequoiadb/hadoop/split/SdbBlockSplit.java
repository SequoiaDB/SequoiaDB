package com.sequoiadb.hadoop.split;

import java.io.DataInput;
import java.io.DataOutput;
import java.io.IOException;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;
import org.apache.hadoop.fs.Path;
import org.apache.hadoop.io.Writable;
import org.apache.hadoop.mapreduce.InputSplit;
import org.apache.hadoop.mapreduce.lib.input.FileSplit;

import com.sequoiadb.hadoop.util.SdbConnAddr;

/**
 * 
 * @className：SdbBlockSplit
 *
 * @author： gaoshengjie
 *
 * @createtime:2013年12月11日 下午3:52:06
 *
 * @changetime:TODO
 *
 * @version 1.0.0 
 *
 */
public class SdbBlockSplit extends InputSplit implements Writable,org.apache.hadoop.mapred.InputSplit{
	private static final Log log = LogFactory.getLog( SdbBlockSplit.class );

	private SdbConnAddr sdbAddr;
	private String scanType;
	private int dataBlockId;
	private String collectionSpaceName = null;
	private String collectionName = null;
	
	public SdbBlockSplit() {
		super();
	}

	public SdbBlockSplit(SdbConnAddr sdbAddr, String scanType,
			int dataBlockId, String colletionSpaceName, String collectionName) {
		super();
		this.sdbAddr = sdbAddr;
		this.scanType = scanType;
		this.dataBlockId = dataBlockId;
		this.collectionSpaceName = colletionSpaceName;
		this.collectionName = collectionName;
	}

	public String getCollectionSpaceName()
	{
		return this.collectionSpaceName;
	}
	public String getCollectionName()
	{
		return this.collectionName;
	}
	public SdbConnAddr getSdbAddr() {
		return sdbAddr;
	}

	public void setSdbAddr(SdbConnAddr sdbAddr) {
		this.sdbAddr = sdbAddr;
	}

	public String getScanType() {
		return scanType;
	}

	public void setScanType(String scanType) {
		this.scanType = scanType;
	}

	public int getDataBlockId() {
		return dataBlockId;
	}

	public void setDataBlockId(int dataBlockId) {
		this.dataBlockId = dataBlockId;
	}

	@Override
	public long getLength() {
		return 128*1024*1024;
	}

	@Override
	public String[] getLocations() throws IOException {
		return new String[]{sdbAddr.getHost()};
	}

	@Override
	public void readFields(DataInput in) throws IOException {
		this.sdbAddr=new SdbConnAddr();
		sdbAddr.setHost(in.readUTF());
		sdbAddr.setPort(in.readInt());
		scanType=in.readUTF();
		dataBlockId=in.readInt();
		collectionSpaceName = in.readUTF();
		collectionName = in.readUTF();
	}

	@Override
	public String toString() {
		return "SdbSplit [sdbAddr=" + sdbAddr + ", scanType=" + scanType
				+ ", dataBlockId=" + dataBlockId + "]";
	}

	@Override
	public void write(DataOutput out) throws IOException {
		out.writeUTF(this.sdbAddr.getHost());
		out.writeInt(this.sdbAddr.getPort());
		out.writeUTF(this.scanType);
		out.writeInt(this.dataBlockId);
		out.writeUTF(this.collectionSpaceName);
		out.writeUTF(this.collectionName);
	}


}
