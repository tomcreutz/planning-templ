#include "PlannerElement.hpp"
#include <boost/assign/list_of.hpp>

namespace templ {

std::map<PlannerElement::Type, std::string> PlannerElement::TypeTxt = boost::assign::map_list_of
    (UNKNOWN, "UNKNOWN")
    (STATE_VARIABLE, "STATE_VARIABLE")
    (OBJECT_VARIABLE, "OBJECT_VARIABLE")
    (CONSTANT, "CONSTANT")
    (TEMPORAL_VARIABLE, "TEMPORAL_VARIABLE")
    (VALUE, "VALUE")
    ;

PlannerElement::PlannerElement(const std::string& name, const std::string& type_name, Type type)
    : std::pair<std::string, std::string>(name, type_name)
    , mType(type)
{}

std::string PlannerElement::toString() const
{
    return TypeTxt[mType] + ": (" + first + ", " + second + ")";
}

} // end namespace templ
