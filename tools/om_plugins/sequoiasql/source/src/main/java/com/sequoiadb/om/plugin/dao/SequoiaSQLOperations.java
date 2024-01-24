package com.sequoiadb.om.plugin.dao;

import org.json.JSONObject;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.sql.*;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.List;

public abstract class SequoiaSQLOperations {

    protected final Logger logger = LoggerFactory.getLogger(PostgreSQLOperations.class);
    protected String className = "";
    protected String scheme = "";
    protected String defaultDBName = "";
    protected String defaultUser = "";

    public List<JSONObject> query(String hostName, String svcname,
                                  String user, String pwd,
                                  String dbName, String sql, boolean isAll) throws Exception {
        Connection c = null;
        Statement stmt = null;
        ResultSet rs = null;
        List<JSONObject> content = new ArrayList<JSONObject>();

        Class.forName(className);

        if (dbName == null || dbName.trim().length() == 0) {
            dbName = defaultDBName;
        }

        try {
            c = DriverManager.getConnection(scheme + "://" + hostName + ":" + svcname + "/" + dbName + "?useSSL=false", user, pwd);

            stmt = c.createStatement();

            if (sql.toLowerCase().indexOf("select") == 0) {
                rs = stmt.executeQuery(sql);
                for (int i = 0; (i < 100 || isAll) && rs.next(); ++i) {
                    content.add(resultSet2Json(rs));
                }
            } else {
                stmt.executeUpdate(sql);
            }
        } catch (Exception e) {
            throw e;
        } finally {
            resultSetClose(rs);
            statementClose(stmt);
            ConnectionClose(c);
        }
        return content;
    }

    public String getDefaultUser() {
        return defaultUser;
    }

    protected void resultSetClose(ResultSet rs) {
        try {
            if (rs != null) {
                rs.close();
            }
        } catch (Exception e) {
            logger.warn(e.getMessage());
        }
    }

    protected void statementClose(Statement stmt) {
        try {
            if (stmt != null) {
                stmt.close();
            }
        } catch (Exception e) {
            logger.warn(e.getMessage());
        }
    }

    protected void ConnectionClose(Connection c) {
        try {
            if (c != null) {
                c.close();
            }
        } catch (Exception e) {
            logger.warn(e.getMessage());
        }
    }

    protected JSONObject resultSet2Json(ResultSet rs) throws SQLException {
        JSONObject json = new JSONObject();
        ResultSetMetaData metaData = rs.getMetaData();
        int columnCount = metaData.getColumnCount();


        for (int i = 1; i <= columnCount; i++) {
            String columnName = metaData.getColumnLabel(i);

            if (null == rs.getObject(i)) {
                json.put(columnName, JSONObject.NULL);
            } else {

                switch (metaData.getColumnType(i)) {
                    case Types.ARRAY:
                        json.put(columnName, rs.getArray(i));
                        break;
                    case Types.BIGINT:
                        json.put(columnName, rs.getBigDecimal(i));
                        break;
                    case Types.INTEGER:
                        json.put(columnName, rs.getBigDecimal(i));
                        break;
                    case Types.TINYINT:
                        json.put(columnName, rs.getBigDecimal(i));
                        break;
                    case Types.SMALLINT:
                        json.put(columnName, rs.getBigDecimal(i));
                        break;
                    case Types.BIT:
                    case Types.BOOLEAN:
                        json.put(columnName, rs.getBoolean(i));
                        break;
                    case Types.REAL:
                        json.put(columnName, rs.getBigDecimal(i));
                        break;
                    case Types.FLOAT:
                    case Types.DOUBLE:
                        json.put(columnName, rs.getBigDecimal(i));
                        break;
                    case Types.TIME:
                        json.put(columnName, rs.getTime(i));
                        break;
                    case Types.DATE:
                        json.put(columnName, rs.getDate(i));
                        break;
                    case Types.TIMESTAMP:
                        Timestamp times = rs.getTimestamp(i);
                        if (times != null) {
                            SimpleDateFormat df = new SimpleDateFormat("yyyy-MM-dd HH:mm:ss");
                            json.put(columnName, df.format(times));
                        } else {
                            json.put(columnName, rs.getString(i));
                        }
                        break;
                    case Types.DECIMAL:
                        json.put(columnName, rs.getBigDecimal(i));
                        break;
                    default:
                        json.put(columnName, rs.getString(i));
                        break;

                }
            }
        }

        return json;
    }
}