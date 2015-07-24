#ifndef TEMPL_SOLVERS_TEMPORAL_POINT_ALGEBRA_QUALITATIVE_TIME_POINT_HPP
#define TEMPL_SOLVERS_TEMPORAL_POINT_ALGEBRA_QUALITATIVE_TIME_POINT_HPP

#include <templ/solvers/temporal/point_algebra/TimePoint.hpp>

namespace templ {
namespace solvers {
namespace temporal {
namespace point_algebra {

/**
 * \class QualitativeTimePoint
 * \details A QualitativeTimePoint represent a labelled timepoint, 
 * which allows to formulate qualitative timepoint relationships
 *
 * A QualitativeTimePoint can have one or more aliases and identical
 * aliases identify equal timepoints, which will allow for later
 * constraint checking
 *
 */
class QualitativeTimePoint : public TimePoint
{
    std::vector<TimePoint::Label> mAliases;
    TimePoint::Label mLabel;

public:
    typedef boost::shared_ptr<QualitativeTimePoint> Ptr;

    /**
     * Default constructor
     * \param label (main) label for this Timepoint
     */
    QualitativeTimePoint(const TimePoint::Label& label);

    /**
     * Get label of this QualitativeTimePoint
     */
    const std::string& getLabel() const { return mLabel; }

    /**
     * Create instance of QualitativeTimePoint
     */
    static QualitativeTimePoint::Ptr getInstance(const TimePoint::Label& label);

    /**
     * Add an alias for this timepoint
     * \param alias Alias
     */
    void addAlias(const TimePoint::Label& alias);

    /**
     * Check if the given label is an alias (or the actual label) of this
     * QualitativeTimePoint
     * \return True if label is an alias, false otherwise
     o*/
    bool isAlias(const TimePoint::Label& label) const;

    /**
     * Check equality of two QualitativeTimePoint instances
     * \return True if they are equal, false otherwise
     */
    virtual bool operator==(const QualitativeTimePoint& other) const;

    /**
     * Check if two QualitativeTimePoint instances are distinct
     * \return True if they are distinct, false otherwise
     */
    bool operator!=(const QualitativeTimePoint& other) const { return ! (*this == other); }

    /**
     * Stringify object
     */
    virtual std::string toString() const { return "QualitativeTimePoint: " + mLabel; }
};

} // end namespace point_algebra
} // end namespace temporal
} // end namespace solvers
} // end namespace templ
#endif // TEMPL_SOLVERS_TEMPORAL_POINT_ALGEBRA_QUALITATIVE_TIME_POINT_HPP
