#include "TimePointComparator.hpp"

namespace templ {
namespace solvers {
namespace temporal {
namespace point_algebra {

TimePointComparator::TimePointComparator(QualitativeTemporalConstraintNetwork::Ptr tcn)
    : mpTemporalConstraintNetwork(tcn)
{
    if(mpTemporalConstraintNetwork && !mpTemporalConstraintNetwork->isConsistent())
    {
        throw std::invalid_argument("templ::solvers::temporal::point_algebra::TimePointComparator: given constraint network is not consistent -- cannot construct comparator");
    }
}

bool TimePointComparator::equals(TimePoint::Ptr t0, TimePoint::Ptr t1) const
{
    return t0->equals(t1);
}

bool TimePointComparator::greaterThan(TimePoint::Ptr t0, TimePoint::Ptr t1) const
{
    if(t0->getType() != t1->getType())
    {
        throw std::invalid_argument("templ::solvers::temporal::point_algebra::TimePointComparator::greaterThan: cannot compare different types of TimePoints");
    }

    if(t0->getType() == TimePoint::QUANTITATIVE)
    {
        throw std::invalid_argument("templ::solvers::temporal::point_algebra::TimePointComparator::greaterThan: cannot compare different types of TimePoints");
    } else if(t1->getType() == TimePoint::QUALITATIVE)
    {
        if(!mpTemporalConstraintNetwork)
        {
            throw std::runtime_error("templ::solvers::temporal::point_algebra::TimePointComparator::greaterThan: comparing qualitive timepoints, but not QualitativeTimePointConstraintNetwork given to comparator");
        }

        std::vector<point_algebra::QualitativeTimePointConstraint::Ptr> constraints = mpTemporalConstraintNetwork->getConstraints(t0,t1);
        if(constraints.size() == 1 && constraints[0]->getType() == point_algebra::QualitativeTimePointConstraint::Empty)
        {
            throw std::invalid_argument("templ::solvers::temporal::point_algebra::TimePointComparator::greaterThan: no constraints defined between given timepoints");
        }

        std::vector<point_algebra::QualitativeTimePointConstraint::Ptr>::const_iterator cit = constraints.begin();
        for(; cit != constraints.end(); ++cit)
        {
            if( (*cit)->getType() == point_algebra::QualitativeTimePointConstraint::Greater)
            {
                return true;
            }
        }

        return false;
    } else {
        throw std::invalid_argument("templ::solvers::temporal::point_algebra::TimePointComparator::greaterThan: cannot compare this type of TimePoints");
    }
}

bool TimePointComparator::hasIntervalOverlap(TimePoint::Ptr a_start, TimePoint::Ptr a_end, TimePoint::Ptr b_start, TimePoint::Ptr b_end) const
{
    if(TimePointComparator::inInterval(a_start, b_start, b_end))
    {
        return true;
    } else if(TimePointComparator::inInterval(a_end, b_start, b_end))
    {
        return true;
    } else if(TimePointComparator::inInterval(b_start, a_start, a_end))
    {
        return true;
    } else if(TimePointComparator::inInterval(b_end, a_start, a_end))
    {
        return true;
    }

    return false;
}

bool TimePointComparator::inInterval(TimePoint::Ptr t0, TimePoint::Ptr i_start, TimePoint::Ptr i_end) const
{
    if(TimePointComparator::greaterOrEqual(t0, i_start) && TimePointComparator::lessOrEqual(t0, i_end))
    {
        return true;
    }
    return false;
}

} // end namespace point_algebra
} // end namespace temporal
} // end namespace solvers
} // end namespace templ