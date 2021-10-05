'use strict'

let dbConf = {
	host     : 'dursley.socs.uoguelph.ca',
	user     : 'usernameGoesHere',
	password : 'passwordGoesHere',
	database : 'databaseNameGoesHere'
};

let insRec = "INSERT INTO student (last_name, first_name, mark) VALUES ('Hugo','Victor','B+'),('Rudin','Walter','A-'),('Stevens','Richard','C')";

async function main() {
    // get the client
    const mysql = require('mysql2/promise');
    
    let connection;

    try{
        // create the connection
        connection = await mysql.createConnection(dbConf)

         // **** uncomment if you want to create the table on the fly **** 
        // connection.execute("create table student (id int not null auto_increment,  last_name char(15),  first_name char(15), mark char(2), primary key(id) )");
    
        //Populate the table
        await connection.execute(insRec);

        //Run select query, wait for results
        const [rows1, fields1] = await connection.execute('SELECT * from `student` ORDER BY `last_name`');

        console.log("\nSorted by last name:");
        for (let row of rows1){
            console.log("ID: "+row.id+" Last name: "+row.last_name+" First name: "+row.first_name+" mark: "+row.mark );
        }

        //Run select query, wait for results
        console.log("\nSorted by first name:");
        const [rows2, fields2] = await connection.execute('SELECT * from `student` ORDER BY `first_name`');
        for (let row of rows2){
            console.log("ID: "+row.id+" Last name: "+row.last_name+" First name: "+row.first_name+" mark: "+row.mark );
        }

        await connection.execute("DELETE FROM student");
    }catch(e){
        console.log("Query error: "+e);
    }finally {
        if (connection && connection.end) connection.end();
    }
    
  }

main();