#ifndef TEMPL_SOLVERS_TEMPORAL_TEMPORAL_CONSTRAINT_NETWORK
#define TEMPL_SOLVERS_TEMPORAL_TEMPORAL_CONSTRAINT_NETWORK

#include <templ/solvers/ConstraintNetwork.hpp>
#include <templ/solvers/temporal/Bounds.hpp>
#include <templ/solvers/temporal/point_algebra/TimePoint.hpp>
#include <templ/solvers/temporal/IntervalConstraint.hpp>
#include <templ/solvers/temporal/point_algebra/QualitativeTimePointConstraint.hpp>

namespace templ {
namespace solvers {
namespace temporal {

/**
 * \class TemporalConstraintNetwork
 * \brief Abstraction of qualitative and quantitative temporal constraint
 * networks
 */
class TemporalConstraintNetwork : public ConstraintNetwork
{
protected:
    // graph to compute distance between vertices
    graph_analysis::BaseGraph::Ptr mpDistanceGraph;

public:
    typedef shared_ptr<TemporalConstraintNetwork> Ptr;

    TemporalConstraintNetwork();
    virtual ~TemporalConstraintNetwork();
    TemporalConstraintNetwork(const TemporalConstraintNetwork& other);

    /**
     * Add timepoint constraint to the constraint network
     * \return Added constraint
     */
    virtual point_algebra::QualitativeTimePointConstraint::Ptr addQualitativeConstraint(const point_algebra::TimePoint::Ptr&, const point_algebra::TimePoint::Ptr&, point_algebra::QualitativeTimePointConstraint::Type) { throw std::runtime_error("templ::solvers::temporal::TemporalConstraintNetwork::addQualitativeConstraint is not implemented"); }

    /**
     * Remove a constraint from the constraint network
     */
    virtual void removeQualitativeConstraint(const point_algebra::QualitativeTimePointConstraint::Ptr&) { throw std::runtime_error("templ::solvers::temporal::TemporalConstraintNetwork::removeQualitativeConstraint is not implemented"); }
    /**
     * Get the known and consolidated constraint between two timepoints
     * \return consolidated timepoint constraint
     */
    virtual point_algebra::QualitativeTimePointConstraint::Type getQualitativeConstraint(const point_algebra::TimePoint::Ptr& t1, const point_algebra::TimePoint::Ptr& t2);

    /**
     * Add a TimePoint to the internal (distance) graph
     */
    void addTimePoint(const point_algebra::TimePoint::Ptr& t) { mpDistanceGraph->addVertex(t); }

    /**
     * Add interval, i.e., and edge between two TimePoints to the internal
     * (distance) graph
     */
    void addIntervalConstraint(const IntervalConstraint::Ptr& i) { mpDistanceGraph->addEdge(i); }

    virtual bool isConsistent() { throw std::runtime_error("templ::solvers::temporal::TemporalConstraintNetwork::isConsistent is not implemented"); }

    /**
     * \brief Simple Temporal Problem (STP)
     * \details stp(N) is generated by upper-lower bounds of range on disjunctive intervals
     */
    void stp();

    // the intersection between a temporal constraint network and a simple temporal constraint network given as argument by its DistanceGraph
    graph_analysis::BaseGraph::Ptr intersection(graph_analysis::BaseGraph::Ptr other);

    graph_analysis::BaseGraph::Ptr getDistanceGraph() const { return mpDistanceGraph; }

    // change a simple temporal constraint network into a weighted graph
    // Upper and lower bounds of each interval are added as edges in forward and backward
    // direction between two edges
    // A --- weight: upper bound   --> B
    // B --- weight: - lower bound --> A
    // the lower bound will be added as negative cost
    graph_analysis::BaseGraph::Ptr toWeightedGraph();

    /**
     * \brief Compute the minimal network of a simple temporal network using the shortest-path algorithm (Floyd-Warshall)
     */
    virtual void minNetwork();

    /**
     * \brief Check if temporal constraint network has an equal DistanceGraph
     */
    bool equals(const graph_analysis::BaseGraph::Ptr& distanceGraph);

    /**
     * \brief Upper-Lower-Tightening Algorithm
     * \detail Input: A Temporal Constraint Network T
     * Output: A tighter Temporal Constraint Network equivalent to T
     * Steps:
     * N <- T
     * repeat
     *      Compute N1 <- STP(N)
     *      N2 <- compute the minimal network of N1 (minNetwork)
     *      N3 <- intersection between N2 and N
     * until (N3 = N) or inconsistent
     * \throw If we get an inconsistent network; the algorithm will throw (from Floyd-Warshall)
     */
    void upperLowerTightening();
    
    /**
     * Get number of edges of the internal (distance) graph
     * \returns the number of edges in a temporal constraint network
     */
    int getEdgeNumber();

    /**
     * Save to file (dot and gexf will be used) -- suffix will be automatically
     * appended
     */
    virtual void save(const std::string& filename) const;

    /**
     * Sort a list of timepoints based on the constraint network
     */
    void sort(std::vector<point_algebra::TimePoint::Ptr>& timepoints) const;

protected:
    virtual ConstraintNetwork* getClone() const { return new TemporalConstraintNetwork(*this); }

};

} // end namespace temporal
} // end namespace solvers
} // end namespace templ
#endif // TEMPL_SOLVERS_TEMPORAL_TEMPORAL_CONSTRAINT_NETWORK
