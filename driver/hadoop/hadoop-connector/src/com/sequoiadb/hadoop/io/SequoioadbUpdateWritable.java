package com.sequoiadb.hadoop.io;

import java.io.DataInput;
import java.io.DataOutput;
import java.io.IOException;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;
import org.apache.hadoop.io.Writable;
import org.bson.BSONCallback;
import org.bson.BSONDecoder;
import org.bson.BSONEncoder;
import org.bson.BSONObject;
import org.bson.BasicBSONCallback;
import org.bson.BasicBSONDecoder;
import org.bson.BasicBSONEncoder;
import org.bson.BasicBSONObject;
import org.bson.io.BasicOutputBuffer;
import org.bson.io.Bits;

public class SequoioadbUpdateWritable implements Writable{
	private static final Log LOG = LogFactory.getLog(SequoioadbUpdateWritable.class);
	private BSONObject query;
	private BSONObject modifiers;
	private boolean upsert;

	public SequoioadbUpdateWritable(final BSONObject query, final BSONObject modifiers, final boolean upsert) {
		this.query = query;
		this.modifiers = modifiers;
		this.upsert = upsert;
	}
	
	public SequoioadbUpdateWritable(final BSONObject query, final BSONObject modifiers) {
		this(query, modifiers, true);
	}
	
	
	public BSONObject getQuery() {
		return this.query;
	}
	
	public BSONObject getModifiers() {
		return this.modifiers;
	}
	
	public boolean isUpsert() {
		return this.upsert;
	}
	
	
	@Override
	public void readFields(DataInput in) throws IOException {
		BSONDecoder dec = new BasicBSONDecoder();
		BSONCallback cb = new BasicBSONCallback();
		
		
		byte [] l = new byte[4];
		try {
			in.readFully(l);
			int dataLen = Bits.readInt(l);
			byte [] data = new byte[dataLen + 4];
			System.arraycopy(l, 0, data, 0, 4);
			in.readFully(data, 4, dataLen-4);
			dec.decode(data,  cb);
			this.query = (BSONObject) cb.get();
			
			in.readFully(l);
			dataLen = Bits.readInt(l);
			data = new byte[dataLen + 4];
			System.arraycopy(l, 0, data, 0, 4);
			in.readFully(data, 4, dataLen - 4);
			dec.decode(data, cb);
			this.modifiers = (BSONObject) cb.get();
			
			this.upsert = in.readBoolean();
		} catch (Exception e) {
			LOG.info("No length Header available." + e);
			this.query = new BasicBSONObject();
			this.modifiers = new BasicBSONObject();
		}
        
		
	}

	@Override
	public void write(DataOutput out) throws IOException {
		BSONEncoder enc = new BasicBSONEncoder();
        BasicOutputBuffer buf = new BasicOutputBuffer();
        enc.set( buf );
        enc.putObject( this.query );
        enc.done();
        buf.pipe( out );
        
        enc.set(buf);
		enc.putObject(this.modifiers);
		enc.done();
		
		out.writeBoolean(this.upsert);
	}
	
	@Override
	public boolean equals(final Object obj) {
		if (obj == null || getClass() != obj.getClass()) {
			return false;
		}
		
		if (!(obj instanceof SequoioadbUpdateWritable)) {
			return false;
		}
		
		final SequoioadbUpdateWritable other = (SequoioadbUpdateWritable) obj;
		if (this.upsert != other.upsert) {
			return false;
		}
		
		if (query != other.query 
				|| (query != null && !query.equals(other.query))) {
			return false;
		}
		
		if (modifiers != other.modifiers || (modifiers != null && !modifiers.equals(other.modifiers) )) {
			return false;
		}
		
		return true;
	}
	
	@Override
	public int hashCode() {
		int hashCode = this.query.hashCode();
		hashCode ^= this.modifiers.hashCode();
		hashCode ^= (this.upsert ? 1 : 0) << 1;
		return hashCode;
	}

}
