#ifndef TEMP_SOLVERS_SOLUTION_ANALYSIS_HPP
#define TEMP_SOLVERS_SOLUTION_ANALYSIS_HPP

#include "../Mission.hpp"
#include "../SpaceTime.hpp"
#include "Solution.hpp"
#include "FluentTimeResource.hpp"
#include "../Plan.hpp"
#include "temporal/TemporalConstraintNetwork.hpp"
#include <organization_model/Metric.hpp>

namespace templ {
namespace solvers {

/**
 * Allow to analyse an existing solution upon quality, cost, and
 * redundancy
 */
class SolutionAnalysis
{
public:
    typedef std::pair< organization_model::ModelPool::List, organization_model::ModelPool::List >
        MinMaxModelPools;

    /**
     * An existing solution will provide contain RoleInfoTuple as vertices
     * and An existing solution will provide contain RoleInfoTuple as vertices
     * and RoleInfoWeightedEdge as edges
     *
     * The successful assignment of roles to a vertex will result in a entry of
     * the RoleInfo::ASSIGNED tagged set
     * Meanwhile the default set, contains all the required roles
     * Note, that for the first timepoint the starting requirement vs. assiged have to be
     * interpreted differently, i.e. the requirements are merley defining what
     * is available at this 'source' hub
     *
     * \see SpaceTime::Network
     */
    SolutionAnalysis(const Mission::Ptr& mission, const SpaceTime::Network& solution,
            organization_model::metrics::Type metricType = organization_model::metrics::REDUNDANCY);

    void analyse();

    void save() const;

    double getQuality() const { return mQuality; }
    double getCost() const { return mCost; }
    double getMetricValue() const { return mMetricValue; }

    organization_model::Metric::Ptr getMetric() const { return mpMetric; }

    /**
     * Get the metrics, e.g., redundancy of the fluent time resource
     */
    double getMetricValue(const FluentTimeResource& ftr) const;

    /**
     * Retrieve the list of required roles / all roles that are involved in this
     * problem
     *
     * A required role is given, it its usage exceeds 1 (since the initial usage
     * is given by its availability at the initial position)
     */
    std::set<Role> getRequiredRoles(size_t minRequirement = 1) const;

    /**
     * Get the minimum required resource for the given fluent time resource
     * \return model pool of the minimum required resources
     */
    organization_model::ModelPool getMinResourceRequirements(const FluentTimeResource& ftr) const;

    /**
     * Get the maximum required resource for the given fluent time resource
     * \return model pool of the maximum required resources
     */
    organization_model::ModelPool getMaxResourceRequirements(const FluentTimeResource& ftr) const;

    /**
     * Get the corresponding space time tuple for source time of a fluent time resource
     * definition
     */
    SpaceTime::Network::tuple_t::Ptr getFromTuple(const FluentTimeResource& ftr) const;

    /**
     * Get the corresponding space time tuple for the target time of a fluent time resource
     * definition
     */
    SpaceTime::Network::tuple_t::Ptr getToTuple(const FluentTimeResource& ftr) const;

    /**
     * Get the maximum number of missing resource requirements, i.e.
     * required (by original mission definition) resources - minimum available resources
     *
     * This takes into account resolution of functionality to actual agents to
     * come to a particular solution
     */
    organization_model::ModelPoolDelta getMinMissingResourceRequirements(const FluentTimeResource& ftr) const;

    /**
     * Get the minimum number of missing resource requirements, i.e.
     * required (by original mission definition) resources - maximum available resources
     *
     * This takes into account resolution of functionality to actual agents to
     * come to a particular solution
     */
    organization_model::ModelPoolDelta getMaxMissingResourceRequirements(const FluentTimeResource& ftr) const;

    /**
     * Get the maximum number of missing resources, i.e.
     * required (by transformed mission definition) resources - minimum available resources
     *
     * This takes into account resolution of functionality to actual agents to
     * come to a particular solution
     */
    organization_model::ModelPoolDelta getMaxMissingResources(const FluentTimeResource& ftr) const;

    /**
     * Get the minimum number of missing resources, i.e.
     * required resources - maximum available resources
     *
     * This takes into account resolution of functionality to actual agents to
     * come to a particular solution
     */
    organization_model::ModelPoolDelta getMinMissingResources(const FluentTimeResource& ftr) const;

    /**
     * Get the minimum number of available resources for the given fluent time
     * resource definition
     * \return ModelPool containing the available resources (including inferred
     * functionalities)
     */
    organization_model::ModelPool getMinAvailableResources(const FluentTimeResource& ftr) const;

    /**
     * Get the maximum number of available resources for the given fluent time
     * resource definition
     *
     * This function includes all infered functionality
     * \return ModelPool containing the available resources (including inferred
     * functionalities)
     */
    organization_model::ModelPool getMaxAvailableResources(const FluentTimeResource& ftr) const;

    /**
     * Get the required resources as a pair of two model pool list for min
     * cardinalities and max cardinalities
     */
    MinMaxModelPools getRequiredResources(const symbols::constants::Location::Ptr& location, const solvers::temporal::Interval& interval) const;

    /**
     * Get the required resource for an existing fluent time resource from a
     * solution, i.e.
     * find the corresponding match in the mission description
     */
    MinMaxModelPools getRequiredResources(const FluentTimeResource& ftr) const;

    /**
     * Get the availability as list of model pools over the course of one interval
     * This accounts for all included (known) qualitative timepoints
     */
    std::vector<organization_model::ModelPool> getAvailableResources(const symbols::constants::Location::Ptr& location, const solvers::temporal::Interval& interval) const;

    /**
     * Compute a hypergraph
     * The hypergaph contains a number of RoleInfoVertex (as HyperEdge)
     * which are linked to by 'requires' Edges from existing edges
     */
    graph_analysis::BaseGraph::Ptr toHyperGraph();

    // Annotation functions
    /**
     * Provide a quantification on the transition times for this planner
     * this update the Time Distance Graph
     */
    void quantifyTime() const;

    void quantifyMetric() const;

    // End annotiation functions

    /**
     * Get the set of available resources
     */
    //organization_model::ModelPool getAvailableResources(const solvers::FluentTimeResource& e) const;

    std::string toString(size_t indent = 0) const;

    /**
     * Compute a plan for all robot systems
     */
    Plan computePlan() const;

    graph_analysis::BaseGraph::Ptr getTimeDistanceGraph() const { return mpTimeDistanceGraph; }

private:
    /**
     * Check solution with respect to the given requirement
     */
    void analyse(const solvers::FluentTimeResource& requirement);

    double degreeOfFulfillment(const solvers::FluentTimeResource& requirement);


    Mission::Ptr mpMission;
    SpaceTime::Network mSolutionNetwork;
    Plan mPlan;

    std::vector<solvers::FluentTimeResource> mResourceRequirements;
    solvers::temporal::point_algebra::TimePointComparator mTimepointComparator;

    mutable graph_analysis::BaseGraph::Ptr mpTimeDistanceGraph;

    double mQuality;
    double mCost;
    double mMetricValue;

    organization_model::Metric::Ptr mpMetric;

};

} // end namespace solvers
} // end namespace templ
#endif // TEMP_SOLVERS_SOLUTION_ANALYSIS_HPP
