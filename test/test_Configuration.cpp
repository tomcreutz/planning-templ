#include <boost/test/unit_test.hpp>
#include "test_utils.hpp"

#include <templ/Configuration.hpp>

using namespace templ;

BOOST_AUTO_TEST_SUITE(templ_configuration)

BOOST_AUTO_TEST_CASE(load_xml)
{
    std::string xmlFile = getRootDir() + "/test/data/configuration/test-configuration.xml";
    Configuration configuration(xmlFile);
    BOOST_TEST_MESSAGE( configuration.toString() );
    BOOST_REQUIRE_MESSAGE(configuration.getValue("property_a") == "a", "Configuration: expected property_a == a, but was '" << configuration.getValue("property_a"));
    BOOST_REQUIRE_MESSAGE(configuration.getValue("property_b") == "b", "Configuration: expected property_b == b, but was '" << configuration.getValue("property_b"));
    BOOST_REQUIRE_MESSAGE(configuration.getValue("property_c/property_d") == "cd", "Configuration: expected property_c/property_d == cd, but was '" << configuration.getValue("property_c/property_d"));
    BOOST_REQUIRE_MESSAGE(configuration.getValueAsNumeric<int>("numeric_properties/int") == 121, "Configuration: expected numeric_properties/int == 121, but was '" << configuration.getValueAsNumeric<int>("numeric_properties/int"));
    BOOST_REQUIRE_MESSAGE(configuration.getValueAsNumeric<double>("numeric_properties/double") == 1.21, "Configuration: expected numeric_properties/double == 1.21, but was '" << configuration.getValueAsNumeric<double>("numeric_properties/double"));

}

BOOST_AUTO_TEST_SUITE_END()
