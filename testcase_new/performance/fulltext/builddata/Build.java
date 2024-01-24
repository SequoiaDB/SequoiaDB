package com.sequoiadb.builddata;

import java.io.IOException;
import java.util.Properties;

public enum Build {

    BUILD;

    int total;
    int len;
    int size;
    int arrayLen;
    int thNum;
    int keywords;
    int keywordNum;
    int records;
    int recordNum;
    double percent;
    boolean jmeter;
    boolean jmeter2;
    String coord;
    String csName;
    String clName;
    String filePath;
    String fulltextFileName;
    String sdbQuery100FileName;
    String sdbQuery1FileName;
    String sameRecordFileName;
    String[] hostName;
    String[] fields;

    public static void main(String[] args) throws IOException, BuildException {
        BUILD.build();
    }

    public void build() throws BuildException {
        build(null);
    }

    public void build(Properties prop) throws BuildException {
        try {
            init(prop);
            start();
        } catch (IOException e) {
            e.printStackTrace();
        } finally {
            fini();
        }
    }

    private void init(Properties prop) throws IOException {
        BuildExec.EXECSERVICE.init();
        getProperties(prop);
        if (jmeter) {
            BuildUtils.createFile(filePath + fulltextFileName);
            BuildUtils.createFile(filePath + sdbQuery100FileName);
            BuildUtils.createFile(filePath + sdbQuery1FileName);
        }
        if (jmeter2) {
            BuildUtils.createFile(filePath + sameRecordFileName);
        }
    }

    private void fini() {
        BuildExec.EXECSERVICE.fini();
        System.out.println(BuildMQ.MSGQUEUE.getLenth());
    }

    private void start() throws IOException, BuildException {
        if (jmeter2) {
            total = records;
        }

        int arrayNum = (int) (total * percent);
        int strNum = total - arrayNum;
        int[][] thNeed2Insert = new int[thNum][];
        for (int i = 0; i < thNeed2Insert.length; i++) {
            if (0 == i) {
                thNeed2Insert[i] = new int[] { (strNum / thNum) + (strNum % thNum),
                        (arrayNum / thNum) + (arrayNum % thNum) };
                continue;
            }
            thNeed2Insert[i] = new int[] { strNum / thNum, arrayNum / thNum };
        }

        if (jmeter) {
            PollTh th = new PollTh(keywords, keywordNum, filePath + fulltextFileName, total);
            th.query4Num(filePath + sdbQuery100FileName, 100);
            th.query4Num(filePath + sdbQuery1FileName, 1);
            BuildExec.EXECSERVICE.execute(th);
        }

        for (int i = 0; i < thNeed2Insert.length; i++) {
            String host = hostName[i % hostName.length];
            BuildExec.EXECSERVICE.execute(new InsertTh(host, thNeed2Insert[i]));
        }
    }

    public void getProperties(Properties prop) throws IOException {
        if (prop == null) {
            prop = new Properties();
            prop.load(Build.class.getResourceAsStream("/build.properties"));
        }

        String total = prop.getProperty("total");
        String percent = prop.getProperty("percent");
        String len = prop.getProperty("len");
        String size = prop.getProperty("size");
        String arraylen = prop.getProperty("arraylen");
        String thnum = prop.getProperty("thnum");
        String hostname = prop.getProperty("hostname");
        String coord = prop.getProperty("coord");
        String csname = prop.getProperty("csname");
        String clname = prop.getProperty("clname");
        String fields = prop.getProperty("fields");

        String jmeter = prop.getProperty("jmeter");
        String keywords = prop.getProperty("keywords");
        String keywordnum = prop.getProperty("keywordnum");
        String filepath = prop.getProperty("filepath");
        String fulltextfilename = prop.getProperty("fulltextfilename");
        String sdbquery100filename = prop.getProperty("sdbquery100filename");
        String sdbquery1filename = prop.getProperty("sdbquery1filename");

        String jmeter2 = prop.getProperty("jmeter2");
        String records = prop.getProperty("records");
        String recordnum = prop.getProperty("recordnum");
        String samerecordfilename = prop.getProperty("samerecordfilename");

        this.total = Integer.parseInt(total);
        this.percent = "".equals(percent) ? 0 : Double.parseDouble(percent);
        this.len = getLen(len);
        this.size = Integer.parseInt(size);
        this.arrayLen = getLen(arraylen);
        this.thNum = Integer.parseInt(thnum);
        this.hostName = "".equals(hostname) ? "localhost".split(",") : hostname.split(",");
        this.coord = "".equals(coord) ? "11810" : coord;
        this.csName = csname;
        this.clName = clname;
        this.fields = fields.split(",");
        this.jmeter = "".equals(jmeter) ? false : Boolean.parseBoolean(jmeter);
        this.keywordNum = Integer.parseInt(keywordnum);
        this.keywords = Integer.parseInt(keywords);
        this.filePath = filepath;
        this.fulltextFileName = fulltextfilename;
        this.sdbQuery100FileName = sdbquery100filename;
        this.sdbQuery1FileName = sdbquery1filename;
        this.jmeter2 = "".equals(jmeter2) ? false : Boolean.parseBoolean(jmeter2);
        this.records = Integer.parseInt(records);
        this.recordNum = Integer.parseInt(recordnum);
        this.sameRecordFileName = samerecordfilename;
    }

    private int getLen(String len) {
        int len2 = 0;
        String t = len.substring(0, len.length() - 1);
        String t2 = len.substring(len.length() - 1, len.length());
        if ("K".equals(t2)) {
            len2 = Integer.parseInt(t) * 1024;
        } else if ("M".equals(t2)) {
            len2 = Integer.parseInt(t) * 1024 * 1024;
        } else {
            len2 = Integer.parseInt(len);
        }
        return len2;
    }
}