#ifndef TEMPL_TEMPORALLY_EXPANDED_NETWORK_HPP
#define TEMPL_TEMPORALLY_EXPANDED_NETWORK_HPP

#include <limits>
#include <vector>
#include <stdexcept>
#include <algorithm>
#include <graph_analysis/BaseGraph.hpp>
#include <graph_analysis/io/GraphvizWriter.hpp>
#include <graph_analysis/io/GraphvizGridStyle.hpp>
#include <graph_analysis/WeightedEdge.hpp>
#include "solvers/temporal/point_algebra/TimePoint.hpp"
#include "Tuple.hpp"
#include "RoleInfoWeightedEdge.hpp"

namespace templ {

/**
 * \class TemporallyExpandedNetwork
 * \tparam D0 First dimension, e.g., a constant such as Location
 * \tparam D1 Second dimension, which is Timepoint by default
 * \tparam TUPLE Additional information that can be stored in a map and is
 * accessible throught the key formed by std::pair<D0,D1>
 * \brief The temporally expanded network connects all tuples of the first
 * dimension (D0) if they belong to the same value of the second dimension D1
 * and assigns an infinite weight on this edge
 */
template<typename D0, typename D1 = templ::solvers::temporal::point_algebra::TimePoint::Ptr, typename TUPLE = Tuple<D0,D1> >
class TemporallyExpandedNetwork
{

public:
    typedef D0 value_t;
    typedef D1 timepoint_t;

    typedef TUPLE tuple_t;

    typedef std::vector<D0> ValueList;
    typedef std::vector<D1> TimePointList;

protected:
    ValueList mValues;
    TimePointList mTimepoints;
    graph_analysis::Edge::Ptr mpLocalTransitionEdge;
    graph_analysis::BaseGraph::Ptr mpGraph;

    typedef std::pair<value_t, timepoint_t> ValueTimePair;
    /// map allows to resolve from key-value --> tuple Ptr
    typedef std::map< ValueTimePair, typename tuple_t::Ptr > TupleMap;
    TupleMap mTupleMap;

public:
    TemporallyExpandedNetwork()
    {}

    TemporallyExpandedNetwork(const ValueList& values, const TimePointList& timepoints, const graph_analysis::Edge::Ptr& locationTransitionEdge = graph_analysis::Edge::Ptr())
        : mValues(values)
        , mTimepoints(timepoints)
        , mpLocalTransitionEdge(locationTransitionEdge)
    {
        if(mValues.empty())
        {
            throw std::invalid_argument("templ::TemporallyExpandedNetwork: cannot construct network"
                    " since value list is empty");
        }
        if(mTimepoints.empty())
        {
            throw std::invalid_argument("templ::TemporallyExpandedNetwork: cannot construct network"
                    " since timepoint list is empty");
        }
        if(!mpLocalTransitionEdge)
        {
            RoleInfoWeightedEdge::Ptr weightedEdge(new RoleInfoWeightedEdge());
            weightedEdge->setWeight(std::numeric_limits<graph_analysis::WeightedEdge::value_t>::max());
            mpLocalTransitionEdge = weightedEdge;
        }
        initialize();
    }

    virtual ~TemporallyExpandedNetwork() {}

    /**
     * Set the edge that has to be assigned for all transitions between same
     * types of the first dimension
     */
    void setLocalTransitionEdge(const graph_analysis::Edge::Ptr& edge) { mpLocalTransitionEdge = edge; }

    // Construction of the basic time-expanded network
    //
    // (t0,v0)    (t0,v1)
    //    |          |
    // (t1,v0)    (t1,v1)
    //
    void initialize()
    {
        mpGraph = graph_analysis::BaseGraph::getInstance();

        typename ValueList::const_iterator lit = mValues.begin();
        for(; lit != mValues.end(); ++lit)
        {
            typename tuple_t::Ptr previousTuple;

            typename TimePointList::const_iterator tit = mTimepoints.begin();
            for(; tit != mTimepoints.end(); ++tit)
            {
                typename tuple_t::Ptr currentTuple(new tuple_t(*lit, *tit));
                mpGraph->addVertex(currentTuple);

                mTupleMap[ ValueTimePair(*lit, *tit) ] = currentTuple;

                if(previousTuple)
                {
                    using namespace graph_analysis;

                    Edge::Ptr edge = mpLocalTransitionEdge->clone();
                    edge->setSourceVertex(previousTuple);
                    edge->setTargetVertex(currentTuple);
                    mpGraph->addEdge(edge);
                }
                previousTuple = currentTuple;
            }
        }
    }

    const graph_analysis::BaseGraph::Ptr& getGraph() const { return mpGraph; }

    /**
     * Return the list of values, e.g. for the SpaceTime::Network that will mean
     * the locations
     */
    const ValueList& getValues() const { return mValues; }

    /**
     * Return the list of timepoints
     */
    const TimePointList& getTimepoints() const { return mTimepoints; }

    void addTuple(const D0& value, const D1& timepoint, const typename tuple_t::Ptr& tuple)
    {
        mTupleMap[ ValueTimePair(value, timepoint) ] = tuple;
    }

    /**
     * Retrieve a tuple (actually a graph vertex) by the given key tuple
     */
    typename tuple_t::Ptr tupleByKeys(const D0& value, const D1& timepoint) const
    {
        typename TupleMap::const_iterator cit = mTupleMap.find( ValueTimePair(value,timepoint) );
        if(cit != mTupleMap.end())
        {
            return cit->second;
        }

        throw std::invalid_argument("TemporallyExpandedNetwork::tupleByKeys: key does not exist");
    }

    void save(const std::string& filename) const
    {
        using namespace graph_analysis::io;
        GraphvizWriter gvWriter("dot","canon");
        GraphvizGridStyle::Ptr style(new GraphvizGridStyle(
                    mValues.size(),
                    mTimepoints.size(),
                    bind(&TemporallyExpandedNetwork<D0,D1,TUPLE>::getRow,this, placeholder::_1),
                    bind(&TemporallyExpandedNetwork<D0,D1,TUPLE>::getColumn,this, placeholder::_1)
                    ));
        style->setColumnScalingFactor(5.0);
        style->setRowScalingFactor(5.0);

        gvWriter.setStyle(style);
        gvWriter.write(filename, getGraph());
    }

    const ValueTimePair& getValueTimePair(const typename tuple_t::Ptr& searchTuple) const
    {
        typename TupleMap::const_iterator cit = mTupleMap.begin();
        for(; cit != mTupleMap.end(); ++cit)
        {
            const ValueTimePair& valueTimePair = cit->first;
            typename tuple_t::Ptr tuple = cit->second;
            if(tuple ==  searchTuple)
            {
                return valueTimePair;
            }
        }
        throw std::invalid_argument("templ::TemporallyExpandedNetwork::getValueTimePair: could not find provided tuple in network");
    }

    const D0& getValue(const typename tuple_t::Ptr& tuple) const
    {
        return getValueTimePair(tuple).first;
    }

    const D1& getTimepoint(const typename tuple_t::Ptr& tuple) const
    {
        return getValueTimePair(tuple).second;
    }

    size_t getColumn(const graph_analysis::Vertex::Ptr& vertex) const
    {
        typename tuple_t::Ptr tuple = dynamic_pointer_cast<tuple_t>(vertex);
        const D0& value = getValue(tuple);

        typename ValueList::const_iterator cit = std::find_if(mValues.begin(), mValues.end(), [value](const D0& other)
                {
                    return value == other;
                });
        return std::distance(mValues.begin(), cit);
    }

    size_t getRow(const graph_analysis::Vertex::Ptr& vertex) const
    {
        typename tuple_t::Ptr tuple = dynamic_pointer_cast<tuple_t>(vertex);
        const D1& timepoint = getTimepoint(tuple);

        typename TimePointList::const_iterator cit = std::find_if(mTimepoints.begin(), mTimepoints.end(), [timepoint](const D1& other)
                {
                    return timepoint == other;
                });
        return std::distance(mTimepoints.begin(), cit);
    }
};

} // end namespace templ
#endif // TEMPL_TEMPORALLY_EXPANDED_NETWORK_HPP
