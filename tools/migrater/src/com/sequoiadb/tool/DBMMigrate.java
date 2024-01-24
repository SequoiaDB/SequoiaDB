package com.sequoiadb.tool;

import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.IOException;
import java.io.PrintWriter;
import java.io.StringWriter;
import java.util.ArrayList;
import java.util.List;

import org.jdom2.Document;
import org.jdom2.Element;
import org.jdom2.JDOMException;
import org.jdom2.input.SAXBuilder;
import org.jdom2.output.Format;
import org.jdom2.output.XMLOutputter;

import org.apache.commons.cli.CommandLine;
import org.apache.commons.cli.CommandLineParser;
import org.apache.commons.cli.Options;
import org.apache.commons.cli.HelpFormatter;
import org.apache.commons.cli.PosixParser;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BasicBSONList;
import org.bson.util.JSON;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;

public class DBMMigrate {

	// Buffer max size for import data
	private static int BUFFER_MAX_SIZE = 1024;
	private boolean verbose = false;
	private String host;
	private int port;
	private String user;
	private String password;
	private String path;
	private String collectionSpace;
	private boolean clean = false;

	public DBMMigrate(CommandLine commandLine) {
		if (commandLine.hasOption('v')) {
			verbose = true;
		}
		if (commandLine.hasOption('h')) {
			host = commandLine.getOptionValue('h');
		}
		if (commandLine.hasOption('s')) {
			String portStr = commandLine.getOptionValue('s');
			port = Integer.valueOf(portStr);
		}
		if (commandLine.hasOption('u')) {
			user = commandLine.getOptionValue('u');
		}
		if (commandLine.hasOption('w')) {
			password = commandLine.getOptionValue('w');
		}

		if (commandLine.hasOption('b')) {
			collectionSpace = commandLine.getOptionValue('b');
		}

		if (commandLine.hasOption('d')) {
			path = commandLine.getOptionValue('d');
		}

		if (commandLine.hasOption('c')) {
			clean = true;
		}

	}

	// export collection data to file (json format)
	private void exportCollectionData(Sequoiadb sdb, DBCollection cl,
			String fileName) {

		try {
			File jsonFile = new File(fileName);
			jsonFile.createNewFile();
			BufferedWriter jsonFileWriter = new BufferedWriter(new FileWriter(
					jsonFile));

			DBCursor cursor = cl.query();

			boolean bFirst = true;
			while (cursor.hasNext()) {
				if (!bFirst) {
					jsonFileWriter.newLine();
				} else {
					bFirst = false;
				}

				BSONObject record = cursor.getNext();
				jsonFileWriter.write(record.toString());
			}

			jsonFileWriter.flush();
			jsonFileWriter.close();

		} catch (Exception e) {
			e.printStackTrace();
		}
	}

	// export collection data to file (json format)
	private void importCollectionData(DBCollection cl, String fileName)
			throws Exception {

		try {
			File jsonFile = new File(fileName);
			@SuppressWarnings("resource")
			BufferedReader reader = new BufferedReader(new FileReader(jsonFile));

			String line;
			ArrayList<BSONObject> recordBuffer = new ArrayList<BSONObject>(
					BUFFER_MAX_SIZE);
			int i = 0;
			while ((line = reader.readLine()) != null) {
				BSONObject record = null;
				try {
					record = (BSONObject) JSON.parse(line);
				} catch (Exception e) {
					// parser record occur exception, skip
					continue;
				}
				// Add record to buffer
				recordBuffer.add(record);
				++i;

				// buffer is full, bulk insert to sequoiadb.
				if (i >= BUFFER_MAX_SIZE) {
					cl.bulkInsert(recordBuffer,
							DBCollection.FLG_INSERT_CONTONDUP);
					i = 0;
					recordBuffer.clear();
				}
			}

			// insert the last buffer
			if (i > 0) {
				cl.bulkInsert(recordBuffer, DBCollection.FLG_INSERT_CONTONDUP);
			}

		} catch (Exception e) {
			throw new Exception("Failed to export collection:"
					+ cl.getFullName() + " to " + fileName, e);
		}
	}

	private void exportCollection(Sequoiadb sdb, BSONObject clObj,
			String csName, Element csEle) throws Exception {
		String clFullName = (String) clObj.get("Name");
		String clName = clFullName.split("\\.")[1];

		Integer replSize = (Integer) clObj.get("ReplSize");

		Element clEle = new Element("collection");
		clEle.setAttribute("name", clName);
		clEle.setAttribute("ReplSize", replSize.toString());

		BSONObject shardingKey = (BSONObject) clObj.get("ShardingKey");
		if (shardingKey != null) {

			// ShardingKey xml format
			/*
			 * <ShardingKey> <field sort="1"> </Shardingkey>
			 */
			Element shardingKeyEle = new Element("ShardingKey");
			for (String key : shardingKey.keySet()) {
				Element keyEle = new Element(key);
				Integer sort = (Integer) shardingKey.get(key);

				// Add field to shardkey element
				keyEle.setAttribute("sort", sort.toString());
				shardingKeyEle.addContent(keyEle);
			}
			clEle.addContent(shardingKeyEle);

			// Add EnsureShardingIndex
			Boolean ensureIndex = (Boolean) clObj.get("EnsureShardingIndex");
			clEle.addContent(new Element("EnsureShardingIndex").setAttribute(
					"value", ensureIndex.toString()));

			// Add partition
			Integer Partition = (Integer) clObj.get("Partition");
			if (Partition == null) {
				Partition = 4096;
			}
			clEle.addContent(new Element("Partition").setAttribute("value",
					Partition.toString()));

		}

		// Add compress flag
		Boolean compressed = (Boolean) clObj.get("Compressed");
		if (compressed != null) {
			clEle.addContent(new Element("Compressed").setAttribute("value",
					compressed.toString()));
		}

		// Add index information
		Element indexesEle = new Element("indexes");
		CollectionSpace space = sdb.getCollectionSpace(csName);
		DBCollection cl = space.getCollection(clName);
		DBCursor cursor = cl.getIndexes();
		while (cursor.hasNext()) {

			BSONObject indexObj = cursor.getNext();
			BSONObject indexDefObj = (BSONObject) indexObj.get("IndexDef");
			String indexName = (String) indexDefObj.get("name");

			if (indexName == null) {
				throw new Exception("The collections:" + clFullName
						+ "'s have a index is not name.");
			}
			// skip system index, for example '$id'
			if (indexName.startsWith("$")) {
				continue;
			}

			Element indexEle = new Element("index");
			indexEle.setAttribute("name", indexName);

			Boolean unique = (Boolean) indexDefObj.get("unique");
			Boolean enforced = (Boolean) indexDefObj.get("enforced");

			if (unique != null) {
				indexEle.setAttribute("isUnique", unique.toString());
			}

			if (enforced != null) {
				indexEle.setAttribute("enforced", enforced.toString());
			}

			BSONObject keyObj = (BSONObject) indexDefObj.get("key");
			for (String key : keyObj.keySet()) {
				Integer sort = (Integer) keyObj.get(key);

				indexEle.addContent(new Element("key")
						.setAttribute("name", key).setAttribute("value",
								sort.toString()));
			}
			indexesEle.addContent(indexEle);

		}
		cursor.close();

		if (indexesEle.getContentSize() > 0) {
			clEle.addContent(indexesEle);
		}

		String clJsonFile = this.path + "/" + clFullName + ".json";
		clEle.addContent(new Element("file").setAttribute("value", clJsonFile));

		// export collection data to file
		printMessage(0, "Start export collection:%s data to %s ", clFullName,
				clJsonFile);
		exportCollectionData(sdb, cl, clJsonFile);
		printMessage(0, "success to export collection:%s data to %s.",
				clFullName, clJsonFile);

		csEle.addContent(clEle);
	}

	private void exportCollectionSpace(Sequoiadb sdb, BSONObject csObj,
			Element root) throws Exception {
		String csName = (String) csObj.get("Name");
		Integer pageSize = (Integer) csObj.get("PageSize");

		// create collectionspace xml node
		Element csEle = new Element("collectionspace");
		csEle.setAttribute("name", csName);
		csEle.setAttribute("PageSize", pageSize.toString());

		DBCursor clCursor = null;

		try {
			// Loop all collections of the collectionspace
			clCursor = sdb.getSnapshot(Sequoiadb.SDB_SNAP_CATALOG,
					"{Name:{$regex:'^" + csName + ".*',$options:''}}", null,
					null);
			while (clCursor.hasNext()) {
				BSONObject clObj = clCursor.getNext();
				String clName = (String) clObj.get("Name");

				if (!clName.startsWith(csName + ".")) // the collection not in
														// the collectionspace
					continue;

				printMessage(0, "Start export collection: %s ...", clName);

				exportCollection(sdb, clObj, csName, csEle);

				printMessage(0, "success to export collection:%s's.", clName);
			}

		} catch (BaseException e) {
			printMessage(1, "Failed to export collection:%s.", csName);
			System.out.println(e.getMessage());
			throw new Exception(
					"Failed to read data from sequoiadb. exception:%s", e);
		} finally {
			clCursor.close();
		}

		root.addContent(csEle);
	}

	public void executeExport() {

		Sequoiadb sdb = null;

		try {
			printMessage(1,
					"Start to excute export sequoiadb(%s:%d) to files(%s)",
					this.host, this.port, this.path);

			sdb = new Sequoiadb(host, port, user, password);
			printMessage(0, "Success to connect sequoiadb url:%s:%d", host,
					port);

			// create xml document, record collection meta information
			/*
			 * <sdbmigrate> <collectionspace name="default" PageSize="4096">
			 * <collection name="defualt.bar" ReplSize="1"> <ShardingKey /> <id
			 * sort="1"/> <name sort="-1"/> </ShardingKey> <EnsureShardingIndex
			 * value="1" /> <Partition value="4096"/> <file
			 * value="default.bar.json/> </collection> <collection
			 * name="defualt.bar2" ReplSize="1"> <file value="default.bar2.json"
			 * /> </collection> </collectionspace> </sdbmigrate>
			 */
			Document doc = new Document();
			Element root = new Element("sdbmigrate");
			doc.setRootElement(root);

			// loop all collectionspace
			DBCursor csCursor = sdb.getSnapshot(
					Sequoiadb.SDB_SNAP_COLLECTIONSPACES, "", "", "");

			while (csCursor.hasNext()) {
				BSONObject csObj = csCursor.getNext();

				String csName = (String) csObj.get("Name");

				// if have collectionspace params, then just export this
				// collectionspace
				if (collectionSpace != null && !collectionSpace.equals(csName)) {
					continue;
				}

				printMessage(0, "Start export collectionspace:%s ...", csName);
				// export this collectionspace
				exportCollectionSpace(sdb, csObj, root);
				printMessage(0, "compelete export collectionspace:%s", csName);
			}
			csCursor.close();

			Format format = Format.getCompactFormat();
			format.setEncoding("UTF-8");
			format.setIndent("   ");
			XMLOutputter outputter = new XMLOutputter(format);
			outputter.output(doc,
					new FileOutputStream(path + "/sdbmigrate.xml"));

			printMessage(1,
					"Success to excute export sequoiadb(%s:%d) to files(%s)",
					this.host, this.port, this.path);
		} catch (FileNotFoundException e) {
			printMessage(1, "The file %s is not be found int %s",
					"sdbmigrate.xml", path);
		} catch (IOException e) {
			printMessage(
					1,
					"Failed to write file %s, please check user authority and disk space. exception:",
					getExceptionTrace(e));
		} catch (Exception e) {
			printMessage(1, "Failed to export sequoiadb.exception:%s",
					getExceptionTrace(e));
		} finally {
			if (sdb != null) {
				sdb.disconnect();
			}
		}
	}

	private void splitToAddGroup(Sequoiadb sdb, DBCollection cl,
			int partitionNum) throws Exception {

		// Get the collection's current group by snapshot(8)
		String clFirstGroupName = null;
		DBCursor cursor = sdb.getSnapshot(Sequoiadb.SDB_SNAP_CATALOG, "{Name:'"
				+ cl.getFullName() + "'}", null, null);
		if (cursor.hasNext()) {
			BSONObject clCatInfo = cursor.getCurrent();
			BasicBSONList groupCataInfo = (BasicBSONList) clCatInfo
					.get("CataInfo");
			BSONObject groupInfo = (BSONObject) groupCataInfo.get(0);
			clFirstGroupName = (String) groupInfo.get("GroupName");
		} else {
			throw new Exception("The collection:" + cl.getFullName()
					+ " is not be created or the cata info is error.");
		}
		cursor.close();

		// Split the group to all groups
		List<String> groupNames = sdb.getReplicaGroupNames();
		// The group size must remove 'SYSCatalogGroup'
		int groupSize = groupNames.size() - 1;
		int splitNum = partitionNum / groupSize;
		int beginNum = 0;
		for (String groupName : groupNames) {
			// skip the source group which collection created first.
			if (groupName.equals(clFirstGroupName)
					|| groupName.equals("SYSCatalogGroup")) {
				continue;
			}
			BSONObject beginPartition = new BasicBSONObject();
			beginPartition.put("Partition", beginNum);
			BSONObject endPartition = new BasicBSONObject();
			endPartition.put("Partition", beginNum += splitNum);
			cl.split(clFirstGroupName, groupName, beginPartition, endPartition);
		}
	}

	private void importCollection(Sequoiadb sdb, CollectionSpace space,
			Element eleCL) throws Exception {
		// Read collection meta data, and create cl in sequoiadb
		String clName = eleCL.getAttributeValue("name");
		if (clName == null) {
			throw new Exception(
					"Failed to import collection:"
							+ clName
							+ ", Xml file error, the collection is short of 'name' attribute.");
		}

		// construct cl option object
		BSONObject clOptionObj = new BasicBSONObject();
		Integer replSize = Integer.valueOf(eleCL.getAttributeValue("ReplSize"));
		if (replSize != null) {
			clOptionObj.put("ReplSize", replSize);
		}

		Integer PartitionNum = 4096;
		Element eleShardingKey = eleCL.getChild("ShardingKey");
		if (eleShardingKey != null) {

			// Get shardingKey field and sort attribute, then push to option
			// bson object.
			BSONObject shardingKey = new BasicBSONObject();
			List<Element> shardingKeyList = eleShardingKey.getChildren();
			int i = 0;
			for (i = 0; i < shardingKeyList.size(); i++) {
				Element key = shardingKeyList.get(i);
				shardingKey.put(key.getName(),
						Integer.valueOf(key.getAttributeValue("sort")));
			}
			if (i == 0) {
				throw new Exception("Xml file error, the collection " + clName
						+ " is short of 'shardingKey' element.");
			}

			clOptionObj.put("ShardingKey", shardingKey);

			// Get&set EnsureShardingIndex attribute
			Element eleEnsureShardingIndex = eleCL
					.getChild("EnsureShardingIndex");
			if (eleEnsureShardingIndex != null) {
				Boolean ensureIndex = Boolean.valueOf(eleEnsureShardingIndex
						.getAttributeValue("value"));
				clOptionObj.put("EnsureShardingIndex", ensureIndex);
			}

			// Get&set Partition number
			Element elePartition = eleCL.getChild("Partition");
			if (elePartition != null) {
				PartitionNum = Integer.valueOf(elePartition
						.getAttributeValue("value"));
				clOptionObj.put("Partition", PartitionNum);
			}

			// Get&set ShardingType
			// TODO: this version just support hash split, and just support
			// split to all data group
			clOptionObj.put("ShardingType", "hash");

		}

		// Add compress flag
		Element compressedEle = eleCL.getChild("Compressed");
		if (compressedEle != null) {
			Boolean compressed = Boolean.valueOf(compressedEle
					.getAttributeValue("value"));
			clOptionObj.put("Compressed", compressed);
		}

		// Create collection with option
		DBCollection cl = space.createCollection(clName, clOptionObj);

		// Create indexes
		Element indexesEle = eleCL.getChild("indexes");
		if (indexesEle != null) {
			List<Element> indexList = indexesEle.getChildren();
			for (Element indexEle : indexList) {

				String indexName = indexEle.getAttributeValue("name");

				Boolean unique = false;
				String uniqueStr = indexEle.getAttributeValue("isUnique");
				if (uniqueStr != null) {
					unique = Boolean.valueOf(uniqueStr);
				}

				Boolean enforced = false;
				String enforcedStr = indexEle.getAttributeValue("enforced");
				if (enforcedStr != null) {
					enforced = Boolean.valueOf(enforcedStr);
				}

				List<Element> keyEles = indexEle.getChildren();
				BSONObject keysObj = new BasicBSONObject();
				for (Element keyEle : keyEles) {
					String name = keyEle.getAttributeValue("name");
					String sort = keyEle.getAttributeValue("value");

					if (name == null || sort == null) {
						throw new Exception(
								"Xml file error, the collection "
										+ clName
										+ "'s index is short of 'name' or 'value' attribute. "
										+ keyEle.toString());
					}

					keysObj.put(name, Integer.valueOf(sort));
					keysObj.put(name, Integer.valueOf(sort));
				}

				// Call sequoiadb api create index.
				cl.createIndex(indexName, keysObj, unique, enforced);
			}
		}

		// if have sharking key, then need split to all groups
		if (eleShardingKey != null) {
			splitToAddGroup(sdb, cl, PartitionNum);
		}

		// import data to sequoiadb
		Element fileEle = eleCL.getChild("file");
		if (fileEle != null) {
			String file = fileEle.getAttributeValue("value");

			if (file != null) {
				// Import collection's data to sequoiadb
				importCollectionData(cl, file);
			} else {
				throw new Exception("Xml file error, the collection " + clName
						+ " file elemente is short of 'value' attribute.");
			}
		} else {
			throw new Exception("Xml file error, the collection " + clName
					+ " is short of 'file' element.");
		}
	}

	private void importCollectionSpace(Sequoiadb sdb, Element eleCS)
			throws Exception {

		String csName = null;
		try {
			// Read collection space meta data, and create cs in sequoiadb
			csName = eleCS.getAttributeValue("name");
			Integer pageSize = Integer.valueOf(eleCS
					.getAttributeValue("PageSize"));
			CollectionSpace space = sdb.createCollectionSpace(csName, pageSize);

			// Loop the collections information in this collection space
			List<Element> childrenCL = eleCS.getChildren();
			for (int i = 0; i < childrenCL.size(); i++) {
				Element eleCL = childrenCL.get(i);

				printMessage(0, "Start to import collection:%s",
						eleCL.getAttributeValue("name"));
				importCollection(sdb, space, eleCL);
				printMessage(0, "Success to import collection:%s",
						eleCL.getAttributeValue("name"));
			}
		} catch (Exception e) {
			printMessage(1, "Failed to import collectionspace:%s", csName);
			throw e;
		}
	}

	private void cleanAllData(Sequoiadb sdb) {
		List<String> csNames = sdb.getCollectionSpaceNames();
		for (String csName : csNames) {
			sdb.dropCollectionSpace(csName);
		}
	}

	public void executeImport() {
		Sequoiadb sdb = null;
		try {
			printMessage(1,
					"Start to excute import files(%s) to sequoiadb(%s:%d)",
					this.path, this.host, this.port);

			sdb = new Sequoiadb(host, port, user, password);

			if (clean) {
				cleanAllData(sdb);
			}

			printMessage(0, "Success to connect sequoiadb url: %s:%d", host,
					port);

			// Open sdbmigrate.xml file, read cs/cl meta data information
			SAXBuilder builder = new SAXBuilder();
			Document doc = builder.build(new File(path + "/sdbmigrate.xml"));

			Element root = doc.getRootElement();
			List<Element> children = root.getChildren();

			// Loop the collection spaces information
			for (int i = 0; i < children.size(); i++) {
				Element eleCS = children.get(i);
				String csName = eleCS.getAttributeValue("name");

				// if have collectionspace params, then just import this
				// collectionspace
				if (collectionSpace != null && !collectionSpace.equals(csName)) {
					continue;
				}

				printMessage(0, "Start to import collectionspace:%s", csName);
				importCollectionSpace(sdb, eleCS);
				printMessage(0, "Success to import collectionspace:%s",
						eleCS.getAttributeValue("name"));
			}

			printMessage(1,
					"Success to excute import files(%s) to sequoiadb(%s:%d)",
					this.path, this.host, this.port);
		} catch (JDOMException e) {
			e.printStackTrace();
			printMessage(1, "Failed to read migrate xml file:%ssdbmigrate.xml",
					path);
		} catch (IOException e) {
			e.printStackTrace();
			printMessage(1, "Failed to read migrate xml file:%ssdbmigrate.xml",
					path);
		} catch (Exception e) {
			e.printStackTrace();
			printMessage(1, "Failed to import collectionspace to sequoiadb.");
		} finally {
			sdb.disconnect();
		}
	}

	private void printMessage(int level, String format, Object... args) {
		if (level == 1 || (this.verbose)) {
			String message = String.format(format, args);
			System.out.println(message);
		}
	}

	private String getExceptionTrace(Throwable e) {
		StringWriter sw = new StringWriter();
		PrintWriter ps = new PrintWriter(sw);
		e.printStackTrace(ps);
		return sw.toString();
	}

	private static void printUsage(Options options) {
		HelpFormatter formatter = new HelpFormatter();
		formatter
				.printHelp(
						"-a <export|import> -h <host> -s <port> [-u <user>] [-w <password>] -d <directory> [-b <collectionspace>] [-c] [-v]",
						options);
		// System.out.println("Command options:");
		// System.out.println("  --help                 help");
		// System.out.println("  -a [ --action ] arg    the action: export or import.");
		// System.out.println("  -h [ --hostname ] arg  sequoiadb host name");
		// System.out.println("  -s [ --svcname ] arg   sequoiadb coord name, default:11810");
		// System.out.println("  -u [ --user ] arg      sequoiadb user, default:''");
		// System.out.println("  -w [ --password ] arg  sequoiadb password, default:''");
		// System.out.println("  -d [ --directory] arg  the path of data files which use to import or export.");
		// System.out.println("  -c [ --clean]          whether cleaning up the original data before import");
		// System.out.println("  -v [ --verbose]        whether print verbose messsage");
	}

	/**
	 * @param args
	 * @throws Exception
	 */
	public static void main(String[] args) throws Exception {
		CommandLineParser parser = new PosixParser();
		Options options = new Options();
		options.addOption("help", false, "Print this usage information");
		options.addOption("a", "action", true, "select import or export action");
		options.addOption("h", "host", true, "The sequoiadb host or ip");
		options.addOption("s", "svcname", true,
				"The sequoiadb coord service port, default:11810");
		options.addOption("u", "user", true,
				"The sequoiadb login user name, default:''");
		options.addOption("w", "password", true,
				"The sequoiadb user's password, default:''");
		options.addOption("d", "directory", true,
				"The path of input/output for import/export");
		options.addOption("b", "collectionspace", true,
				"The collection space which be export or import.");
		options.addOption(
				"c",
				"clean",
				false,
				"The flag whether clean the sequoiadb data before import. \nJust takes effect in action is export.");
		options.addOption("v", "verbose", false,
				"Print out Verbose information");
		// options.addOption("i", "install dir", true,
		// "The path of sequoiadb, default:/opt/sequoiadb");

		CommandLine commandLine = parser.parse(options, args);

		if (!commandLine.hasOption('a')) {
			System.out.println("required parameter is missing in '"
					+ options.getOption("a").getLongOpt() + "'");
			printUsage(options);
			System.exit(0);
		} else if (!commandLine.hasOption('h')) {
			System.out.println("required parameter is missing in '"
					+ options.getOption("h").getLongOpt() + "'");
			printUsage(options);
			System.exit(0);
		} else if (!commandLine.hasOption('s')) {
			System.out.println("required parameter is missing in '"
					+ options.getOption("s").getLongOpt() + "'");
			printUsage(options);
			System.exit(0);
		} else if (!commandLine.hasOption('d')) {
			System.out.println("required parameter is missing in '"
					+ options.getOption("d").getLongOpt() + "'");
			printUsage(options);
			System.exit(0);
		} else if (commandLine.hasOption("help")) {
			printUsage(options);
			System.exit(0);
		}

		DBMMigrate migrate = new DBMMigrate(commandLine);

		String action = commandLine.getOptionValue('a');
		if (action.equals("export")) {
			migrate.executeExport();
		} else if (action.equals("import")) {
			migrate.executeImport();
		} else {
			System.out
					.println("The action argument only select export or import.");
			System.exit(1);
		}
	}
}
