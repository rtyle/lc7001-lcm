#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE SQLite3Test

#include <boost/test/unit_test.hpp>
#include <sqlite3.h>
#include "Debug.h"

using namespace boost::unit_test::framework;

void printTestCase(std::string testCase)
{
   logInfo("************************************************************");
   logInfo("Running Test Case %s", testCase.c_str());
   logInfo("************************************************************");
}

// Fixture to use for the test cases
struct DBFixture
{
   // -------------------------------------------------------------------------
   // -------------------------------------------------------------------------
   // -------------------------------------------------------------------------
   
   DBFixture()
   {
      // Clear the received count
      receivedCount = 0;

      // Open the database
      logDebug("Opening the database");
      int result = sqlite3_open_v2("test.db", &db, SQLITE_OPEN_READWRITE, NULL);
      BOOST_REQUIRE_EQUAL(result, SQLITE_OK);

      // clear the devices table
      logDebug("Clearing the devices table");
      result = sqlite3_exec(db, "delete from devices", NULL, NULL, NULL);
      BOOST_REQUIRE_EQUAL(result, SQLITE_OK);

      // clear the apps table
      logDebug("Clearing the apps table");
      result = sqlite3_exec(db, "delete from apps", NULL, NULL, NULL);
      BOOST_REQUIRE_EQUAL(result, SQLITE_OK);

      // Turn on foreign key support
      logDebug("Turning on foreign key support");
      result = sqlite3_exec(db, "PRAGMA foreign_keys = ON", NULL, NULL, NULL);
      BOOST_REQUIRE_EQUAL(result, SQLITE_OK);
   }

   // -------------------------------------------------------------------------
   // -------------------------------------------------------------------------
   // -------------------------------------------------------------------------
   
   ~DBFixture()
   {
      logDebug("Closing the database");
      sqlite3_close_v2(db);
      db = NULL;
   }

   // -------------------------------------------------------------------------
   // -------------------------------------------------------------------------
   // -------------------------------------------------------------------------
   
   int callback(int argc, char **value, char **column)
   {
      logDebug("In DBFixture callback");
      receivedCount++;

      for(int i = 0; i < argc; i++)
      {
         logDebug("%s, %s", column[i], value[i]);
      }
      return SQLITE_OK;
   }

   sqlite3 *db;
   int receivedCount;
};

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

// Callback function for sqlite exec calls
// This function will call the method in the DBFixture structure to handle
// the callback
static int c_callback(void *param, int argc, char **argv, char **azColName)
{
   DBFixture *dbFixture = static_cast<DBFixture *>(param);
   return dbFixture->callback(argc, argv, azColName);
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(Test_SQLite3OpenInvalidDatabase)
{
   printTestCase(current_test_case().p_name.value);

   // Try to open a database file that does not exist.
   sqlite3 *db;
   int result = sqlite3_open_v2("invalid.db", &db, SQLITE_OPEN_READWRITE, NULL);
   BOOST_REQUIRE_EQUAL(result, SQLITE_CANTOPEN);

   // Close the database handle
   result = sqlite3_close_v2(db);
   BOOST_REQUIRE_EQUAL(result, SQLITE_OK);

   logInfo("Finished with test");
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(Test_SQLite3OpenValidDatabase)
{
   printTestCase(current_test_case().p_name.value);

   // Open the database handle
   sqlite3 *db;
   int result = sqlite3_open_v2("test.db", &db, SQLITE_OPEN_READWRITE, NULL);
   BOOST_REQUIRE_EQUAL(result, SQLITE_OK);

   // Close the database handle
   result = sqlite3_close_v2(db);
   BOOST_REQUIRE_EQUAL(result, SQLITE_OK);

   logInfo("Finished with test");
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

BOOST_FIXTURE_TEST_CASE(Test_InsertToInvalidTable, DBFixture)
{
   printTestCase(current_test_case().p_name.value);

   char *errorString;
   int result;

   logInfo("Attempting to write a row to a table that doesn't exist");
   result = sqlite3_exec(db, "insert into invalidTable values(1,2,3)", NULL, NULL, &errorString);
   BOOST_REQUIRE_EQUAL(result, SQLITE_ERROR);
   logDebug("Error correctly returned (%s)", errorString);
   sqlite3_free(errorString);

   logInfo("Finished with test");
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

BOOST_FIXTURE_TEST_CASE(Test_InsertAndUpdateDevices, DBFixture)
{
   printTestCase(current_test_case().p_name.value);

   char *errorString;
   int result;

   logInfo("Attempting to write a row to devices with too few values");
   result = sqlite3_exec(db, "insert into devices values(1,2)", NULL, NULL, &errorString);
   BOOST_REQUIRE_EQUAL(result, SQLITE_ERROR);
   logDebug("Error correctly returned (%s)", errorString);
   sqlite3_free(errorString);

   logInfo("Attempting to write a row to devices with too many values");
   result = sqlite3_exec(db, "insert into devices values(1,2,\"Test\",3, 4)", NULL, NULL, &errorString);
   BOOST_REQUIRE_EQUAL(result, SQLITE_ERROR);
   logDebug("Error correctly returned (%s)", errorString);
   sqlite3_free(errorString);

   logInfo("Attempting to write a row to devices with invalid value type for the primary key");
   result = sqlite3_exec(db, "insert into devices values(\"Test\", 1, 2, 1)", NULL,NULL, &errorString);
   logDebug("Error correctly returned (%s)", errorString);
   BOOST_REQUIRE_EQUAL(result, SQLITE_MISMATCH);
   sqlite3_free(errorString);

   logInfo("Update a device that does not exist");
   result = sqlite3_exec(db, "update devices set id=1 where uid=100", NULL, NULL, NULL);
   BOOST_REQUIRE_EQUAL(result, SQLITE_OK);
   // Ensure that the update command did not actually change anything
   BOOST_REQUIRE_EQUAL(sqlite3_changes(db), 0);

   logInfo("Add a device to the table");
   result = sqlite3_exec(db, "insert into devices values(1, 2, 'Test', 1)", NULL, NULL, &errorString);
   BOOST_REQUIRE_EQUAL(result, SQLITE_OK);
   logDebug("Successfully added device");
   sqlite3_free(errorString);

   logInfo("Attempt to add another device with the same id (primary key)");
   result = sqlite3_exec(db, "insert into devices values(1, 3, 'Test2', 1)", NULL,NULL, &errorString);
   BOOST_REQUIRE_EQUAL(result, SQLITE_CONSTRAINT);
   logDebug("Error correctly returned (%s)", errorString);
   sqlite3_free(errorString);

   logInfo("Update the first device to have a different device id and then try to insert again");
   result = sqlite3_exec(db, "update devices set id=2 where id=1", NULL, NULL, &errorString);
   BOOST_REQUIRE_EQUAL(result, SQLITE_OK);
   result = sqlite3_exec(db, "insert into devices values(1, 3, 'Test2', 1)", NULL,NULL, &errorString);
   BOOST_REQUIRE_EQUAL(result, SQLITE_OK);
   logDebug("Successfully updated table");
   sqlite3_free(errorString);

   logInfo("Finished with test");
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

BOOST_FIXTURE_TEST_CASE(Test_ReadRowsFromDevices, DBFixture)
{
   printTestCase(current_test_case().p_name.value);

   char *errorString;
   int result;

   logInfo("Add a device to the table");
   result = sqlite3_exec(db, "insert into devices values(1, 2, 'Test', 1)", NULL, NULL, &errorString);
   BOOST_REQUIRE_EQUAL(result, SQLITE_OK);
   logDebug("Successfully added device");
   sqlite3_free(errorString);

   logInfo("Add another device to the table");
   result = sqlite3_exec(db, "insert into devices values(3, 4, 'Test2', 0)", NULL, NULL, &errorString);
   BOOST_REQUIRE_EQUAL(result, SQLITE_OK);
   logDebug("Successfully added device");
   sqlite3_free(errorString);

   logInfo("Read the inserted device from the table");
   result = sqlite3_exec(db, "select * from devices", c_callback, this, &errorString);
   BOOST_REQUIRE_EQUAL(result, SQLITE_OK);
   BOOST_REQUIRE_EQUAL(receivedCount, 2);
   sqlite3_free(errorString);

   // Reset the received Count
   receivedCount = 0;
   logInfo("Attempt to read from table for device that does not exist");
   result = sqlite3_exec(db, "select * from devices where id=5", c_callback, this, &errorString);
   BOOST_REQUIRE_EQUAL(result, SQLITE_OK);
   BOOST_REQUIRE_EQUAL(receivedCount, 0);
   sqlite3_free(errorString);

   logInfo("Finished with test");
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

BOOST_FIXTURE_TEST_CASE(Test_ReadRowsFromDevicesUsingStep, DBFixture)
{
   printTestCase(current_test_case().p_name.value);

   // Add a row to the database
   logInfo("Add a device to the table");
   int result = sqlite3_exec(db, "insert into devices values(1, 2, 'Test', 1)", NULL, NULL, NULL);
   BOOST_REQUIRE_EQUAL(result, SQLITE_OK);
   logDebug("Successfully added device");
   
   // Make sure that row is in the database
   logInfo("Read the inserted device from the table");
   sqlite3_stmt *statement;
   int prepareRv = sqlite3_prepare_v2(db, "select * from devices", -1, &statement, 0);
   BOOST_REQUIRE_EQUAL(prepareRv, SQLITE_OK);

   int numColumns = sqlite3_column_count(statement);
   BOOST_REQUIRE_EQUAL(4, numColumns);

   // First step should return a row
   result = sqlite3_step(statement);
   BOOST_REQUIRE_EQUAL(SQLITE_ROW, result);

   for(int columnIndex = 0; columnIndex < numColumns; columnIndex++)
   {
      std::string columnName(sqlite3_column_name(statement, columnIndex));
      if(!columnName.compare("id"))
      {
         BOOST_REQUIRE_EQUAL(1, sqlite3_column_int64(statement, columnIndex));
      }
      else if(!columnName.compare("uid"))
      {
         BOOST_REQUIRE_EQUAL(2, sqlite3_column_int(statement, columnIndex));
      }
      else if(!columnName.compare("name"))
      {
         std::string nameString(reinterpret_cast<const char*>(sqlite3_column_text(statement, columnIndex)));
         BOOST_REQUIRE_EQUAL(0, nameString.compare("Test"));
      }
      else if(!columnName.compare("connected"))
      {
         BOOST_REQUIRE_EQUAL(true, sqlite3_column_int(statement, columnIndex));
      }
      else
      {
         std::stringstream errorSS;
         errorSS << "Unknown column name received: " << columnName;
         BOOST_FAIL(errorSS.str());
      }
   }

   // Next step should return an end
   result = sqlite3_step(statement);
   BOOST_REQUIRE_EQUAL(SQLITE_DONE, result);

   // Finalize the statement
   sqlite3_finalize(statement);

   logInfo("Finished with test");
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

BOOST_FIXTURE_TEST_CASE(Test_InsertAndUpdateApps, DBFixture)
{
   printTestCase(current_test_case().p_name.value);

   char *errorString;
   int result;

   logInfo("Attempting to write a row to apps with too few values");
   result = sqlite3_exec(db, "insert into apps values(1,2)", NULL, NULL, &errorString);
   BOOST_REQUIRE_EQUAL(result, SQLITE_ERROR);
   logDebug("Error correctly returned (%s)", errorString);
   sqlite3_free(errorString);

   logInfo("Attempting to write a row to apps with too many values");
   result = sqlite3_exec(db, "insert into apps values(1,2,3,4)", NULL, NULL, &errorString);
   BOOST_REQUIRE_EQUAL(result, SQLITE_ERROR);
   logDebug("Error correctly returned (%s)", errorString);
   sqlite3_free(errorString);

   logInfo("Attempting to write a row to apps with invalid value type for the primary key");
   result = sqlite3_exec(db, "insert into apps values(\"Test\", 1, 2)", NULL,NULL, &errorString);
   BOOST_REQUIRE_EQUAL(result, SQLITE_MISMATCH);
   logDebug("Error correctly returned (%s)", errorString);
   sqlite3_free(errorString);

   logInfo("Attempting to write a row to apps with a NULL deviceId");
   result = sqlite3_exec(db, "insert into apps values(2, 3, NULL)", NULL, NULL, &errorString);
   BOOST_REQUIRE_EQUAL(result, SQLITE_OK);
   logDebug("Successfully added app");
   sqlite3_free(errorString);

   logInfo("Attempt to add another app with the same id (primary key)");
   result = sqlite3_exec(db, "insert into apps values(2, 4, NULL)", NULL,NULL, &errorString);
   BOOST_REQUIRE_EQUAL(result, SQLITE_CONSTRAINT);
   logDebug("Error correctly returned (%s)", errorString);
   sqlite3_free(errorString);

   logInfo("Update the app to have a deviceID with no devices inserted");
   result = sqlite3_exec(db, "update apps set deviceId=1 where id=2", NULL, NULL, &errorString);
   BOOST_REQUIRE_EQUAL(result, SQLITE_CONSTRAINT);
   logDebug("Error correctly returned (%s)", errorString);
   sqlite3_free(errorString);

   logInfo("Insert a device with the correct ID and then try to update the app table again");
   result = sqlite3_exec(db, "insert into devices values(1,5,'Test', 1)", NULL, NULL, NULL);
   BOOST_REQUIRE_EQUAL(result, SQLITE_OK);
   result = sqlite3_exec(db, "update apps set deviceId=1 where id=2", NULL, NULL, &errorString);
   BOOST_REQUIRE_EQUAL(result, SQLITE_OK);
   sqlite3_free(errorString);

   logInfo("Finished with test");
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

BOOST_FIXTURE_TEST_CASE(Test_ReadRowsFromApps, DBFixture)
{
   printTestCase(current_test_case().p_name.value);

   int result;

   logInfo("Add a device to the table");
   result = sqlite3_exec(db, "insert into devices values(1, 1, 'Test', 1)", NULL, NULL, NULL);
   BOOST_REQUIRE_EQUAL(result, SQLITE_OK);
   logDebug("Successfully added device");

   logInfo("Add an app to the table");
   result = sqlite3_exec(db, "insert into apps values(2, 2, 1)", NULL, NULL, NULL);
   BOOST_REQUIRE_EQUAL(result, SQLITE_OK);
   logDebug("Successfully added app");

   logInfo("Add another app to the table");
   result = sqlite3_exec(db, "insert into apps values(3, 3, 1)", NULL, NULL, NULL);
   BOOST_REQUIRE_EQUAL(result, SQLITE_OK);
   logDebug("Successfully added app");

   logInfo("Read the inserted apps from the table");
   result = sqlite3_exec(db, "select * from apps", c_callback, this, NULL);
   BOOST_REQUIRE_EQUAL(result, SQLITE_OK);
   BOOST_REQUIRE_EQUAL(receivedCount, 2);

   receivedCount = 0;
   logInfo("Attempt to read from app by app ID");
   result = sqlite3_exec(db, "select * from apps where id=2", c_callback, this, NULL);
   BOOST_REQUIRE_EQUAL(result, SQLITE_OK);
   BOOST_REQUIRE_EQUAL(receivedCount, 1);

   receivedCount = 0;
   logInfo("Attempt to read from table for app that does not exist");
   result = sqlite3_exec(db, "select * from apps where id=5", c_callback, this, NULL);
   BOOST_REQUIRE_EQUAL(result, SQLITE_OK);
   BOOST_REQUIRE_EQUAL(receivedCount, 0);

   logInfo("Finished with test");
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

