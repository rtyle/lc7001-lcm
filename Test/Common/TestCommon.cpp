#include "TestCommon.h"
#include "Debug.h"
#include <boost/test/unit_test.hpp>

using namespace boost::unit_test::framework;
using namespace debugPrint;

namespace TestCommon
{
   void printTestCase()
   {
      std::cout << std::endl;
      logInfo("#######################################################");
      logInfo("# Running %s", current_test_case().p_name.value.c_str());
      logInfo("#######################################################");
   }
}
