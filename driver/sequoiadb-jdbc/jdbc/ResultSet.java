package com.sequoiadb.jdbc;

import java.io.InputStream;
import java.io.Reader;
import java.math.BigDecimal;
import java.net.URL;
import java.sql.Array;
import java.sql.Blob;
import java.sql.Clob;
import java.sql.Date;
import java.sql.NClob;
import java.sql.Ref;
import java.sql.ResultSetMetaData;
import java.sql.RowId;
import java.sql.SQLException;
import java.sql.SQLWarning;
import java.sql.SQLXML;
import java.sql.Statement;
import java.sql.Time;
import java.sql.Timestamp;
import java.util.ArrayList;
import java.util.Calendar;
import java.util.Map;
import org.bson.BSONObject;

import com.sequoiadb.base.DBCursor;

public class ResultSet implements java.sql.ResultSet{

	DBCursor cursor = null;
	BSONObject record = null;
	ArrayList<Object> valueArray = null;
	public static int i = 0;
	
	
	public ResultSet(DBCursor cursor){
		this.cursor = cursor;
	}
	
	public <T> T unwrap(Class<T> iface) throws SQLException {
		// TODO Auto-generated method stub
		return null;
	}

	public boolean isWrapperFor(Class<?> iface) throws SQLException {
		// TODO Auto-generated method stub
		return false;
	}
    public Object getColumnType(String columnLabel){
    	return record.get(columnLabel).getClass();
    }
	public boolean next() throws SQLException {
		boolean flg = false;
		if(cursor.hasNext()){
	    record = cursor.getNext(); 
	    valueArray = new ArrayList<>();
	    for(Object ob:record.toMap().values()){
			valueArray.add(ob);
		}
	    flg = true;
		}
		return flg;
	}

	public void close() throws SQLException {
		// TODO Auto-generated method stub
		
	}

	public boolean wasNull() throws SQLException {
		// TODO Auto-generated method stub
		return false;
	}

	public String getString(int columnIndex) throws SQLException {
		if(columnIndex > valueArray.size()-1)
			throw new SQLException("Column Index out of range,"+columnIndex+" > "+valueArray.size());
		if(columnIndex <= 0)
			throw new SQLException("Column Index out of range,"+columnIndex+" < 1");
		String str = (String) valueArray.get(columnIndex);
		return str;
	}

	public boolean getBoolean(int columnIndex) throws SQLException {
		if(columnIndex > valueArray.size()-1)
			throw new SQLException("Column Index out of range,"+columnIndex+" > "+valueArray.size());
		if(columnIndex <= 0)
			throw new SQLException("Column Index out of range,"+columnIndex+" < 1");
		boolean bl = (boolean)valueArray.get(columnIndex);
		return bl;
	}

	public byte getByte(int columnIndex) throws SQLException {
		// TODO Auto-generated method stub
		return 0;
	}

	public short getShort(int columnIndex) throws SQLException {
		// TODO Auto-generated method stub
		return 0;
	}

	public int getInt(int columnIndex) throws SQLException {
		if(columnIndex > valueArray.size()-1)
			throw new SQLException("Column Index out of range,"+columnIndex+" > "+valueArray.size());
		if(columnIndex <= 0)
			throw new SQLException("Column Index out of range,"+columnIndex+" < 1");
		int in = (int)valueArray.get(columnIndex);
		return in;
	}

	public long getLong(int columnIndex) throws SQLException {
		if(columnIndex > valueArray.size()-1)
			throw new SQLException("Column Index out of range,"+columnIndex+" > "+valueArray.size());
		if(columnIndex <= 0)
			throw new SQLException("Column Index out of range,"+columnIndex+" < 1");
		Long l = (Long)valueArray.get(columnIndex);
		return l;
	}

	public float getFloat(int columnIndex) throws SQLException {
		if(columnIndex > valueArray.size()-1)
			throw new SQLException("Column Index out of range,"+columnIndex+" > "+valueArray.size());
		if(columnIndex <= 0)
			throw new SQLException("Column Index out of range,"+columnIndex+" < 1");
		float flat = (float)valueArray.get(columnIndex);
		return flat;
	}

	public double getDouble(int columnIndex) throws SQLException {
		if(columnIndex > valueArray.size()-1)
			throw new SQLException("Column Index out of range,"+columnIndex+" > "+valueArray.size());
		if(columnIndex <= 0)
			throw new SQLException("Column Index out of range,"+columnIndex+" < 1");
		Double db = (Double)valueArray.get(columnIndex);
		return db;
	}

	public BigDecimal getBigDecimal(int columnIndex, int scale)
			throws SQLException {
		// TODO Auto-generated method stub
		return null;
	}

	public byte[] getBytes(int columnIndex) throws SQLException {
		// TODO Auto-generated method stub
		return null;
	}

	public Date getDate(int columnIndex) throws SQLException {
		if(columnIndex > valueArray.size()-1)
			throw new SQLException("Column Index out of range,"+columnIndex+" > "+valueArray.size());
		if(columnIndex <= 0)
			throw new SQLException("Column Index out of range,"+columnIndex+" < 1");
		Date dt = (Date)valueArray.get(columnIndex);
		return dt;
	}

	public Time getTime(int columnIndex) throws SQLException {
		if(columnIndex > valueArray.size()-1)
			throw new SQLException("Column Index out of range,"+columnIndex+" > "+valueArray.size());
		if(columnIndex <= 0)
			throw new SQLException("Column Index out of range,"+columnIndex+" < 1");
		Time tm = (Time)valueArray.get(columnIndex);
		return tm;
	}

	public Timestamp getTimestamp(int columnIndex) throws SQLException {
		if(columnIndex > valueArray.size()-1)
			throw new SQLException("Column Index out of range,"+columnIndex+" > "+valueArray.size());
		if(columnIndex <= 0)
			throw new SQLException("Column Index out of range,"+columnIndex+" < 1");
		Timestamp tsp = (Timestamp)valueArray.get(columnIndex);
		return tsp;
	}

	public InputStream getAsciiStream(int columnIndex) throws SQLException {
		// TODO Auto-generated method stub
		return null;
	}

	public InputStream getUnicodeStream(int columnIndex) throws SQLException {
		// TODO Auto-generated method stub
		return null;
	}

	public InputStream getBinaryStream(int columnIndex) throws SQLException {
		// TODO Auto-generated method stub
		return null;
	}

	public String getString(String columnLabel) throws SQLException {
		if(record.get(columnLabel) == null)
			throw new SQLException("Column "+"'"+columnLabel+"'"+" not found.");
		String StringVal = (String)record.get(columnLabel);
		return StringVal;
	}

	public boolean getBoolean(String columnLabel) throws SQLException {
		if(record.get(columnLabel) == null)
			throw new SQLException("Column "+"'"+columnLabel+"'"+" not found.");
		boolean bl = (boolean)record.get(columnLabel);
		return bl;
	}

	public byte getByte(String columnLabel) throws SQLException {
		if(record.get(columnLabel) == null)
			throw new SQLException("Column "+"'"+columnLabel+"'"+" not found.");
		byte bit = (byte)record.get(columnLabel);
		return bit;
	}

	public short getShort(String columnLabel) throws SQLException {
		// TODO Auto-generated method stub
		return 0;
	}

	public int getInt(String columnLabel) throws SQLException {
		if(record.get(columnLabel) == null)
			throw new SQLException("Column "+"'"+columnLabel+"'"+" not found.");
        int in = (int)record.get(columnLabel);
		return in;
	}

	public long getLong(String columnLabel) throws SQLException {
		if(record.get(columnLabel) == null)
			throw new SQLException("Column "+"'"+columnLabel+"'"+" not found.");
        Long l = (Long)record.get(columnLabel);
		return l;
	}

	public float getFloat(String columnLabel) throws SQLException {
		if(record.get(columnLabel) == null)
			throw new SQLException("Column "+"'"+columnLabel+"'"+" not found.");
		float flat = (float)record.get(columnLabel);
		return flat;
	}

	public double getDouble(String columnLabel) throws SQLException {
		if(record.get(columnLabel) == null)
			throw new SQLException("Column "+"'"+columnLabel+"'"+" not found.");
		Double db = (Double)record.get(columnLabel);
		return db;
	}

	public BigDecimal getBigDecimal(String columnLabel, int scale)
			throws SQLException {
		// TODO Auto-generated method stub
		return null;
	}

	public byte[] getBytes(String columnLabel) throws SQLException {
		// TODO Auto-generated method stub
		return null;
	}

	public Date getDate(String columnLabel) throws SQLException {
		Date dt = null;
		if(record.get(columnLabel) == null)
			throw new SQLException("Column "+"'"+columnLabel+"'"+" not found.");
		else{
		java.util.Date utildate = (java.util.Date)record.get(columnLabel);
	    dt = new Date(utildate.getTime());
		}
		return dt;
	}

	public Time getTime(String columnLabel) throws SQLException {
		// TODO Auto-generated method stub
		return null;
	}

	public Timestamp getTimestamp(String columnLabel) throws SQLException {
		if(record.get(columnLabel) == null)
			throw new SQLException("Column "+"'"+columnLabel+"'"+" not found.");
		Timestamp ts = (Timestamp)record.get(columnLabel);
		return ts;
	}

	public InputStream getAsciiStream(String columnLabel) throws SQLException {
		// TODO Auto-generated method stub
		return null;
	}

	public InputStream getUnicodeStream(String columnLabel) throws SQLException {
		// TODO Auto-generated method stub
		return null;
	}

	public InputStream getBinaryStream(String columnLabel) throws SQLException {
		// TODO Auto-generated method stub
		return null;
	}

	public SQLWarning getWarnings() throws SQLException {
		// TODO Auto-generated method stub
		return null;
	}

	public void clearWarnings() throws SQLException {
		// TODO Auto-generated method stub
		
	}

	public String getCursorName() throws SQLException {
		throw new SQLException("Positioned Update not supported.");
	}

	public ResultSetMetaData getMetaData() throws SQLException {
		// TODO Auto-generated method stub
		return null;
	}

	public Object getObject(int columnIndex) throws SQLException {
		// TODO Auto-generated method stub
		return null;
	}

	public Object getObject(String columnLabel) throws SQLException {
		// TODO Auto-generated method stub
		return null;
	}

	public int findColumn(String columnLabel) throws SQLException {
		if(record.get(columnLabel) == null)
			throw new SQLException("Column "+"'"+columnLabel+"'"+" not found.");
		//TODO 返回列名所在的列数
		return 0;
	}

	public Reader getCharacterStream(int columnIndex) throws SQLException {
		// TODO Auto-generated method stub
		return null;
	}

	public Reader getCharacterStream(String columnLabel) throws SQLException {
		// TODO Auto-generated method stub
		return null;
	}

	public BigDecimal getBigDecimal(int columnIndex) throws SQLException {
		// TODO Auto-generated method stub
		return null;
	}

	public BigDecimal getBigDecimal(String columnLabel) throws SQLException {
		// TODO Auto-generated method stub
		return null;
	}

	public boolean isBeforeFirst() throws SQLException {
		// TODO 光标在第一行之前打印true
		
		return false;
	}

	public boolean isAfterLast() throws SQLException {
		// TODO Auto-generated method stub
		return false;
	}

	public boolean isFirst() throws SQLException {
		// TODO 判断是否在resultset对象的第一行
		return false;
	}

	public boolean isLast() throws SQLException {
		// TODO 判断是否在resultset对象的第最后一行
		return false;
	}

	public void beforeFirst() throws SQLException {
		// TODO 光标上移
		
	}

	public void afterLast() throws SQLException {
		// TODO 光标移到最后一行
		
	}

	public boolean first() throws SQLException {
		// TODO Auto-generated method stub
		return false;
	}

	public boolean last() throws SQLException {
		// TODO Auto-generated method stub
		return false;
	}

	public int getRow() throws SQLException {
		// TODO 返回行号
		return 0;
	}

	public boolean absolute(int row) throws SQLException {
		// TODO 将指针移到给定编号，如果指针位于结果集上,为true,否则为false.
		return false;
	}

	public boolean relative(int rows) throws SQLException {
		// TODO 相对移动rows行，
		return false;
	}

	public boolean previous() throws SQLException {
		// TODO 向前移动
		return false;
	}

	public void setFetchDirection(int direction) throws SQLException {
		// TODO 设置resultset对象中行的处理方向
		
	}

	public int getFetchDirection() throws SQLException {
		// TODO 获取resultset对象中行的处理方向
		return 0;
	}

	public void setFetchSize(int rows) throws SQLException {
		// TODO 为JDBC驱动程序设置此resultset对象需要更多行时应该从数据库获取的行数。
		
	}

	public int getFetchSize() throws SQLException {
		// TODO 获取此resultset对象的获取大小
		return 0;
	}

	public int getType() throws SQLException {
		// TODO 获取此resultset的对象类型
		return 0;
	}

	public int getConcurrency() throws SQLException {
		// TODO 获取此resultset对象的并发模式
		return 0;
	}

	public boolean rowUpdated() throws SQLException {
		throw new NotImplemented();
	}

	public boolean rowInserted() throws SQLException {
		throw new NotImplemented();
	}

	public boolean rowDeleted() throws SQLException {
        throw new NotImplemented();  
	}

	public void updateNull(int columnIndex) throws SQLException {
		// TODO null值更新指定列
		
	}

	public void updateBoolean(int columnIndex, boolean x) throws SQLException {
		// TODO 用boolean 更新指定列
		
	}

	public void updateByte(int columnIndex, byte x) throws SQLException {
		// TODO 用byte更新指定列
		
	}

	public void updateShort(int columnIndex, short x) throws SQLException {
		// TODO Auto-generated method stub
		
	}

	public void updateInt(int columnIndex, int x) throws SQLException {
		// TODO Auto-generated method stub
		
	}

	public void updateLong(int columnIndex, long x) throws SQLException {
		// TODO Auto-generated method stub
		
	}

	public void updateFloat(int columnIndex, float x) throws SQLException {
		// TODO Auto-generated method stub
		
	}

	public void updateDouble(int columnIndex, double x) throws SQLException {
		// TODO Auto-generated method stub
		
	}

	public void updateBigDecimal(int columnIndex, BigDecimal x)
			throws SQLException {
		// TODO Auto-generated method stub
		
	}

	public void updateString(int columnIndex, String x) throws SQLException {
		throw new NotUpdatable();
		
	}

	public void updateBytes(int columnIndex, byte[] x) throws SQLException {
		throw new NotUpdatable();
	}

	public void updateDate(int columnIndex, Date x) throws SQLException {
		throw new NotUpdatable();
	}

	public void updateTime(int columnIndex, Time x) throws SQLException {
		throw new NotUpdatable();
	}

	public void updateTimestamp(int columnIndex, Timestamp x)
			throws SQLException {
		throw new NotUpdatable();
	}

	public void updateAsciiStream(int columnIndex, InputStream x, int length)
			throws SQLException {
		throw new NotUpdatable();
	}

	public void updateBinaryStream(int columnIndex, InputStream x, int length)
			throws SQLException {
		throw new NotUpdatable();
	}

	public void updateCharacterStream(int columnIndex, Reader x, int length)
			throws SQLException {
		throw new NotUpdatable();
	}

	public void updateObject(int columnIndex, Object x, int scaleOrLength)
			throws SQLException {
		throw new NotUpdatable();
	}

	public void updateObject(int columnIndex, Object x) throws SQLException {
		throw new NotUpdatable();
	}

	public void updateNull(String columnLabel) throws SQLException {
		throw new NotUpdatable();
	}

	public void updateBoolean(String columnLabel, boolean x)
			throws SQLException {
		throw new NotUpdatable();
	}

	public void updateByte(String columnLabel, byte x) throws SQLException {
		throw new NotUpdatable();
	}

	public void updateShort(String columnLabel, short x) throws SQLException {
		throw new NotUpdatable();
	}

	public void updateInt(String columnLabel, int x) throws SQLException {
		throw new NotUpdatable();
	}

	public void updateLong(String columnLabel, long x) throws SQLException {
		throw new NotUpdatable();
	}

	public void updateFloat(String columnLabel, float x) throws SQLException {
		throw new NotUpdatable();
	}

	public void updateDouble(String columnLabel, double x) throws SQLException {
		throw new NotUpdatable();
	}

	public void updateBigDecimal(String columnLabel, BigDecimal x)
			throws SQLException {
		throw new NotUpdatable();
	}

	public void updateString(String columnLabel, String x) throws SQLException {
		throw new NotUpdatable();
	}

	public void updateBytes(String columnLabel, byte[] x) throws SQLException {
		throw new NotUpdatable();
	}

	public void updateDate(String columnLabel, Date x) throws SQLException {
		throw new NotUpdatable();
	}

	public void updateTime(String columnLabel, Time x) throws SQLException {
		throw new NotUpdatable();
	}

	public void updateTimestamp(String columnLabel, Timestamp x)
			throws SQLException {
		throw new NotUpdatable();
	}

	public void updateAsciiStream(String columnLabel, InputStream x, int length)
			throws SQLException {
		throw new NotUpdatable();
	}

	public void updateBinaryStream(String columnLabel, InputStream x, int length)
			throws SQLException {
		throw new NotUpdatable();
	}

	public void updateCharacterStream(String columnLabel, Reader reader,
			int length) throws SQLException {
		throw new NotUpdatable();
	}

	public void updateObject(String columnLabel, Object x, int scaleOrLength)
			throws SQLException {
		throw new NotUpdatable();
	}

	public void updateObject(String columnLabel, Object x) throws SQLException {
		throw new NotUpdatable();
	}

	public void insertRow() throws SQLException {
		throw new NotUpdatable();
	}

	public void updateRow() throws SQLException {
		throw new NotUpdatable();
	}

	public void deleteRow() throws SQLException {
		throw new NotUpdatable();
	}

	public void refreshRow() throws SQLException {
		throw new NotUpdatable();
	}

	public void cancelRowUpdates() throws SQLException {
		throw new NotUpdatable();
	}

	public void moveToInsertRow() throws SQLException {
		throw new NotUpdatable();
	}

	public void moveToCurrentRow() throws SQLException {
		throw new NotUpdatable();
	}

	public Statement getStatement() throws SQLException {
		// TODO Auto-generated method stub
		return null;
	}

	public Object getObject(int columnIndex, Map<String, Class<?>> map)
			throws SQLException {
		// TODO Auto-generated method stub
		return null;
	}

	public Ref getRef(int columnIndex) throws SQLException {
	  throw new	NotImplemented();
	}

	public Blob getBlob(int columnIndex) throws SQLException {
		// TODO Auto-generated method stub
		return null;
	}

	public Clob getClob(int columnIndex) throws SQLException {
		// TODO Auto-generated method stub
		return null;
	}

	public Array getArray(int columnIndex) throws SQLException {
		// TODO Auto-generated method stub
		return null;
	}

	public Object getObject(String columnLabel, Map<String, Class<?>> map)
			throws SQLException {
		// TODO Auto-generated method stub
		return null;
	}

	public Ref getRef(String columnLabel) throws SQLException {
		// TODO Auto-generated method stub
		return null;
	}

	public Blob getBlob(String columnLabel) throws SQLException {
		// TODO Auto-generated method stub
		return null;
	}

	public Clob getClob(String columnLabel) throws SQLException {
		// TODO Auto-generated method stub
		return null;
	}

	public Array getArray(String columnLabel) throws SQLException {
		// TODO Auto-generated method stub
		return null;
	}

	public Date getDate(int columnIndex, Calendar cal) throws SQLException {
		// TODO Auto-generated method stub
		return null;
	}

	public Date getDate(String columnLabel, Calendar cal) throws SQLException {
		// TODO Auto-generated method stub
		return null;
	}

	public Time getTime(int columnIndex, Calendar cal) throws SQLException {
		// TODO Auto-generated method stub
		return null;
	}

	public Time getTime(String columnLabel, Calendar cal) throws SQLException {
		// TODO Auto-generated method stub
		return null;
	}

	public Timestamp getTimestamp(int columnIndex, Calendar cal)
			throws SQLException {
		// TODO Auto-generated method stub
		return null;
	}

	public Timestamp getTimestamp(String columnLabel, Calendar cal)
			throws SQLException {
		// TODO Auto-generated method stub
		return null;
	}

	public URL getURL(int columnIndex) throws SQLException {
		// TODO Auto-generated method stub
		return null;
	}

	public URL getURL(String columnLabel) throws SQLException {
		// TODO Auto-generated method stub
		return null;
	}

	public void updateRef(int columnIndex, Ref x) throws SQLException {
		// TODO Auto-generated method stub
		
	}

	public void updateRef(String columnLabel, Ref x) throws SQLException {
		// TODO Auto-generated method stub
		
	}

	public void updateBlob(int columnIndex, Blob x) throws SQLException {
		// TODO Auto-generated method stub
		
	}

	public void updateBlob(String columnLabel, Blob x) throws SQLException {
		// TODO Auto-generated method stub
		
	}

	public void updateClob(int columnIndex, Clob x) throws SQLException {
		// TODO Auto-generated method stub
		
	}

	public void updateClob(String columnLabel, Clob x) throws SQLException {
		// TODO Auto-generated method stub
		
	}

	public void updateArray(int columnIndex, Array x) throws SQLException {
		throw new NotImplemented();
	}

	public void updateArray(String columnLabel, Array x) throws SQLException {
		throw new NotImplemented();
	}

	public RowId getRowId(int columnIndex) throws SQLException {
		// TODO Auto-generated method stub
		return null;
	}

	public RowId getRowId(String columnLabel) throws SQLException {
		// TODO Auto-generated method stub
		return null;
	}

	public void updateRowId(int columnIndex, RowId x) throws SQLException {
		// TODO Auto-generated method stub
		
	}

	public void updateRowId(String columnLabel, RowId x) throws SQLException {
		// TODO Auto-generated method stub
		
	}

	public int getHoldability() throws SQLException {
		// TODO Auto-generated method stub
		return 0;
	}

	public boolean isClosed() throws SQLException {
		// TODO Auto-generated method stub
		return false;
	}

	public void updateNString(int columnIndex, String nString)
			throws SQLException {
		// TODO Auto-generated method stub
		
	}

	public void updateNString(String columnLabel, String nString)
			throws SQLException {
		// TODO Auto-generated method stub
		
	}

	public void updateNClob(int columnIndex, NClob nClob) throws SQLException {
		// TODO Auto-generated method stub
		
	}

	public void updateNClob(String columnLabel, NClob nClob)
			throws SQLException {
		// TODO Auto-generated method stub
		
	}

	public NClob getNClob(int columnIndex) throws SQLException {
		// TODO Auto-generated method stub
		return null;
	}

	public NClob getNClob(String columnLabel) throws SQLException {
		// TODO Auto-generated method stub
		return null;
	}

	public SQLXML getSQLXML(int columnIndex) throws SQLException {
		// TODO Auto-generated method stub
		return null;
	}

	public SQLXML getSQLXML(String columnLabel) throws SQLException {
		// TODO Auto-generated method stub
		return null;
	}

	public void updateSQLXML(int columnIndex, SQLXML xmlObject)
			throws SQLException {
		// TODO Auto-generated method stub
		
	}

	public void updateSQLXML(String columnLabel, SQLXML xmlObject)
			throws SQLException {
		// TODO Auto-generated method stub
		
	}

	public String getNString(int columnIndex) throws SQLException {
		// TODO Auto-generated method stub
		return null;
	}

	public String getNString(String columnLabel) throws SQLException {
		// TODO Auto-generated method stub
		return null;
	}

	public Reader getNCharacterStream(int columnIndex) throws SQLException {
		// TODO Auto-generated method stub
		return null;
	}

	public Reader getNCharacterStream(String columnLabel) throws SQLException {
		// TODO Auto-generated method stub
		return null;
	}

	public void updateNCharacterStream(int columnIndex, Reader x, long length)
			throws SQLException {
		// TODO Auto-generated method stub
		
	}

	public void updateNCharacterStream(String columnLabel, Reader reader,
			long length) throws SQLException {
		// TODO Auto-generated method stub
		
	}

	public void updateAsciiStream(int columnIndex, InputStream x, long length)
			throws SQLException {
		// TODO Auto-generated method stub
		
	}

	public void updateBinaryStream(int columnIndex, InputStream x, long length)
			throws SQLException {
		// TODO Auto-generated method stub
		
	}

	public void updateCharacterStream(int columnIndex, Reader x, long length)
			throws SQLException {
		// TODO Auto-generated method stub
		
	}

	public void updateAsciiStream(String columnLabel, InputStream x, long length)
			throws SQLException {
		// TODO Auto-generated method stub
		
	}

	public void updateBinaryStream(String columnLabel, InputStream x,
			long length) throws SQLException {
		// TODO Auto-generated method stub
		
	}

	public void updateCharacterStream(String columnLabel, Reader reader,
			long length) throws SQLException {
		// TODO Auto-generated method stub
		
	}

	public void updateBlob(int columnIndex, InputStream inputStream, long length)
			throws SQLException {
		// TODO Auto-generated method stub
		
	}

	public void updateBlob(String columnLabel, InputStream inputStream,
			long length) throws SQLException {
		// TODO Auto-generated method stub
		
	}

	public void updateClob(int columnIndex, Reader reader, long length)
			throws SQLException {
		// TODO Auto-generated method stub
		
	}

	public void updateClob(String columnLabel, Reader reader, long length)
			throws SQLException {
		// TODO Auto-generated method stub
		
	}

	public void updateNClob(int columnIndex, Reader reader, long length)
			throws SQLException {
		// TODO Auto-generated method stub
		
	}

	public void updateNClob(String columnLabel, Reader reader, long length)
			throws SQLException {
		// TODO Auto-generated method stub
		
	}

	public void updateNCharacterStream(int columnIndex, Reader x)
			throws SQLException {
		// TODO Auto-generated method stub
		
	}

	public void updateNCharacterStream(String columnLabel, Reader reader)
			throws SQLException {
		// TODO Auto-generated method stub
		
	}

	public void updateAsciiStream(int columnIndex, InputStream x)
			throws SQLException {
		// TODO Auto-generated method stub
		
	}

	public void updateBinaryStream(int columnIndex, InputStream x)
			throws SQLException {
		// TODO Auto-generated method stub
		
	}

	public void updateCharacterStream(int columnIndex, Reader x)
			throws SQLException {
		// TODO Auto-generated method stub
		
	}

	public void updateAsciiStream(String columnLabel, InputStream x)
			throws SQLException {
		// TODO Auto-generated method stub
		
	}

	public void updateBinaryStream(String columnLabel, InputStream x)
			throws SQLException {
		// TODO Auto-generated method stub
		
	}

	public void updateCharacterStream(String columnLabel, Reader reader)
			throws SQLException {
		// TODO Auto-generated method stub
		
	}

	public void updateBlob(int columnIndex, InputStream inputStream)
			throws SQLException {
		// TODO Auto-generated method stub
		
	}

	public void updateBlob(String columnLabel, InputStream inputStream)
			throws SQLException {
		// TODO Auto-generated method stub
		
	}

	public void updateClob(int columnIndex, Reader reader) throws SQLException {
		// TODO Auto-generated method stub
		
	}

	public void updateClob(String columnLabel, Reader reader)
			throws SQLException {
		// TODO Auto-generated method stub
		
	}

	public void updateNClob(int columnIndex, Reader reader) throws SQLException {
		// TODO Auto-generated method stub
		
	}

	public void updateNClob(String columnLabel, Reader reader)
			throws SQLException {
		// TODO Auto-generated method stub
		
	}

	public <T> T getObject(int columnIndex, Class<T> type) throws SQLException {
		// TODO Auto-generated method stub
		return null;
	}

	public <T> T getObject(String columnLabel, Class<T> type)
			throws SQLException {
		// TODO Auto-generated method stub
		return null;
	}

}
