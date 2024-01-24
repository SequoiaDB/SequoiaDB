package com.sequoiadb.builddata;

public class InsertTh implements Runnable {

    private Connect conn;
    private int[] need2Insert;

    private DataRecord strRecord;
    private DataRecord arrayRecord;
    private int strRecordSize;
    private int arrayRecordSize;

    public InsertTh(String hostName, int[] need2Insert) {
        conn = new Connect(hostName, Build.BUILD.coord, Build.BUILD.csName, Build.BUILD.clName);
        this.need2Insert = need2Insert;
    }

    @Override
    public void run() {
        generate();
        try {
            insertData(strRecord, strRecordSize);
            insertData(arrayRecord, arrayRecordSize);
        } catch (BuildException e) {
            e.printStackTrace();
        } finally {
            conn.close();
        }
    }

    private void generate() {
        strRecordSize = need2Insert[0];
        arrayRecordSize = need2Insert[1];

        strRecord = new DataRecord();
        arrayRecord = new DataRecord();
        strRecord.add(new DataField(Build.BUILD.fields[0], Build.BUILD.len));
        arrayRecord.add(new DataField(Build.BUILD.fields[0], Build.BUILD.arrayLen, Build.BUILD.size));

        for (int i = 1; i < Build.BUILD.fields.length; i++) {
            strRecord.add(new DataField(Build.BUILD.fields[i], Build.BUILD.len));
            arrayRecord.add(new DataField(Build.BUILD.fields[i], Build.BUILD.len));
        }
    }

    private void insertData(DataRecord record, int size) throws BuildException {
        if (size > 0) {
            if (Build.BUILD.jmeter) {
                conn.insert(record, size, true);
            } else if (Build.BUILD.jmeter2) {
                conn.insert(record, size, Build.BUILD.recordNum);
            } else {
                conn.insert(record, size);
            }
        }
    }
}
