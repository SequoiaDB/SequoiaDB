package com.sequoiadb.wordConvertor;

import org.apache.log4j.Logger;

import com.jacob.activeX.ActiveXComponent;
import com.jacob.com.ComThread;
import com.jacob.com.Dispatch;
import com.jacob.com.Variant;

public class WordConvertor {
    private static final int max_word_picture_width = 420;
    private static Logger logger = Logger.getLogger(WordConvertor.class);
    private Boolean isVisible = false;
    private int periodCountOfPics;
    private ActiveXComponent app = null;

    public WordConvertor(Boolean isVisible, int periodCountOfPics) {
        this.isVisible = isVisible;
        this.periodCountOfPics = periodCountOfPics;
    }

    public void quitApp() {
        if (null != app) {
            app.invoke("Quit", new Variant[] {});
            app = null;
        }
    }

    public void htmlToWord(String htmlFile, String outWordFile) throws Exception {
        quitApp();
        app = new ActiveXComponent("Word.Application");
        try {
            app.setProperty("Visible", new Variant(isVisible));

            Dispatch documents = app.getProperty("Documents").toDispatch();
            Dispatch doc = Dispatch.invoke(documents, "Open", Dispatch.Method,
                    new Object[] { htmlFile, new Variant(false), new Variant(true) }, new int[1])
                    .toDispatch();

            Dispatch.invoke(doc, "SaveAs", Dispatch.Method,
                    new Object[] { outWordFile, new Variant(1) }, new int[1]);

            Dispatch.call(doc, "Close", new Variant(0));
            //Dispatch.call(documents, "Close", new Variant(0));
        }
        catch (Exception e) {
            throw e;
        }
        finally {
            try {
                quitApp();
            }
            catch (Exception e) {
                throw e;
            }
            finally {
                ComThread.Release();
            }
        }
    }

    private void setHeaderFooter(ActiveXComponent app, Dispatch doc, String headerText) {
        //move selection to the begin of the document
        Dispatch selection = app.getProperty("Selection").toDispatch();
        Dispatch.call(selection, "HomeKey", new Variant(6));

        Dispatch activeWindow = app.getProperty("ActiveWindow").toDispatch();
        Dispatch activePane = Dispatch.get(activeWindow, "ActivePane").toDispatch();
        Dispatch view = Dispatch.get(activePane, "View").toDispatch();

        Dispatch.put(view, "SeekView", new Variant("9")); //header
        Dispatch.put(selection, "Text", new Variant(headerText));

        Dispatch.put(view, "SeekView", new Variant("10")); //footer

        final Dispatch sections = Dispatch.get(doc, "Sections").toDispatch();
        final Dispatch item = Dispatch.call(sections, "Item", new Variant(1)).toDispatch();
        final Dispatch footer = Dispatch.get(item, "Footers").toDispatch();
        final Dispatch f1 = Dispatch.call(footer, "Item", new Variant(1)).toDispatch();
        final Dispatch range = Dispatch.get(f1, "Range").toDispatch();
        final Dispatch fields = Dispatch.get(range, "Fields").toDispatch();

        Dispatch paragraphFormat = Dispatch.get(selection, "ParagraphFormat").getDispatch();
        Dispatch.put(paragraphFormat, "Alignment", 1);
        //the page number
        Dispatch.call(fields, "Add", Dispatch.get(selection, "Range").toDispatch(), new Variant(-1),
                "Page", true).toDispatch();
        Dispatch.call(selection, "TypeText", "/");
        //total page numbers
        Dispatch.call(fields, "Add", Dispatch.get(selection, "Range").toDispatch(), new Variant(-1),
                "NumPages", true).toDispatch();
    }

    private int getScale(int width, int height) {
        int scale = 100;
        if (width > max_word_picture_width) {
            scale = (max_word_picture_width * 100) / width;
        }

        return scale;
    }

    private void adjustPicture(Dispatch doc, PictureAdjustContext context) {
        Dispatch shapes = Dispatch.get(doc, "InLineShapes").toDispatch();
        int maxCount = Dispatch.get(shapes, "Count").toInt();
        context.setMaxCount(maxCount);
        logger.debug("pictures count=" + maxCount);

        int idx = context.getIndex();
        for (int pictureCount = 0; idx <= context.getMaxCount()
                && pictureCount < context.getPeriodCount(); idx++, pictureCount++) {
            Dispatch shape = Dispatch.call(shapes, "Item", new Variant(idx)).toDispatch();
            Dispatch.call(shape, "Select");
            int width = Dispatch.get(shape, "Width").toInt();
            int height = Dispatch.get(shape, "Height").toInt();
            int type = Dispatch.get(shape, "Type").toInt();

            if (type == 4 || type == 3) {
                // 4 Linked picture. 
                // 3 Picture. 
                int scale = getScale(width, height);
                logger.debug("picture[" + idx + "]:width=" + width + ",height=" + height + ",scale="
                        + scale + ",type=" + type);
                if (100 != scale) {
                    Dispatch.put(shape, "ScaleWidth", new Variant(scale));
                    Dispatch.put(shape, "ScaleHeight", new Variant(scale));
                }

                if (type == 4) {
                    Dispatch linkFormat = Dispatch.get(shape, "LinkFormat").toDispatch();
                    Dispatch.put(linkFormat, "SavePictureWithDocument", new Variant(true));
                    Dispatch.call(linkFormat, "BreakLink");
                    linkFormat.safeRelease();
                }
            }

            shape.safeRelease();
        }

        context.setIndex(idx);
    }

    private void adjustTables(Dispatch doc) {
        Dispatch tables = Dispatch.get(doc, "Tables").toDispatch();
        int count = Dispatch.get(tables, "Count").toInt();
        logger.debug("tables count=" + count);

        for (int i = 1; i <= count; i++) {
            try {
                Dispatch table = Dispatch.call(tables, "Item", new Variant(i)).toDispatch();
                Dispatch.call(table, "Select");
                Dispatch.put(table, "ApplyStyleFirstColumn", new Variant(true));
                Dispatch borders = Dispatch.get(table, "Borders").toDispatch();
                Dispatch.put(borders, "InsideLineStyle", new Variant(1));
                logger.debug("adjust table[" + i + "]");
            }
            catch (Exception e) {
                logger.error("adjust table failed:i=" + i, e);
            }
        }
    }

    private void addTablesOfContent(ActiveXComponent app) {
        //选定插入点
        Dispatch selection = app.getProperty("Selection").toDispatch();
        Dispatch.call(selection, "HomeKey", new Variant(6));
        Dispatch range = Dispatch.get(selection, "Range").toDispatch();

        Dispatch activeDocument = app.getProperty("ActiveDocument").toDispatch();

        Dispatch tableOfContent = Dispatch.get(activeDocument, "TablesOfContents").toDispatch();

        Dispatch.put(tableOfContent, "Format", new Variant(0));
        Dispatch.call(tableOfContent, "Add", range, new Variant(true), new Variant(1),
                new Variant(3));
    }

    //    public void printTabContent(String wordFile) throws Exception {
    //        ActiveXComponent app = new ActiveXComponent("Word.Application");
    //        Dispatch doc = null;
    //        try {
    //            //app.setProperty("Visible", new Variant(isVisible));
    //            Dispatch documents = app.getProperty("Documents").toDispatch();
    //
    //            //open the document
    //            doc = Dispatch.call(documents, "Open", wordFile).toDispatch();
    //            Dispatch tables = Dispatch.get(doc, "Tables").toDispatch();
    //            int count = Dispatch.get(tables, "Count").toInt();
    //            logger.debug("tables count=" + count);
    //            for (int k = 458; k < 460; k++) {
    //                Dispatch table = Dispatch.call(tables, "Item", new Variant(k)).toDispatch();
    //                int rows = Dispatch.get(Dispatch.call(table, "Rows").toDispatch(), "Count").toInt();
    //                int columns = Dispatch.get(Dispatch.call(table, "Columns").toDispatch(), "Count")
    //                        .toInt();
    //                logger.info("table:" + k);
    //                for (int j = 1; j < columns; j++) {
    //                    for (int i = 1; i < rows; i++) {
    //
    //                        Dispatch cell = Dispatch.call(table, "Cell", new Variant(i), new Variant(j))
    //                                .toDispatch();
    //                        Dispatch range = Dispatch.get(cell, "Range").toDispatch();
    //                        String t = Dispatch.get(range, "Text").toString();
    //                        System.out.print(new String(t.getBytes("utf-8")) + "\t");
    //                    }
    //                    System.out.println();
    //                }
    //            }
    //        }
    //        catch (Exception e) {
    //            e.printStackTrace();
    //            throw e;
    //        }
    //        finally {
    //            try {
    //                if (null != doc) {
    //                    Dispatch.call(doc, "Save");
    //                    Dispatch.call(doc, "Close", new Variant(0));
    //                }
    //
    //                app.invoke("Quit", new Variant[] {});
    //            }
    //            catch (Exception e) {
    //                e.printStackTrace();
    //                throw e;
    //            }
    //            finally {
    //                ComThread.Release();
    //            }
    //        }
    //    }

    private static void addShutdownHook(WordConvertor wc) {
        Runtime.getRuntime().addShutdownHook(new ShutdownHook(wc));
    }

    public static void main(String args[]) throws Exception {
        ConvertHelper helper = new ConvertHelper();
        int result = helper.parseArgs(args);
        if (0 != result) {
            throw new Exception("param error");
        }

        String input = helper.getInput();
        String output = helper.getOutput();
        Boolean hasTableOfContent = helper.getHasTableOfContent();

        WordConvertor wc = new WordConvertor(helper.isVisible(), helper.getPeriodCountOfPic());
        addShutdownHook(wc);

        logger.info("start to convert to doc");
        wc.htmlToWord(input, output);
        logger.info("start to adjust");
        wc.adjust(output, hasTableOfContent);
        logger.info("start to adjust pictures");
        wc.adjustAllPictures(output);
        logger.info("finish");

    }

    private Dispatch saveAndReopen(Dispatch documents, Dispatch doc, String wordFile) {
        Dispatch.call(doc, "Save");
        Dispatch.call(doc, "Close", new Variant(0));

        doc = Dispatch.call(documents, "Open", wordFile).toDispatch();
        return doc;
    }

    public void adjustAllPictures(String wordFile) throws Exception {
        PictureAdjustContext context = new PictureAdjustContext(1, periodCountOfPics);
        adjustPictures(wordFile, context);
        while (context.getIndex() < context.getMaxCount()) {
            adjustPictures(wordFile, context);
        }
    }

    private void adjustPictures(String wordFile, PictureAdjustContext context) throws Exception {
        quitApp();
        app = new ActiveXComponent("Word.Application");
        Dispatch doc = null;
        try {
            app.setProperty("Visible", new Variant(isVisible));
            Dispatch documents = app.getProperty("Documents").toDispatch();

            //open the document
            doc = Dispatch.call(documents, "Open", wordFile).toDispatch();

            //in default pictures are too big to display in the word.
            //adjust picture to a appropriate size
            logger.info("start to adjust pictures from index:" + context.getIndex());
            adjustPicture(doc, context);
        }
        catch (Exception e) {
            e.printStackTrace();
            throw e;
        }
        finally {
            try {
                if (null != doc) {
                    Dispatch.call(doc, "Save");
                    Dispatch.call(doc, "Close", new Variant(0));
                }
            }
            catch (Exception e) {
                e.printStackTrace();
                throw e;
            }
            finally {
                quitApp();
                ComThread.Release();
            }
        }
    }

    public void adjust(String wordFile, Boolean hasTableOfContent) throws Exception {
        quitApp();
        app = new ActiveXComponent("Word.Application");
        Dispatch doc = null;
        try {
            app.setProperty("Visible", new Variant(isVisible));
            Dispatch documents = app.getProperty("Documents").toDispatch();

            //open the document
            doc = Dispatch.call(documents, "Open", wordFile).toDispatch();

            if (hasTableOfContent) {
                logger.info("start to add table of content");
                addTablesOfContent(app);
            }

            logger.info("start to add header and footer");
            setHeaderFooter(app, doc, "SequoiaDB用户使用手册");
            //save and reopen the doc. otherwise, setHeaderFooter will take no effect.
            doc = saveAndReopen(documents, doc, wordFile);
        }
        catch (Exception e) {
            e.printStackTrace();
            throw e;
        }
        finally {
            try {
                if (null != doc) {
                    Dispatch.call(doc, "Save");
                    Dispatch.call(doc, "Close", new Variant(0));
                }
            }
            catch (Exception e) {
                e.printStackTrace();
                throw e;
            }
            finally {
                quitApp();
                ComThread.Release();
            }
        }
    }
}

class ShutdownHook extends Thread {
    private WordConvertor wc;

    public ShutdownHook(WordConvertor wc) {
        this.wc = wc;
    }

    @Override
    public void run() {
        if (null != wc) {
            wc.quitApp();
        }
    }
}

class PictureAdjustContext {
    private int maxCount;
    private int index;
    private int periodCount;

    public PictureAdjustContext(int startIndex, int periodCount) {
        this.index = startIndex;
        this.periodCount = periodCount;
    }

    public int getIndex() {
        return index;
    }

    public void setIndex(int index) {
        this.index = index;
    }

    public int getMaxCount() {
        return maxCount;
    }

    public void setMaxCount(int maxCount) {
        this.maxCount = maxCount;
    }

    public int getPeriodCount() {
        return periodCount;
    }

    public void setPeriodCount(int periodCount) {
        this.periodCount = periodCount;
    }
}