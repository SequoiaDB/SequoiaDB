package com.sequoiadb.om.plugin;

import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;

import com.sequoiadb.om.plugin.dao.DaoFactory;
import com.sequoiadb.om.plugin.dao.SequoiaSQLOperations;
import com.sequoiadb.om.plugin.om.NodeAuth;
import com.sequoiadb.om.plugin.om.OMClient;
import com.sequoiadb.om.plugin.om.SequoiaSQLNode;
import org.json.JSONObject;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.web.bind.annotation.*;

import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;
import java.sql.SQLException;
import java.util.ArrayList;
import java.util.List;

@RestController
public class SQLController {

    private final Logger logger = LoggerFactory.getLogger(SQLController.class);

    @Autowired
    private OMClient omCtrl;

    @Autowired
    private DaoFactory factory;

    @RequestMapping(value = "/sql")
    public String sql(HttpServletRequest request, HttpServletResponse response) {
        String type = request.getParameter("Type");

        if (type == null) {
            type = "";
        }

        type = type.trim();

        return exec_sql(request, response, factory.getDao(type));
    }

    private String exec_sql(HttpServletRequest request, HttpServletResponse response, SequoiaSQLOperations ssqlo) {
        List<JSONObject> content = new ArrayList<JSONObject>();

        String ClusterName = request.getHeader("SdbClusterName");
        String BusinessName = request.getHeader("SdbBusinessName");
        String Sql = request.getParameter("Sql");
        String DbName = request.getParameter("DbName");
        String allData = request.getParameter("IsAll");
        boolean isAll = false;
        if (ClusterName == null || ClusterName.trim().length() == 0) {
            return outputResult(-1, "SdbClusterName is NULL", "", content);
        }
        if (BusinessName == null || BusinessName.trim().length() == 0) {
            return outputResult(-1, "SdbBusinessName is NULL", "", content);
        }
        if (Sql == null || Sql.trim().length() == 0) {
            return outputResult(-1, "Sql is NULL", "", content);
        }
        if (allData != null && allData.equals("true")) {
            isAll = true;
        }

        Sql = Sql.trim();
        SequoiaSQLNode node;

        try {
            node = omCtrl.getSsqlInfo(ClusterName, BusinessName);
        } catch (BaseException e) {
            return outputResult(e.getErrorCode(), "Failed to get " + BusinessName + " service info", e.getMessage(), content);
        }

        if (node.getHostName() == null || node.getHostName().length() == 0 ||
                node.getSvcName() == null || node.getSvcName().isEmpty()) {
            return outputResult(SDBError.SDB_SYS.getErrorCode(), "Failed to get " + BusinessName + " service info", SDBError.SDB_SYS.getErrorDescription(), content);
        }

        NodeAuth auth;

        try {
            auth = omCtrl.getSsqlAccountInfo(ClusterName, BusinessName, ssqlo.getDefaultUser());
        } catch (BaseException e) {
            return outputResult(e.getErrorCode(), "Failed to get " + BusinessName + " auth info", e.getMessage(), content);
        }

        if (DbName == null || DbName.trim().length() == 0) {
            if (auth.getDefaultDb() != null && auth.getDefaultDb().length() > 0) {
                DbName = auth.getDefaultDb();
            }
        }

        try {
            content = ssqlo.query(node.getHostName(), node.getSvcName(),
                    auth.getUser(), auth.getPasswd(), DbName, Sql, isAll);
        } catch (SQLException e) {
            int rc = e.getErrorCode();
            if (rc == 0) {
                rc = -1;
            }
            return outputResult(rc, e.getMessage() == null ? e.toString() : e.getMessage(),
                    "", content);
        } catch (Exception e) {
            /*
            StackTraceElement elements[] = e.getStackTrace();
            String traceStr = "";
            for (int i = 0; i < elements.length; i++) {
                StackTraceElement stackTraceElement = elements[i];
                String className = stackTraceElement.getClassName();
                String methodName = stackTraceElement.getMethodName();
                String fileName = stackTraceElement.getFileName();
                int lineNumber = stackTraceElement.getLineNumber();
                traceStr += "fileName=" + fileName + ",className=" + className +
                        ",methodName=" + methodName + ",lineNumber=" + lineNumber + "\n";
            }
            logger.error(traceStr);
            */
            return outputResult(-1, e.getMessage() == null ? e.toString() : e.getMessage(),
                    "", content);
        }

        return outputResult(0, "", "Succeed", content);
    }

    private String outputResult(int rc, String detail, String description, List<JSONObject> content) {

        JSONObject result = new JSONObject();
        result.put("errno", rc);
        result.put("detail", detail == null ? "" : detail);
        result.put("description", description == null ? "" : description);

        content.add(0, result);

        return content.toString();
    }
}
