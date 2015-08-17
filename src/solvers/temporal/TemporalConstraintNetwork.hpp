#ifndef TEMPL_SOLVERS_TEMPORAL_TEMPORAL_CONSTRAINT_NETWORK
#define TEMPL_SOLVERS_TEMPORAL_TEMPORAL_CONSTRAINT_NETWORK

#include <templ/solvers/ConstraintNetwork.hpp>
#include <templ/solvers/temporal/Bounds.hpp>
#include <templ/solvers/temporal/point_algebra/TimePoint.hpp>
#include <templ/solvers/temporal/IntervalConstraint.hpp>

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
    graph_analysis::BaseGraph::Ptr mpDistanceGraph;

public:
    typedef boost::shared_ptr<TemporalConstraintNetwork> Ptr;

    TemporalConstraintNetwork();

    virtual ~TemporalConstraintNetwork();

    void addTimePoint(point_algebra::TimePoint::Ptr t) { mpDistanceGraph->addVertex(t); }

    void addIntervalConstraint(IntervalConstraint::Ptr i) { mpDistanceGraph->addEdge(i); }

    virtual bool isConsistent() { throw std::runtime_error("templ::solvers::temporal::TemporalConstraintNetwork::isConsistent is not implemented"); }

    // stp(N) is generated by upper-lower bounds of range on disjunctive intervals
    void stp();

    // the intersection between a temporal constraint network and a simple temporal constraint network given as argument by its DistanceGraph
    graph_analysis::BaseGraph::Ptr intersection(graph_analysis::BaseGraph::Ptr other);

    graph_analysis::BaseGraph::Ptr getDistanceGraph() const { return mpDistanceGraph; }

    // change a simple temporal constraint network into a weighted graph
    // Upper and lower bound are added as edges in forward and backward
    // direction between two edges
    // A --- weight: upper bound   --> B
    // B --- weight: - lower bound --> A
    // the lower bound will be added as negative cost
    graph_analysis::BaseGraph::Ptr toWeightedGraph();

    // compute the minimal network of a simple temporal network using the shortest-path algorithm (Floyd-Warshall)
    void minNetwork();

    // check if 2 temporal constraint networks are the same (second one given as argument by its DistranceGraph)  
    bool areEqual(graph_analysis::BaseGraph::Ptr other);

    // Upper-Lower-Tightening Algorithm
    // Input: A Temporal Constraint Network T
    // Output: A tighter Temporal Constraint Network equivalent to T
    /* Steps:
    * N <- T
    * repeat
    *      Compute N1 <- STP(N)
    *      N2 <- compute the minimal network of N1 (minNetwork)
    *      N3 <- intersection between N2 and N
    * until (N3 = N) or inconsistent
    */
    // If we get an inconsistent network; the algorithm will return an error (from Floyd-Warshall)
    void upperLowerTightening();
    
    // returns the number of edges in a temporal constraint network
    int getEdgeNumber();

};

} // end namespace temporal
} // end namespace solvers
} // end namespace templ
#endif // TEMPL_SOLVERS_TEMPORAL_TEMPORAL_CONSTRAINT_NETWORK
