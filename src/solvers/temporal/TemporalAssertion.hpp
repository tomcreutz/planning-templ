#ifndef TEMPL_TEMPORAL_ASSERTION_HPP
#define TEMPL_TEMPORAL_ASSERTION_HPP

#include <vector>
#include <map>
#include <boost/shared_ptr.hpp>
#include <templ/StateVariable.hpp>
#include <templ/solvers/temporal/point_algebra/TimePointComparator.hpp>

namespace templ {
namespace solvers {
namespace temporal {

class Event;
class PersistenceCondition;

/**
 * \class TemporalAssertion
 * \brief A temporal assertion is part of a Chronicle or a Timeline
 * It can be either an Event or a PersistenceCondition
 */
class TemporalAssertion
{
public:
    enum Type { UNKNOWN = 0,
        /// Type of an Event
        EVENT,
        /// Type of a PersistenceCondition 
        PERSISTENCE_CONDITION
    };

    static std::map<Type, std::string> TypeTxt;

protected:
    /**
     * Construct temporal assertion type
     * Only possible via subclasses
     */
    TemporalAssertion(const StateVariable& stateVariable, Type type);

    virtual bool refersToSameValue(boost::shared_ptr<Event> other, const point_algebra::TimePointComparator& comparator) const { throw std::runtime_error("templ::TemporalAssertion::refersToSameValue: not implemented"); }

    virtual bool refersToSameValue(boost::shared_ptr<PersistenceCondition> other, const point_algebra::TimePointComparator& comparator) const { throw std::runtime_error("templ::TemporalAssertion::refersToSameValue: not implemented"); }

    virtual bool disjointFrom(boost::shared_ptr<Event> other, const point_algebra::TimePointComparator& comparator) const { throw std::runtime_error("templ::TemporalAssertion::disjointFrom: not implemented"); }
    virtual bool disjointFrom(boost::shared_ptr<PersistenceCondition> other, const point_algebra::TimePointComparator& comparator) const { throw std::runtime_error("templ::TemporalAssertion::disjointFrom: not implemented"); }


public:
    typedef boost::shared_ptr<TemporalAssertion> Ptr;

    virtual ~TemporalAssertion() {}

    /**
     * Get the type of the temporal assertion
     * \return Type
     */
    Type getType() const { return mType; }

    /**
     * Get the StateVariable
     */
    const StateVariable& getStateVariable() const { return mStateVariable; }

    /**
     * Check that TemporalAssertion is disjoint from another TemporalAssertion
     * \param other Other temporal assertion to compare to
     * \param comparator Comparator that allows to compare two Timepoints
     * \return True if both assertions are disjoint, false otherwise
     */
    bool isDisjointFrom(TemporalAssertion::Ptr other, const point_algebra::TimePointComparator& comparator) const;

    /**
     * Check if this TemporalAssertion refers to the same value and/or same timepoint 
     * as the other TemporalAssertion
     * \throw std::invalid_argument if invalid type is used
     */
    bool isReferringToSameValue(TemporalAssertion::Ptr other, const point_algebra::TimePointComparator& comparator) const;

    virtual std::string toString() const;

private:
    Type mType;
    StateVariable mStateVariable;
};

typedef std::vector<TemporalAssertion::Ptr> TemporalAssertionList;

} // end namespace temporal
} // end namespace solvers
} // end namespace templ
#endif // TEMPL_TEMPORAL_ASSERTION_HPP
