package com.sequoiadb.main;

import java.sql.Connection;
import java.sql.PreparedStatement;
import java.sql.ResultSet;
import java.sql.SQLException;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.concurrent.ArrayBlockingQueue;
import java.util.concurrent.ThreadPoolExecutor;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicInteger;

import org.apache.commons.cli.CommandLine;
import org.apache.commons.cli.CommandLineParser;
import org.apache.commons.cli.HelpFormatter;
import org.apache.commons.cli.Options;
import org.apache.commons.cli.ParseException;
import org.apache.commons.cli.PosixParser;
import org.apache.log4j.Logger;
import org.apache.log4j.PropertyConfigurator;

import com.sequoiadb.service.ConnectDataBase;
import com.sequoiadb.tasks.ReadSdb;

public class SdbMain {
	
public	static Logger logger = Logger.getLogger(SdbMain.class);

public static AtomicInteger paraseSuccess = new AtomicInteger(0);

public static int totalselect = 0;

public static int times = 0;

public static void main(String[] args) {
	
	/**
	 * @param corePoolSize
	 * @param maximumPoolSize
	 * @param keepAliveTime
	 * @param unit
	 * @param workQueue
	 * */
	ThreadPoolExecutor executor = new ThreadPoolExecutor(32, 100, 200, TimeUnit.MILLISECONDS,
            new ArrayBlockingQueue<Runnable>(100));
	PropertyConfigurator.configure("lib/log4j.properties");
	
	Map<String,Object> map=paraseCommand(args);
	if(map != null){
	List<String> obj = (List)map.get("sqlList");
	for(String sql: obj){
	    ReadSdb myTask = new ReadSdb(map,sql);
	    try{
        executor.execute(myTask);
	    }catch(Exception e){
	    	logger.info(e.getMessage());
	    }
    }
    executor.shutdown();
    while(!executor.isTerminated()){
    	if(executor.getTaskCount() >= executor.getCompletedTaskCount()){
    	  ++times;
    	  if(times >= 20){
    		  executor.shutdown();
    	  }
    	}
    	new Thread(
    		 new Runnable() {
				public void run() {
					try {
						Thread.sleep(1000);
					} catch (InterruptedException e) {
						e.printStackTrace();
					}
				}
			}
    	).start();
    }
    logger.info("Finished all threads");
    logger.info("paraseSuccess : "+paraseSuccess);
    logger.info("\n\n");
	}
}

   private static Map<String,Object> paraseCommand(String[] args){
	   
		Options opt = new Options();
		opt.addOption("connect", true, "JDBC URL.");
		opt.addOption("username", true, "DataBase UserName.");
		opt.addOption("password", true, "DataBase password.");
		opt.addOption("table", true, "table name which you want to select.");
		opt.addOption("fieldmodify", true, "modify field name as _id in table");
		opt.addOption("startrow", true, "startRow which you want to select.");
		opt.addOption("endrow", true, "endRow which you want to select.");
		opt.addOption("threads", true, "Concurrent number");
		opt.addOption("sql", true, "sql of user defined");
		opt.addOption("version", false, "information about sdbJdbc");
		opt.addOption("h", "help", false, "print help for the command.");
		String formatstr = "gmkdir [--connect][--username][password][--table][--sumrow][--threads][--sql][-h/--help] DirectoryName";
		String versionstr = "sdbimport-jdbc version  v1.0";
		HelpFormatter formatter = new HelpFormatter();
		CommandLineParser parser = new PosixParser();
		CommandLine cl = null;
		List<String> list = new ArrayList<String>();
		Map<String, Object> map = new HashMap<String, Object>();
		try {
			cl = parser.parse(opt, args);
		} catch (ParseException e) {
			formatter.printHelp(formatstr, opt);
			logger.info(e.getStackTrace(), e);
		}
		// -h or --help
		if (cl == null) {
			logger.info("CommandLine error");
			System.exit(1);
		}
		if (null != cl && cl.hasOption("h")) {
			HelpFormatter hf = new HelpFormatter();
			hf.printHelp(formatstr, "", opt, "",true);
			System.exit(1);
		}
		if (null != cl && cl.hasOption("version")) {
			System.out.println(versionstr);
			System.exit(1);
		}
		// --connect
		if (null != cl && cl.hasOption("connect")) {
			String conStr = cl.getOptionValue("connect");
			int mysqlIndex = conStr.indexOf("oracle");
			int DB2Index = conStr.indexOf("db2");
			if (mysqlIndex > 0)
				map.put("dbType", "oracle");
			if (DB2Index > 0)
				map.put("dbType", "db2");
			map.put("url", cl.getOptionValue("connect"));
		}
		// --username
		if (null != cl && cl.hasOption("username")) {
			map.put("user", cl.getOptionValue("username"));
		}
		// --password
		if (null != cl && cl.hasOption("password")) {
			map.put("password", cl.getOptionValue("password"));
		}
		// --table
		if (null != cl && cl.hasOption("table")) {
			map.put("table", cl.getOptionValue("table"));
		}
		// --startrow
	    if (null != cl && cl.hasOption("startrow")) {
			map.put("startRow", cl.getOptionValue("startrow"));
			}
		// --endrow
		if (null != cl && cl.hasOption("endrow")) {
			map.put("endRow", cl.getOptionValue("endrow"));
		}
		// --threads
		if (null != cl && cl.hasOption("threads")) {
			int thr = Integer.parseInt(cl.getOptionValue("threads"));
			if(thr < 0 || thr>100){
				System.out.println("--threads option value bettween 1~100");
				System.exit(1);
			}
			map.put("threads", cl.getOptionValue("threads"));
		}
		if (map.get("url") == null) {
			System.out.println("could not found --connect option");
			logger.info("could not found --connect option");
			System.exit(1);
		}
		if (map.get("dbType") == null) {
			logger.info("could not found --dbType option");
			System.exit(1);
		}
		if (map.get("user") == null) {
			System.out.println("could not found --username option");
			logger.info("could not found --username option");
			System.exit(1);
		}
		if (map.get("password") == null) {
			System.out.println("could not found --password option");
			logger.info("could not found --password option");
			System.exit(1);
		}
		if(cl != null && !cl.hasOption("sql")){
		if (map.get("table") == null) {
				System.out.println("could not found --table option");
				logger.info("could not found --table option");
				System.exit(1);
		}
		if(map.get("startRow") == null){
			map.put("startRow", 0);
			logger.info("could not found --startrow option");
			logger.info("default query from "+map.get("table") +" start 0 row");
		}
		if (map.get("endRow") == null) {
			int sumrow = queryRowCount(map);
			map.put("endRow",sumrow);
			logger.info("could not found --endrow option");
			logger.info("default query all rows from"+map.get("table"));
		}
		if (map.get("threads") == null) {
			map.put("threads", 1);
			logger.info("could not found --threads option");
		}
		
		int endRow = Integer.parseInt(map.get("endRow").toString());
		int startRow = Integer.parseInt(map.get("startRow").toString());
		int threads = Integer.parseInt(map.get("threads").toString());
		
		int avg = (endRow-startRow) / threads;
		String sql = null;
		String table = map.get("table").toString();
		if(endRow < 0){
			System.out.println("--endRow option value must greater than zero");
			logger.info("--endRow option value must greater than zero");
			System.exit(1);
		}
		if(startRow > endRow){
			System.out.println("--startRow option value could not greater than endRow");
			logger.info("--startRow option value could not greater than endRow");
			System.exit(1);
		}
		
		if(endRow == startRow){
			String sql1 = null;
			if (map.get("dbType").toString() == "oracle")
				sql1 = "select * from (select rownum as rown,t.* from " + table + " t where rownum <=" + endRow
						+ ") tabalias where tabalias.rown >=" + endRow;
			if (map.get("dbType").toString() == "db2")
				sql1 = "select * from (select s.*,rownumber() over() as rn from (select * from " + table
						+ ") as s ) as s1 where s1.rn between " + startRow + " and " + startRow;
			list.add(sql1);
			map.put("sqlList", list);
			return map;
		}
		int start = 1;
		if(startRow >= 1)
		start = startRow;
		int end = start+avg;
		totalselect = endRow-startRow+1;
		if(totalselect < threads){
			logger.info("threads could not bigger than totalselect");
			System.exit(1);
		}
		for (int i = 1; i <= threads; i++) {
			if (i == threads) {
				if (map.get("dbType").toString() == "db2")
					sql = "select * from (select s.*,rownumber() over() as rn from (select * from " + table
							+ ") as s ) as s1 where s1.rn between " + start + " and " + endRow;
				if (map.get("dbType").toString() == "oracle")
					sql = "select * from (select rownum as rown,t.* from " + table + " t where rownum <=" + endRow
							+ ") tabalias where tabalias.rown >=" + start;
			} else {
				if (map.get("dbType").toString() == "db2")
					sql = "select * from (select s.*,rownumber() over() as rn from (select * from " + table
							+ ") as s ) as s1 where s1.rn between " + start + " and " + end;
				if (map.get("dbType").toString() == "oracle")
					sql = "select * from (select rownum as rown,t.* from " + table + " t where rownum <=" + end
					+ ") tabalias where tabalias.rown >=" + start;
			}
			start = end + 1;
			end = (i + 1) * avg+startRow;
			list.add(sql);
		}
		map.put("issql", false);
		}
		if(cl != null && cl.hasOption("sql")){
			String sqlList = cl.getOptionValue("sql");
			String sqlStr[] = sqlList.split(";");
			for(String sql : sqlStr){
			list.add(sql);
			}
		map.put("issql", true);
		}
		if(cl != null && cl.hasOption("fieldmodify")){
			map.put("fieldname", cl.getOptionValue("fieldmodify"));
		}
		map.put("sqlList", list);
		logger.info("args:" + map);
		return map;
   }
   private static int queryRowCount(Map<String,Object> map){
	   ConnectDataBase cdb = new ConnectDataBase(map.get("dbType").toString(), map.get("url").toString(),map.get("user").toString(),map.get("password").toString());
	   Connection conn = null;
	   PreparedStatement pstmt = null;
	   ResultSet rs = null;
	   conn = cdb.getConnection();
	   int rowCount = 0;
	   String queryRowSql = "select count(1) from "+map.get("table");
	   try {
		pstmt = conn.prepareStatement(queryRowSql);
		rs = pstmt.executeQuery();
		while(rs.next()){
			rowCount = Integer.parseInt(rs.getString(1));
		}
	} catch (SQLException e) {
		logger.info("query sumRows from table error");
		logger.info(e.getMessage());
	}
		
	   return rowCount;
   }
}
