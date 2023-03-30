/* This SQL script is used to set up the database table needed by the LCM */
PRAGMA foreign_keys = ON;

CREATE TABLE IF NOT EXISTS devices 
(
   id INTEGER PRIMARY KEY CHECK (id>=0),
   uid INTEGER NOT NULL CHECK (uid>=0),
   name VARCHAR(100) NOT NULL DEFAULT 'LC7001',
   connected INTEGER NOT NULL
);

CREATE TABLE IF NOT EXISTS apps 
(
   id INTEGER PRIMARY KEY CHECK (id>=0),
   uid INTEGER NOT NULL CHECK (uid>=0),
   deviceId INTEGER,
   FOREIGN KEY(deviceId) REFERENCES devices(id)
);
