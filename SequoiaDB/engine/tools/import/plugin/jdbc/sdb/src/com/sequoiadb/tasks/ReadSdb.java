package com.sequoiadb.tasks;

import java.sql.Connection;
import java.sql.PreparedStatement;
import java.sql.ResultSet;
import java.sql.ResultSetMetaData;
import java.sql.SQLException;
import java.sql.Types;
import java.text.SimpleDateFormat;
import java.util.Map;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

import org.apache.log4j.Logger;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BSONTimestamp;

import com.sequoiadb.main.SdbMain;
import com.sequoiadb.service.ConnectDataBase;

public class ReadSdb implements Runnable {


	private Map<String, Object> map;

	private String sql = null;

	Logger logger = Logger.getLogger(ReadSdb.class);
    

	public ReadSdb() {

	}
	public ReadSdb(Map<String, Object> map, String sql) {

		this.map = map;
		this.sql = sql;
	}

	Connection conn = null;

	PreparedStatement pstmt = null;

	ResultSet rs = null;

	@Override
	public void run() {
		try {
            if(map.get("fieldname") != null)
			read(map.get("dbType").toString(), map.get("url").toString(), map.get("user").toString(),
					map.get("password").toString(), sql,map.get("fieldname").toString(),(Boolean)map.get("issql"));
            else
            	read(map.get("dbType").toString(), map.get("url").toString(), map.get("user").toString(),
    					map.get("password").toString(), sql,null,(Boolean)map.get("issql"));
		} catch (InterruptedException e) {
			logger.info(e.getMessage());
		}
	}
	@SuppressWarnings("unused")
	public void read(String dbType, String url, String user, String password, String sql,String fieldname,boolean flag) throws InterruptedException {

		ConnectDataBase cdb = new ConnectDataBase(dbType, url, user, password);
		conn = cdb.getConnection();
		long startTime = System.currentTimeMillis();
		logger.info("dbType=" + dbType + " url=" + url + " user=" + user + " sql=" + sql+" fieldname="+fieldname);
		try {
			pstmt = conn.prepareStatement(sql);
			rs = pstmt.executeQuery();
			ExecutorService es = Executors.newFixedThreadPool(4);
			while (rs.next()) {
				ResultSetMetaData rsmd = rs.getMetaData();
				BSONObject bson = new BasicBSONObject();
				int i = -1;
				int type = -1;
				try {
					for (i = 1; i <= rsmd.getColumnCount(); i++) {
						String name = rsmd.getColumnName(i);
						type = rsmd.getColumnType(i);
						switch (type) {
						case Types.BIGINT:
							if(fieldname!= null && name.equals(fieldname))
							bson.put("_id", rs.getLong(name));
							else
							bson.put(name, rs.getLong(name));
							break;
						case 101:
							if(fieldname!= null && name.equals(fieldname))
							bson.put("_id", rs.getDouble(name));
							else
							bson.put(name, rs.getDouble(name));
							break;
						case Types.LONGNVARCHAR:
							if(fieldname!= null && name.equals(fieldname))
							bson.put("_id", rs.getLong(name));
							else	
							bson.put(name, rs.getLong(name));
							break;
						case Types.LONGVARCHAR:
							if(fieldname!= null && name.equals(fieldname))
							bson.put("_id", rs.getBigDecimal(name));
							else		
							bson.put(name, rs.getBigDecimal(name));
							break;
						case Types.BOOLEAN:
							bson.put(name, rs.getBoolean(name));
							break;
						case Types.DATE:
							if(fieldname!= null && name.equals(fieldname))
							bson.put("_id", rs.getDate(name));
							else
							bson.put(name, rs.getDate(name));
							break;
						case Types.DOUBLE:
							if(fieldname!= null && name.equals(fieldname))
							bson.put("_id", rs.getDouble(name));
							else
							bson.put(name, rs.getDouble(name));
							break;
						case Types.FLOAT:
							if(fieldname!= null && name.equals(fieldname))
							bson.put("_id", rs.getFloat(name));
							else
							bson.put(name, rs.getFloat(name));
							break;
						case Types.INTEGER:
							if(fieldname!= null && name.equals(fieldname))
							bson.put("_id", rs.getInt(name));
							else
							bson.put(name, rs.getInt(name));
							break;
						case Types.SMALLINT:
							if(fieldname!= null && name.equals(fieldname))
							bson.put("_id", rs.getInt(name));
							else
							bson.put(name, rs.getInt(name));
							break;
						case Types.TIME:
							if(fieldname!= null && name.equals(fieldname))
							bson.put("_id", rs.getTime(name));
							else
							bson.put(name, rs.getTime(name));
							break;
						case Types.TIMESTAMP:
							String tsamp = rs.getObject(name).toString();
							String dateStr = tsamp.substring(0, tsamp.lastIndexOf("."));
							String incStr = tsamp.substring(tsamp.lastIndexOf(".")+1);
							if(!incStr.equals("0")){
							SimpleDateFormat format = new SimpleDateFormat("yyyy-MM-dd HH:mm:ss");
							java.util.Date date = format.parse(dateStr);
							int seconds = (int)(date.getTime()/1000);
							int inc = Integer.parseInt(incStr);
							BSONTimestamp ts = new BSONTimestamp(seconds,inc);
							if(fieldname!= null && name.equals(fieldname))
							bson.put("_id", ts.toString());
							else
							bson.put(name, ts.toString());	
							}else{
								if(fieldname!= null && name.equals(fieldname))
								bson.put("_id", rs.getTimestamp(name));
								else
								bson.put(name, rs.getTimestamp(name));
							}
							break;
						case Types.TINYINT:
							bson.put(name, rs.getShort(name));
							break;
						case Types.VARCHAR:
							if(fieldname!= null && name.equals(fieldname))
							bson.put("_id", rs.getString(name));
							else
							bson.put(name, rs.getString(name));
							break;
						case Types.CHAR:
							if(fieldname!= null && name.equals(fieldname))
							bson.put("_id", rs.getString(name).trim());
							else
							bson.put(name, rs.getString(name).trim());
							break;
						case Types.NCHAR:
							if(fieldname!= null && name.equals(fieldname))
							bson.put("_id", rs.getNString(name));
							else
							bson.put(name, rs.getNString(name));
							break;
						case Types.NVARCHAR:
							if(fieldname!= null && name.equals(fieldname))
							bson.put("_id", rs.getNString(name));
							else
							bson.put(name, rs.getNString(name));
							break;
						case Types.BIT:
							bson.put(name, rs.getByte(name));
							break;
						case Types.BINARY:
							bson.put(name, rs.getBinaryStream(name));
							break;	
						case Types.BLOB:						
							logger.info("unsupport this type of BLOB");
							throw new Exception("unsupport this type of BLOB");
						case Types.CLOB:
							logger.info("unsupport this type of CLOB");
							throw new Exception("unsupport this type of CLOB");
						case Types.NCLOB:
							logger.info("unsupport this type of NCLOB");
							throw new Exception("unsupport this type of NCLOB");
						case Types.NUMERIC:
							if(fieldname!= null && name.equals(fieldname))
							bson.put("_id", rs.getBigDecimal(name));
							else
							if(dbType.equals("oracle") && !name.equals("ROWN"))
							bson.put(name, rs.getBigDecimal(name));
							break;
						case Types.ROWID:
							if(fieldname!= null && name.equals(fieldname))
							bson.put("_id", rs.getRowId(name));
							else
							bson.put(name, rs.getRowId(name));
							break;
						case Types.VARBINARY:
							logger.info("unsupport this type of RAW");
							throw new Exception("unsupport this type of RAW");	
						case -13:
							logger.info("unsupport this type of BFILE");
							throw new Exception("unsupport this type of BFILE");	
						case Types.NULL:
							bson.put(name, null);
							break;
						default:
							logger.info("Field "+"'"+name+"'"+" could not found this type");
							throw new Exception("Field "+"'"+name+"'"+" could not found this type");
						}
					}
				} catch (Exception e) {
					logger.error("failure parase bsonObject: "+bson);
				}
				 
				if(flag == false && bson.toMap().size() == rsmd.getColumnCount()-1){
					System.out.println(bson);
					SdbMain.paraseSuccess.getAndIncrement();
				}
				if(flag == true && bson.toMap().size() == rsmd.getColumnCount()){
					System.out.println(bson);
					SdbMain.paraseSuccess.getAndIncrement();
				}
				
			}
			
		} catch (SQLException e) {
			logger.info(e.getMessage());
			
		} finally {
			try {
				if (pstmt != null)
					pstmt.close();
				if (conn != null)
					conn.close();
				logger.info("costTime:" + (System.currentTimeMillis() - startTime)+" ms");
			} catch (SQLException e) {
				logger.info(e.getMessage());
			}
		}
		
	}
}
