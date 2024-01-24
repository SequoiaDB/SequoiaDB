package com.sequoiadb.builddata;

import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

public class PollTh implements Runnable {

    private Keyword[] keywords;
    private int oncePushKeyword;
    private int pushNum;
    private int total;

    private int delNum;

    public static volatile boolean insertOk = false;

    public PollTh(int keywords, int keywordNum, String path, int total) throws IOException {
        this.total = total;
        init(keywords, keywordNum, path, total);
    }

    public void init(int keywordsSize, int keywordNum, String path, int total) throws IOException {
        keywords = new Keyword[keywordsSize];
        for (int i = 0; i < keywordsSize; i++) {
            String value = RandomUtils.UTILS.getNonSpaceStr(10);
            keywords[i] = new Keyword(value, keywordNum);
            BuildUtils.write2File(path, value + "\n");
        }
    }

    public void query4Num(String path, int num) throws BuildException, IOException {
        int size = total / (keywords.length * num);
        if (size == 0) {
            throw new BuildException("Insert total data at least: " + keywords.length * num);
        }
        if (size >= 3) {
            size = 3;
        }
        List<String> l = new ArrayList<>();
        for (int i = 0; i < size; i++) {
            String t = RandomUtils.UTILS.getNonSpaceStr(30);
            l.add(t);
            BuildUtils.write2File(path, t + "\n");
        }
        if (l.size() * num * keywords.length > total) {
            throw new BuildException("total data cannot less than keywords * query4Sdb keyword's size * query num");
        }

        query4Num(l, num);
    }

    private void query4Num(List<String> l, int num) throws BuildException {
        int count = 0;
        for (int i = 0; i < l.size(); i++) {
            for (int j = 0; j < num; j++) {
                for (int j2 = 0; j2 < keywords.length; j2++) {
                    if (keywords[j2].getAndDecrement() > 0) {
                        String containStr = keywords[j2].getKeyword();
                        containStr = containStr + " " + l.get(i) + " flag";
                        BuildMQ.MSGQUEUE.push(containStr.trim());
                        count++;
                    } else {
                        throw new BuildException("Keyword's num cannot less than query 4 sdb total num");
                    }
                }
            }
        }

        delNum = delNum + count;
    }

    @Override
    public void run() {
        int total = 0;
        for (int i = 0; i < keywords.length; i++) {
            total += keywords[i].get();
        }
        this.total = this.total - delNum;
        oncePushKeyword = total / this.total;
        pushNum = total % this.total;
        if (oncePushKeyword < 0) {
            try {
                throw new BuildException("total records: " + this.total + " del records: " + delNum
                        + " oncePushKeyword: " + oncePushKeyword);
            } catch (BuildException e) {
                e.printStackTrace();
            }
        }

        System.out.println("this.total: " + this.total + " delNum: " + delNum + " oncePushKeyword: " + oncePushKeyword
                + " pushNum:" + pushNum);
        int count = 0;
        int currentStart = 0;

        // total keywords % total num need to be positive
        int oncePush = oncePushKeyword + 1;
        String containStr = "";
        while (true) {
            boolean doFlag = false;
            for (int i = 0; i < keywords.length; i++) {
                if (count == oncePush + 1) {
                    count = 0;
                }
                if (i == currentStart) {
                    doFlag = true;
                }

                if (doFlag) {
                    if (count++ < oncePush) {
                        if (keywords[i].getAndDecrement() > 0) {
                            containStr = containStr + keywords[i].getKeyword() + " ";
                            if (keywords[i].get() == 0) {
                                keywords = remove(keywords, i);
                                i--;
                            }
                        } else {
                            keywords = remove(keywords, i);
                            i--;
                            count--;
                            continue;
                        }
                    } else {
                        currentStart = i;
                        break;
                    }
                }

                if (keywords.length - 1 == i) {
                    currentStart = 0;
                }
            }

            if (count == oncePush + 1) {
                if (!"".equals(containStr)) {
                    System.out.println(BuildMQ.MSGQUEUE.getLenth() + ":" + containStr.length());
                    BuildMQ.MSGQUEUE.push(containStr.trim());
                    containStr = "";

                    if (--pushNum == 0) {
                        oncePush = oncePushKeyword;
                        count = count - 1;
                    }
                }
            }

            if (keywords.length <= 0) {
                if (!"".equals(containStr)) {
                    BuildMQ.MSGQUEUE.push(containStr.trim());
                    containStr = "";
                }
                break;
            }
        }

        insertOk = true;
    }

    private Keyword[] remove(Keyword[] keywords, int i) {
        int count = 0;
        Keyword[] keywords2 = new Keyword[keywords.length - 1];
        for (int j = 0; j < keywords.length; j++) {
            if (j != i) {
                keywords2[count] = keywords[j];
                count++;
            }
        }
        return keywords2;
    }
}
