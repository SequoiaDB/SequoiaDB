package com.sequoiadb.chmProjectCreator;

public class ProjectTemplate {
    public static final int general_title_index = 2;
    public static final int general_rootdir_index = 3;
    public static final int general_default_topic_index = 4;
    public static final int general_compiled_file_index = 5;
    public static final int general_webhelp_output_folder_index = 19;
    public static final int general_singlehtml_output_folder_index = 30;
    public static final String []GENERAL = {
        "[GENERAL]",
        "Ver=2",
        "Title=", // fill title
        "RootDir=",  // fill  input dir
        "DefaultTopic=",
        "CompiledFile=", // fill E:\work\sequoiadb\doc_new\build\help.chm"
        "CustomTemplate=",
        "DefaultTemplate=1",
        "Language=0x0804",
        "Encoding=UTF-8",
        "DeleteProject=0",
        "ViewCompiledFile=0",
        "HasChild=0",
        "NoChild=10",
        "HtmlHelpTemplate=",
        "HtmlHelpTitle=",
        "HtmlHelpTitleSame=1",
        "HtmlHelpOutputEncoding=gb2312",
        "WebHelpDefault=",
        "WebHelpOutputFolder=", // outputdir <Project_Folder>_output\WebHelp\",
        "WebHelpTemplate=",
        "WebHelpTitle=",
        "WebHelpDefaultSame=1",
        "WebHelpTemplateSame=1",
        "WebHelpTilteSame=1",
        "WebHelpLanguage=1",
        "StartFromRoot=1",
        "AutoCollapse=0",
        "DrawLines=1",
        "SingleHtmlFilename=all.htm",
        "SingleHtmlOutputFolder=", // outputdir <Project_Folder>_output\SingleHTML\",
        "SingleHtmlTitle=",
        "SingleHtmlHasToc=0",
        "SingleHtmlSame=1",
        "HeadProperties=1",
        "PageProperties=1",
        "RealColorIcon=0",
        "ShowIndex=1",
        "NavWidth=270",
        "WebFontColor=#DBEFF9",
        "WebBackColor=",
        "WebBackground=1",
        "HHPFolder="
    };
    
    public static final int chmsetting_title_index = 48;
    public static final int chmsetting_topic_index = 51;
    public static final String []CHMSetting = {
        "[CHMSetting]",
        "Top=50",
        "Left=50",
        "Height=650",
        "Width=900",
        "PaneWidth=270",
        "DefaultTab=0",
        "ShowMSDNMenu=0",
        "ShowPanesToolbar=1",
        "ShowPane=1",
        "HideToolbar=0",
        "HideToolbarText=0",
        "StayOnTop=0",
        "Maximize=0",
        "Hide=1",
        "Locate=1",
        "Back=1",
        "bForward=1",
        "Stop=1",
        "Refresh=1",
        "Home=1",
        "Print=1",
        "Option=1",
        "Jump1=0",
        "Jump2=0",
        "AutoShowHide=0",
        "AutoSync=1",
        "Content=1",
        "Index=1",
        "Search=1",
        "Favorite=1",
        "UseFolder=0",
        "AutoTrack=0",
        "SelectRow=0",
        "PlusMinus=1",
        "ShowSelection=1",
        "ShowRoot=1",
        "DrawLines=1",
        "AutoExpand=0",
        "RightToLeft=0",
        "LeftScroll=0",
        "Border=0",
        "DialogFrame=0",
        "RaisedEdge=0",
        "SunkenEdge=0",
        "SavePosition=0",
        "ContentsFont=,8,134",
        "IndexFont=,8,134",
        "Title=", //fill title
        "Language=0x0804",
        "Font=",
        "DefaultTopic=", //fill release_note.html
    };
    
    public static final String topics = "[TOPICS]";
    public static final String titlelist = "TitleList";
    public static final String titlelist_title = "TitleList.Title";
    public static final String titlelist_level = "TitleList.Level";
    public static final String titlelist_url = "TitleList.Url";
    public static final String titlelist_icon = "TitleList.Icon";
    public static final String titlelist_status = "TitleList.Status";
    public static final String titlelist_keywords = "TitleList.Keywords";
    public static final String titlelist_contextnumber = "TitleList.ContextNumber";
    public static final String titlelist_applytemp = "TitleList.ApplyTemp";
    public static final String titlelist_expanded = "TitleList.Expanded";
    public static final String titlelist_kind = "TitleList.Kind";
//    public static final String []Topics = {
//        "TitleList.Title", //TODO: fill title(change to gbk)
//        "TitleList.Level", //TODO: level
//        "TitleList.Url", //TODO: fill url, 相对目录 release_note.html
//        "TitleList.Icon",
//        "TitleList.Status", //TODO: dir:2, html:0
//        "TitleList.Keywords",
//        "TitleList.ContextNumber", //TODO: increase from 1000
//        "TitleList.ApplyTemp",
//        "TitleList.Expanded",
//        "TitleList.Kind"
//    };
}

class title{
    private String title;       //
    private int level;          //   
    private String url;         //
    private int icon = 0;
    private int status;         //
    private String keywords = "";
    private int contextNumber;  //
    private int applyTemp = 0;
    private int expand = 0;
    private int kind = 0;
    
    public title(String title, int level, String url, int status, int contextNumer) {
        this.title = title;
        this.level = level;
        this.url = url;
        this.status = status;
        this.contextNumber=contextNumer;
    }
    
    public String getTitle() {
        return title;
    }

    public void setTitle(String title) {
        this.title = title;
    }

    public int getLevel() {
        return level;
    }

    public void setLevel(int level) {
        this.level = level;
    }

    public String getUrl() {
        return url;
    }

    public void setUrl(String url) {
        this.url = url;
    }

    public int getStatus() {
        return status;
    }

    public void setStatus(int status) {
        this.status = status;
    }

    public int getContextNumber() {
        return contextNumber;
    }

    public void setContextNumber(int contextNumber) {
        this.contextNumber = contextNumber;
    }

    public int getIcon() {
        return icon;
    }

    public String getKeywords() {
        return keywords;
    }

    public int getApplyTemp() {
        return applyTemp;
    }

    public int getExpand() {
        return expand;
    }

    public int getKind() {
        return kind;
    }
    
}

