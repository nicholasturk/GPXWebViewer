"use strict";

// Express App (Routes)
const express = require("express");
const app = express();
const path = require("path");
const fileUpload = require("express-fileupload");
const bodyParser = require("body-parser");
const mysql = require("mysql2/promise");

app.use(bodyParser.json());
app.use(bodyParser.urlencoded({ extended: false }));
app.use(fileUpload());
app.use(express.static(path.join(__dirname + "/uploads")));

// Minimization
const fs = require("fs");
const JavaScriptObfuscator = require("javascript-obfuscator");

// Important, pass in port as in `npm run dev 1234`, do not change
const portNum = process.argv[2];
const schema = path.join(__dirname) + "/parser/validator/validator.xsd";

let connection;
let dbConf = { host: "dursley.socs.uoguelph.ca" };

// Send HTML at root, do not change
app.get("/", function(req, res) {
  res.sendFile(path.join(__dirname + "/public/index.html"));
});

// Send Style, do not change
app.get("/style.css", function(req, res) {
  //Feel free to change the contents of style.css to prettify your Web app
  res.sendFile(path.join(__dirname + "/public/style.css"));
});

// Send obfuscated JS, do not change
app.get("/index.js", function(req, res) {
  fs.readFile(path.join(__dirname + "/public/index.js"), "utf8", function(
    err,
    contents
  ) {
    const minimizedContents = JavaScriptObfuscator.obfuscate(contents, {
      compact: true,
      controlFlowFlattening: true
    });
    res.contentType("application/javascript");
    res.send(minimizedContents._obfuscatedCode);
  });
});

//Respond to POST requests that upload files to uploads/ directory
app.post("/upload", function(req, res) {
  if (!req.files) {
    return res.status(400).send("No files were uploaded.");
  }

  let uploadFile = req.files.uploadFile;
  if (fs.existsSync("./uploads/" + uploadFile.name)) {
    return res.status(400).send("File exists already.");
  }

  // Use the mv() method to place the file somewhere on your server
  uploadFile.mv("uploads/" + uploadFile.name, function(err) {
    if (err) {
      return res.status(500).send(err);
    }
    res.redirect("/");
  });
});

//Respond to GET requests for files in the uploads/ directory
app.get("/uploads/:name", function(req, res) {
  fs.stat("uploads/" + req.params.name, function(err, stat) {
    if (err == null) {
      res.sendFile(path.join(__dirname + "/uploads/" + req.params.name));
    } else {
      console.log("Error in file downloading route: " + err);
      res.send("");
    }
  });
});

//******************** Your code goes here ********************

let gpxParser = require("./libraryFunctions");

// File logs info endpoint
app.get("/fileLogs", (req, res) => {
  var fileInfoArr = [];

  fs.readdir(path.join(__dirname) + "/uploads", (err, files) => {
    if (!files) {
      return res.status(400).send("No files.");
    }
    files.forEach(file => {
      if (!file.endsWith(".gpx")) return;
      let doc = gpxParser.createValidGPXdoc(
        path.join(__dirname) + `/uploads/${file}`,
        schema
      );
      if (doc.address() === 0) return;
      let tmpObj = JSON.parse(gpxParser.GPXtoJSON(doc));
      tmpObj["fileName"] = file;
      fileInfoArr.push(tmpObj);
    });
    res.send(fileInfoArr);
  });
});

app.get("/fileNames", (req, res) => {
  var fileNames = [];

  fs.readdir(path.join(__dirname) + "/uploads", (err, files) => {
    files.forEach(file => {
      if (!file.endsWith(".gpx")) return;
      let doc = gpxParser.createValidGPXdoc(
        path.join(__dirname) + `/uploads/${file}`,
        schema
      );
      if (doc.address() === 0) return;
      fileNames.push(file);
    });
    res.send(fileNames);
  });
});

app.get("/fileInfo/:fileName", (req, res) => {
  let filePath = path.join(__dirname) + `/uploads/${req.params.fileName}`;
  if (!fs.existsSync(filePath)) {
    res.status(500).send("File does not exist.");
  } else {
    let components = {};
    let doc = gpxParser.createValidGPXdoc(filePath, schema);
    if (!doc) res.status(400).send("Invalid file.");
    components["Route"] = JSON.parse(
      gpxParser
        .routeListToJSON(gpxParser.getRouteList(doc))
        .replace(/(\r\n|\n|\r)/gm, "")
    );
    components["Track"] = JSON.parse(
      gpxParser
        .trackListToJSON(gpxParser.getTrackList(doc))
        .replace(/(\r\n|\n|\r)/gm, "")
    );
    res.send(components);
  }
});

app.post("/changeComponentName", async (req, res) => {
  let filePath = path.join(__dirname) + `/uploads/${req.body.fileName}`;
  if (!fs.existsSync(filePath)) {
    res.status(500).send("File does not exist.");
  } else {
    let doc = gpxParser.createValidGPXdoc(filePath, schema);
    if (req.body.componentType == "Track") {
      if (
        gpxParser.updateTrackName(
          doc,
          filePath,
          req.body.oldName,
          req.body.newName
        )
      ) {
        res.status(200).send("OK");
      }
    } else if (req.body.componentType == "Route") {
      if (
        gpxParser.updateRouteName(
          doc,
          filePath,
          req.body.oldName,
          req.body.newName
        )
      ) {
        try {
          connection = await mysql.createConnection(dbConf);
          let [f, _] = await connection.execute(
            `select * from FILE where file_name = '${req.body.fileName}'`
          );
          if (f.length > 0) {
            await connection.execute(
              `update ROUTE set route_name = '${req.body.newName}' where gpx_id = ${f[0].gpx_id} and route_name = '${req.body.oldName}'`
            );
          }
        } catch (e) {
          console.log(e);
        } finally {
          if (connection && connection.end) connection.end();
        }
        res.status(200).send("OK");
      }
    } else {
      res.status(500).send("Invalid component type.");
    }
  }
});

app.post("/addRoute", async (req, res) => {
  let isErr = 0;
  let filePath = path.join(__dirname) + `/uploads/${req.body.fileName}`;
  let doc = gpxParser.createValidGPXdoc(filePath, schema);
  if (!doc) {
    return res.status(400).send("Invalid file.");
  }
  let lats = Array.isArray(req.body["waypoints[][lat]"])
    ? req.body["waypoints[][lat]"].map(Number)
    : [req.body["waypoints[][lat]"]].map(Number);
  let lons = Array.isArray(req.body["waypoints[][lon]"])
    ? req.body["waypoints[][lon]"].map(Number)
    : [req.body["waypoints[][lon]"]].map(Number);

  if (lats.length == 0 || req.body.routeName.length == 0) {
    return res.status(400).send("Invalid parameters");
  }

  let success = gpxParser.addRouteWrapper(
    doc,
    filePath,
    req.body.routeName,
    lats,
    lons,
    lats.length
  );
  if (success == 1) {
    try {
      connection = await mysql.createConnection(dbConf);
      let [r, _] = await connection.execute(
        `select * from FILE where file_name = '${req.body.fileName}'`
      );
      if (r.length > 0) {
        let doc = gpxParser.createValidGPXdoc(filePath, schema);
        let ptrRt = gpxParser.getRoute(doc, req.body.routeName);
        let rt = JSON.parse(gpxParser.routeToJSON(ptrRt));
        let rt_ret = await connection.execute(
          `insert into ROUTE(route_name,route_len,gpx_id) values('${rt.name}',${rt.len},${r[0].gpx_id})`
        );
        let wps = JSON.parse(gpxParser.getRouteWpList(ptrRt));
        for (let wp of wps) {
          wp.name = wp.name === "None" ? null : `'${wp.name}'`;
          await connection.execute(
            `insert into POINT(point_name,latitude,longitude,route_id) values(${wp.name},${wp.latitude},${wp.longitude},${rt_ret[0].insertId})`
          );
        }
      }
    } catch (e) {
      res.status(500).send(e);
      res.end();
      isErr = 1;
    } finally {
      if (connection && connection.end) connection.end();
    }
    if (isErr === 0) {
      res.redirect("/");
    }
  } else {
    res.status(400).send("Failed.");
  }
});

app.post("/createGPX", (req, res) => {
  let filePath = path.join(__dirname) + `/uploads/${req.body.fileName}`;
  if (!filePath.endsWith(".gpx")) {
    filePath += ".gpx";
  }
  if (fs.existsSync(filePath)) {
    return res.status(400).send("File name already exists.");
  }
  let success = gpxParser.writeNewGPXDoc(
    filePath,
    req.body.creator,
    req.body.namespace,
    req.body.version
  );
  if (!success) {
    return res.status(500).send("Failed to write document.");
  }
  let doc = gpxParser.createValidGPXdoc(filePath, schema);
  if (doc.address() == 0) {
    fs.unlinkSync(filePath);
    return res.status(500).send("Invalid doc.");
  }
  return res.redirect("/");
});

app.post("/findPath", (req, res) => {
  let ret = [];
  fs.readdir(path.join(__dirname) + "/uploads", (err, files) => {
    if (err) {
      return res.status(500).send("Error in processing files.");
    }
    files.forEach(file => {
      if (!file.endsWith(".gpx")) return;
      let doc = gpxParser.createValidGPXdoc(
        path.join(__dirname) + `/uploads/${file}`,
        schema
      );
      if (doc.address() !== 0) {
        let components = {};
        let routeList = gpxParser.getRoutesBetween(
          doc,
          req.body.lat1,
          req.body.lon1,
          req.body.lat2,
          req.body.lon2,
          req.body.delta
        );
        let trackList = gpxParser.getTracksBetween(
          doc,
          req.body.lat1,
          req.body.lon1,
          req.body.lat2,
          req.body.lon2,
          req.body.delta
        );
        components["fileName"] = file;
        components["Route"] = JSON.parse(
          gpxParser.routeListToJSON(routeList).replace(/(\r\n|\n|\r)/gm, "")
        );
        components["Track"] = JSON.parse(
          gpxParser.trackListToJSON(trackList).replace(/(\r\n|\n|\r)/gm, "")
        );
        if (components["Route"].length > 0 || components["Track"].length > 0) {
          ret.push(components);
        }
      }
    });
    console.log(ret);
    res.send(ret);
  });
});

// dbConf["user"] = "nsturk";
// dbConf["password"] = "1058650";
// dbConf["database"] = "nsturk";

app.post("/login", async (req, res) => {
  try {
    dbConf["user"] = req.body.user;
    dbConf["password"] = req.body.pass;
    dbConf["database"] = req.body.db;
    console.log(dbConf);
    connection = await mysql.createConnection(dbConf);
    // await createTables();
    res.send({ user: req.body.user, db: req.body.db });
  } catch (e) {
    res.status(500).send(e);
  } finally {
    if (connection && connection.end) connection.end();
  }
});

app.get("/storeAllFiles", async (req, res) => {
  let fileInfoArr = [];
  let numFiles = 0,
    numPoints = 0,
    numRoutes = 0;

  fs.readdir(path.join(__dirname) + "/uploads", (err, files) => {
    if (!files) {
      return res.status(400).send("No files.");
    }
    files.forEach(file => {
      if (!file.endsWith(".gpx")) return;
      let doc = gpxParser.createValidGPXdoc(
        path.join(__dirname) + `/uploads/${file}`,
        schema
      );
      if (doc.address() === 0) return;
      let tmpObj = JSON.parse(gpxParser.GPXtoJSON(doc));
      tmpObj["fileName"] = file;
      fileInfoArr.push(tmpObj);
    });
  });

  try {
    connection = await mysql.createConnection(dbConf);
    let idx = 0;
    for (let obj of fileInfoArr) {
      let [rows, cols] = await connection.execute(
        `select * from FILE where file_name = '${obj.fileName}'`
      );
      if (rows.length == 0) {
        numFiles += 1;
        await connection.execute(
          `insert into FILE(file_name,ver,creator) values('${obj.fileName}','${obj.version}','${obj.creator}')`
        );
        let [rows, cols] = await connection.execute(
          `select * from FILE where file_name = '${obj.fileName}'`
        );

        let filePath = path.join(__dirname) + `/uploads/${obj.fileName}`;
        let doc = gpxParser.createValidGPXdoc(filePath, schema);
        let rts = JSON.parse(
          gpxParser
            .routeListToJSON(gpxParser.getRouteList(doc))
            .replace(/(\r\n|\n|\r)/gm, "")
        );
        for (let route of rts) {
          numRoutes += 1;
          let rtins = await connection.execute(
            `insert into ROUTE(route_name,route_len,gpx_id) values('${route.name}',${route.len},${rows[0].gpx_id})`
          );
          let rt = gpxParser.getRoute(doc, route.name);
          let wps = JSON.parse(gpxParser.getRouteWpList(rt));
          numPoints += wps.length;
          for (let wp of wps) {
            wp.name = wp.name === "None" ? null : `'${wp.name}'`;
            await connection.execute(
              `insert into POINT(point_name,latitude,longitude,route_id) values(${wp.name},${wp.latitude},${wp.longitude},${rtins[0].insertId})`
            );
          }
        }
      }
      idx++;
      if (idx == fileInfoArr.length) {
        res.send({ numFiles, numRoutes, numPoints });
      }
    }
  } catch (e) {
    res.status(500).send(e);
  } finally {
    if (connection && connection.end) connection.end();
  }
});

app.get("/clearAllData", async (req, res) => {
  try {
    connection = await mysql.createConnection(dbConf);
    await connection.execute("delete from POINT");
    await connection.execute("delete from ROUTE");
    await connection.execute("delete from FILE");
    res.send("OK");
  } catch (e) {
    res.send(e);
    console.log(e);
  } finally {
    if (connection && connection.end) connection.end();
  }
});

app.get("/displayDBStatus", async (req, res) => {
  try {
    connection = await mysql.createConnection(dbConf);
    let [p, _] = await connection.execute("select * from POINT");
    let [r, __] = await connection.execute("select * from ROUTE");
    let [f, ___] = await connection.execute("select * from FILE");

    res.send(
      `Database has ${f.length} files, ${r.length} routes, and ${p.length} points`
    );
  } catch (e) {
    res.send(e);
    console.log(e);
  } finally {
    if (connection && connection.end) connection.end();
  }
});

app.get("/getAllRoutes", async (req, res) => {
  try {
    if (!("user" in dbConf)) {
      res.send([]);
    } else {
      let connection = await mysql.createConnection(dbConf);
      let [r, _] = await connection.execute("select * from ROUTE");
      res.send(r);
    }
  } catch (e) {
    console.log(e);
    res.status(500).send(e);
  }
});

app.post("/queryTable", async (req, res) => {
  try {
    let connection = await mysql.createConnection(dbConf);
    let queryLookup = {};
    let sortBy = `${
      req.body.sortBy === "none" ? "" : "order by " + req.body.sortBy
    }`;

    // Query 1
    if (req.body.query === "1") {
      queryLookup["1"] = `select * from ROUTE ${sortBy}`;
    }

    // Query 2
    if (req.body.query === "2") {
      let [f, _] = await connection.execute(
        `select * from FILE where file_name = '${req.body.sec}'`
      );
      if (f.length > 0) {
        queryLookup[
          "2"
        ] = `select * from ROUTE where gpx_id = ${f[0].gpx_id} ${sortBy}`;
      }
    }

    //Query 3
    if (req.body.query === "3") {
      let [f, _] = await connection.execute(
        `select * from ROUTE where route_name = '${req.body.sec}'`
      );
      console.log(f);
      if (f.length > 0) {
        queryLookup[
          "3"
        ] = `select * from POINT where route_id = ${f[0].route_id} ${sortBy}`;
      }
    }

    //Query 4
    if (req.body.query === "4") {
      let q = "select * from POINT where route_id = ";
      let [f, _] = await connection.execute(
        `select * from FILE where file_name = '${req.body.sec}'`
      );
      let [r, x] = await connection.execute(
        `select * from ROUTE where gpx_id = ${f[0].gpx_id}`
      );
      let i = 0;
      for (let route of r) {
        console.log(q);
        if (i === 0) {
          q += `${route.route_id}`;
        } else {
          q += ` or route_id = ${route.route_id}`;
        }
        i++;
      }
      q += ` ${sortBy}`;
      queryLookup["4"] = q;
    }

    // Query 5
    if (req.body.query === "5") {
      let [f, _] = await connection.execute(
        `select * from FILE where file_name = '${req.body.sec}'`
      );
      let orderBy = req.body.larOrSmall == "Longest" ? "desc" : "asc";
      queryLookup[
        "5"
      ] = `select * from ROUTE where gpx_id = ${f[0].gpx_id} order by route_len ${orderBy} limit ${req.body.N}`;
    }

    console.log(queryLookup[req.body.query]);

    if (req.body.query in queryLookup) {
      let [rows, columns] = await connection.execute(
        queryLookup[req.body.query]
      );
      res.send({ rows, columns, secData: req.body.sec });
    } else {
      res.send({
        rows: [],
        columns: [
          {
            name:
              "No records found in db. Try loading all files again to update table"
          }
        ]
      });
    }
  } catch (e) {
    console.log(e);
    res.status(500).send(e);
  }
});

async function createTables() {
  try {
    connection = await mysql.createConnection(dbConf);
    // await connection.execute(`drop table if exists POINT`);
    // await connection.execute(`drop table if exists ROUTE`);
    // await connection.execute(`drop table if exists FILE`);
    await connection.execute(
      "create table if not exists FILE (gpx_id int auto_increment, file_name varchar(60) not null, ver decimal(2,1) not null, creator varchar(256) not null, primary key (gpx_id))"
    );
    await connection.execute(
      "create table if not exists ROUTE (route_id int auto_increment, route_name varchar(256), route_len float(15,7), gpx_id int not null, primary key (route_id), foreign key (gpx_id) references FILE(gpx_id) on delete cascade)"
    );
    await connection.execute(
      "create table if not exists POINT (point_id int auto_increment, point_index int not null, latitude decimal(11,7) not null, longitude decimal(11,7) not null, point_name varchar(256), route_id int not null, primary key (point_id), foreign key (route_id) references ROUTE(route_id) on delete cascade)"
    );
  } catch (e) {
    console.log(e);
  } finally {
    if (connection && connection.end) connection.end();
  }
}

app.listen(portNum);
console.log("Running app at localhost: " + portNum);
