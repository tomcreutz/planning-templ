#include <boost/test/unit_test.hpp>
#include <templ/solvers/temporal/QualitativeTemporalConstraintNetwork.hpp>

using namespace templ::solvers;
using namespace templ::solvers::temporal;

BOOST_AUTO_TEST_SUITE(qualitative_temporal_constraint_network)

BOOST_AUTO_TEST_CASE(consistency_checking)
{
    {
        QualitativeTemporalConstraintNetwork qtcn;
        point_algebra::TimePoint::Ptr tp0(new point_algebra::QualitativeTimePoint("tp0"));
        point_algebra::TimePoint::Ptr tp1(new point_algebra::QualitativeTimePoint("tp1"));
        point_algebra::TimePoint::Ptr tp2(new point_algebra::QualitativeTimePoint("tp2"));


        qtcn.addConstraint(tp0, tp1, point_algebra::QualitativeTimePointConstraint::GreaterOrEqual);
        qtcn.addConstraint(tp1, tp2, point_algebra::QualitativeTimePointConstraint::GreaterOrEqual);

        BOOST_REQUIRE_MESSAGE(qtcn.isConsistent(), "qtcn is consistent");
    }
    {
        QualitativeTemporalConstraintNetwork qtcn;
        point_algebra::TimePoint::Ptr tp0(new point_algebra::QualitativeTimePoint("tp0"));
        point_algebra::TimePoint::Ptr tp1(new point_algebra::QualitativeTimePoint("tp1"));
        point_algebra::TimePoint::Ptr tp2(new point_algebra::QualitativeTimePoint("tp2"));

        qtcn.addConstraint(tp0, tp1, point_algebra::QualitativeTimePointConstraint::GreaterOrEqual);
        qtcn.addConstraint(tp1, tp2, point_algebra::QualitativeTimePointConstraint::GreaterOrEqual);
        qtcn.addConstraint(tp0, tp2, point_algebra::QualitativeTimePointConstraint::Less);

        BOOST_REQUIRE_MESSAGE(!qtcn.isConsistent(), "qtcn is inconsistent");
    }
    {
        QualitativeTemporalConstraintNetwork qtcn;
        point_algebra::TimePoint::Ptr tp0(new point_algebra::QualitativeTimePoint("tp0"));
        point_algebra::TimePoint::Ptr tp1(new point_algebra::QualitativeTimePoint("tp1"));
        point_algebra::TimePoint::Ptr tp2(new point_algebra::QualitativeTimePoint("tp2"));

        qtcn.addConstraint(tp0, tp1, point_algebra::QualitativeTimePointConstraint::Greater);
        qtcn.addConstraint(tp1, tp2, point_algebra::QualitativeTimePointConstraint::Less);
        qtcn.addConstraint(tp0,tp2,  point_algebra::QualitativeTimePointConstraint::Less);

        BOOST_REQUIRE_MESSAGE(qtcn.isConsistent(), "qtcn is consistent");
    }

    {
        QualitativeTemporalConstraintNetwork qtcn;
        point_algebra::TimePoint::Ptr tp0(new point_algebra::QualitativeTimePoint("tp0"));
        point_algebra::TimePoint::Ptr tp1(new point_algebra::QualitativeTimePoint("tp1"));

        qtcn.addConstraint(tp0,tp1, point_algebra::QualitativeTimePointConstraint::Less);
        qtcn.addConstraint(tp1,tp0, point_algebra::QualitativeTimePointConstraint::Greater);

        BOOST_REQUIRE_MESSAGE(qtcn.isConsistent(), "qtcn is consistent for exact timepoint");
    }
    {
        QualitativeTemporalConstraintNetwork qtcn;
        point_algebra::TimePoint::Ptr tp0(new point_algebra::QualitativeTimePoint("tp0"));
        point_algebra::TimePoint::Ptr tp1(new point_algebra::QualitativeTimePoint("tp1"));

        qtcn.addConstraint(tp0,tp1, point_algebra::QualitativeTimePointConstraint::Greater);
        qtcn.addConstraint(tp1,tp0, point_algebra::QualitativeTimePointConstraint::Greater);

        BOOST_REQUIRE_MESSAGE(!qtcn.isConsistent(), "qtcn is not consistent for contradicting timepoint relationships");
    }
    {
        QualitativeTemporalConstraintNetwork qtcn;
        point_algebra::TimePoint::Ptr tp0(new point_algebra::QualitativeTimePoint("tp0"));
        point_algebra::TimePoint::Ptr tp1(new point_algebra::QualitativeTimePoint("tp1"));

        qtcn.addConstraint(tp0,tp1, point_algebra::QualitativeTimePointConstraint::GreaterOrEqual);
        qtcn.addConstraint(tp0,tp1, point_algebra::QualitativeTimePointConstraint::Less);

        BOOST_REQUIRE_MESSAGE(!qtcn.isConsistent(), "qtcn is not consistent for contradicting timepoint relationships");
    }
}


BOOST_AUTO_TEST_SUITE_END()