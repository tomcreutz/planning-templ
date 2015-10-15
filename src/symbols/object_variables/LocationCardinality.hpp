#ifndef TEMPL_SYMBOLS_OBJECT_VARIABLES_LOCATION_CARDINALITY_HPP
#define TEMPL_SYMBOLS_OBJECT_VARIABLES_LOCATION_CARDINALITY_HPP

#include <templ/symbols/constants/Location.hpp>
#include <templ/symbols/ObjectVariable.hpp>
#include <owlapi/OWLApi.hpp>

namespace templ {
namespace symbols {
namespace object_variables {

class LocationCardinality : public ObjectVariable
{
    constants::Location::Ptr mLocation;
    uint32_t mCardinality;
    owlapi::model::OWLCardinalityRestriction::CardinalityRestrictionType mCardinalityRestrictionType;

public:
    typedef boost::shared_ptr<LocationCardinality> Ptr;

    LocationCardinality(const constants::Location::Ptr location,
            uint32_t cardinality = 1,
            owlapi::model::OWLCardinalityRestriction::CardinalityRestrictionType type = owlapi::model::OWLCardinalityRestriction::MIN
    );

    virtual ~LocationCardinality();

    constants::Location::Ptr getLocation() const  { return mLocation; }
    uint32_t getCardinality() const { return mCardinality; }
    owlapi::model::OWLCardinalityRestriction::CardinalityRestrictionType getCardinalityRestrictionType() const { return mCardinalityRestrictionType; }

    virtual bool equals(const Symbol::Ptr& symbol) const;
    virtual std::string toString() const; 
};

} // end namespace object_variables
} // end namespace symbols
} // end templ
#endif // TEMPL_SYMBOLS_OBJECT_VARIABLES_LOCATION_CARDINALITY_HPP
