#include "TransportNetwork.hpp"
#include <base/Logging.hpp>
#include <numeric/Combinatorics.hpp>
#include <gecode/minimodel.hh>

#include <gecode/gist.hh>
#include <organization_model/Algebra.hpp>
#include <organization_model/vocabularies/OM.hpp>
#include <templ/SharedPtr.hpp>
#include <templ/symbols/object_variables/LocationCardinality.hpp>
#include <templ/solvers/csp/ConstraintMatrix.hpp>
#include <templ/SpaceTimeNetwork.hpp>
#include <iomanip>

namespace templ {
namespace solvers {
namespace csp {

std::string TransportNetwork::Solution::toString(uint32_t indent) const
{
    std::stringstream ss;
    std::string hspace(indent,' ');
    {
        ModelDistribution::const_iterator cit = mModelDistribution.begin();
        size_t count = 0;
        ss << hspace << "ModelDistribution" << std::endl;
        for(; cit != mModelDistribution.end(); ++cit)
        {
            const FluentTimeResource& fts = cit->first;
            ss << hspace << "--- requirement #" << count++ << std::endl;
            ss << hspace << fts.toString() << std::endl;

            const organization_model::ModelPool& modelPool = cit->second;
            ss << modelPool.toString(indent) << std::endl;
        }
    }
    {
        TransportNetwork::ModelDistribution::const_iterator cit = mModelDistribution.begin();
        size_t count = 0;
        ss << hspace << "ModelDistribution" << std::endl;
        for(; cit != mModelDistribution.end(); ++cit)
        {
            const FluentTimeResource& fts = cit->first;
            ss << hspace << "--- requirement #" << count++ << std::endl;
            ss << hspace << fts.toString() << std::endl;

            const organization_model::ModelPool& modelPool = cit->second;
            ss << modelPool.toString(indent) << std::endl;
        }
    }
    {
        RoleDistribution::const_iterator cit = mRoleDistribution.begin();
        size_t count = 0;
        ss << hspace << "RoleDistribution" << std::endl;
        for(; cit != mRoleDistribution.end(); ++cit)
        {
            const FluentTimeResource& fts = cit->first;
            ss << hspace << "--- requirement #" << count++ << std::endl;
            ss << hspace << fts.toString() << std::endl;

            const Role::List& roles = cit->second;
            ss << hspace << Role::toString(roles) << std::endl;
        }
    }
    return ss.str();
}

TransportNetwork::SearchState::SearchState(const Mission::Ptr& mission)
    : mpMission(mission)
    , mpInitialState(NULL)
    , mpSearchEngine(NULL)
    , mType(OPEN)
{
    assert(mpMission);
    mpInitialState = TransportNetwork::Ptr(new TransportNetwork(mpMission));
    mpSearchEngine = TransportNetwork::BABSearchEnginePtr(new Gecode::BAB<TransportNetwork>(mpInitialState.get()));
}

TransportNetwork::SearchState::SearchState(const TransportNetwork::Ptr& transportNetwork,
        const TransportNetwork::BABSearchEnginePtr& searchEngine)
    : mpMission(transportNetwork->mpMission)
    , mpInitialState(transportNetwork)
    , mpSearchEngine(searchEngine)
    , mType(OPEN)
{
    assert(mpMission);
    assert(mpInitialState);
    if(!mpSearchEngine)
    {
        mpSearchEngine = TransportNetwork::BABSearchEnginePtr(new Gecode::BAB<TransportNetwork>(mpInitialState.get()));
    }
}

TransportNetwork::SearchState TransportNetwork::SearchState::next() const
{
    if(!getInitialState())
    {
        throw std::runtime_error("templ::solvers::csp::TransportNetwork::SearchState::next: "
                " next() called on an unitialized search state");
    }

    SearchState searchState(mpInitialState, mpSearchEngine);
    TransportNetwork* solvedDistribution = mpSearchEngine->next();
    if(solvedDistribution)
    {
        searchState.mSolution = solvedDistribution->getSolution();
        searchState.mType = SUCCESS;
        delete solvedDistribution;
    } else {
        searchState.mType = FAILED;
    }
    return searchState;
}

TransportNetwork::Solution TransportNetwork::getSolution() const
{
    Solution solution;
    solution.mModelDistribution = getModelDistribution();
    solution.mRoleDistribution = getRoleDistribution();
    return solution;
}

TransportNetwork::ModelDistribution TransportNetwork::getModelDistribution() const
{
    ModelDistribution solution;
    Gecode::Matrix<Gecode::IntVarArray> resourceDistribution(mModelUsage, mModelPool.size(), mResourceRequirements.size());

    // Check if resource requirements holds
    for(size_t i = 0; i < mResourceRequirements.size(); ++i)
    {
        organization_model::ModelPool modelPool;
        for(size_t mi = 0; mi < mpMission->getAvailableResources().size(); ++mi)
        {
            Gecode::IntVar var = resourceDistribution(mi, i);
            if(!var.assigned())
            {
                throw std::runtime_error("templ::solvers::csp::TransportNetwork::getSolution: value has not been assigned");
            }

            Gecode::IntVarValues v( var );

            modelPool[ mAvailableModels[mi] ] = v.val();
        }

        solution[ mResourceRequirements[i] ] = modelPool;
    }
    return solution;
}

TransportNetwork::RoleDistribution TransportNetwork::getRoleDistribution() const
{
    RoleDistribution solution;

    Gecode::Matrix<Gecode::IntVarArray> roleDistribution(mRoleUsage, /*width --> col*/ mRoles.size(), /*height --> row*/ mResourceRequirements.size());

    // Check if resource requirements holds
    for(size_t i = 0; i < mResourceRequirements.size(); ++i)
    {
        Role::List roles;
        for(size_t r = 0; r < mRoles.size(); ++r)
        {
            Gecode::IntVar var = roleDistribution(r, i);
            if(!var.assigned())
            {
                throw std::runtime_error("templ::solvers::csp::RoleDistribution::getSolution: value has not been assigned for role: '" + mRoles[r].toString() + "'");
            }

            Gecode::IntVarValues v( var );

            if( v.val() == 1 )
            {
                roles.push_back( mRoles[r] );
            }
        }

        solution[ mResourceRequirements[i] ] = roles;
    }

    return solution;
}

std::set< std::vector<uint32_t> > TransportNetwork::toCSP(const organization_model::ModelPoolSet& combinations) const
{
    std::set< std::vector<uint32_t> > csp_combinations;
    organization_model::ModelPoolSet::const_iterator cit = combinations.begin();
    for(; cit != combinations.end(); ++cit)
    {
        csp_combinations.insert( toCSP(*cit) );
    }
    return csp_combinations;
}

std::vector<uint32_t> TransportNetwork::toCSP(const organization_model::ModelPool& combination) const
{
    // return index of model and count per model
    std::vector<uint32_t> csp_combination(mModelPool.size(),0);

    organization_model::ModelPool::const_iterator cit = combination.begin();
    for(; cit != combination.end(); ++cit)
    {
        uint32_t index = systemModelToCSP(cit->first);
        csp_combination.at(index) = cit->second;
    }
    return csp_combination;
}

uint32_t TransportNetwork::systemModelToCSP(const owlapi::model::IRI& model) const
{
    owlapi::model::IRIList::const_iterator cit = std::find(mAvailableModels.begin(),
            mAvailableModels.end(), model);

    if(cit == mAvailableModels.end())
    {
        throw std::invalid_argument("templ::solvers::csp::TransportNetwork::systemModelToCSP:"
                " unknown model '" + model.toString() );
    } else {
        return (cit - mAvailableModels.begin());
    }

}

TransportNetwork::TransportNetwork(const templ::Mission::Ptr& mission)
    : Gecode::Space()
    , mpMission(mission)
    , mModelPool(mpMission->getAvailableResources())
    , mAsk(mpMission->getOrganizationModel(), mpMission->getAvailableResources(), true)
    , mResources(mpMission->getRequestedResources())
    , mIntervals(mpMission->getTimeIntervals())
    , mTimepoints(mpMission->getOrderedTimepoints())
    , mLocations(mpMission->getLocations())
    , mResourceRequirements(getResourceRequirements())
    , mModelUsage(*this, /*# of models*/ mpMission->getAvailableResources().size()*
            /*# of fluent time services*/mResourceRequirements.size(), 0, getMaxResourceCount(mModelPool)) // maximum number of model at that point
    , mAvailableModels(mpMission->getModels())
    , mRoleUsage(*this, /*width --> col */ mission->getRoles().size()* /*height --> row*/ mResourceRequirements.size(), 0, 1) // Domain 0,1 to represent activation
    , mRoles(mission->getRoles())
{
    assert( mpMission->getOrganizationModel() );
    assert(!mIntervals.empty());

    ConstraintMatrix constraintMatrix(mAvailableModels);

    if(mResourceRequirements.empty())
    {
        throw std::invalid_argument("templ::solvers::csp::TransportNetwork: no resource requirements given");
    }

    LOG_INFO_S << "TransportNetwork CSP Problem Construction" << std::endl
    << "    requested resources: " << mResources << std::endl
    << "    intervals: " << mIntervals.size() << std::endl
    << "    # requirements: " << mResourceRequirements.size() << std::endl;

    //Gecode::IntArgs x = Gecode::IntArgs::create(3,2,0);
    //Gecode::IntArgs y = Gecode::IntArgs::create(2,2,0);
    //Gecode::IntArgs z = Gecode::IntArgs::create(2,2,0);
    //LOG_DEBUG_S << "Intargs: " << x;
    //LOG_DEBUG_S << "Intargs: " << Gecode::IntSet( x );

    ////rel(*this, x + y, Gecode::SRT_SUB, z)

    Gecode::Matrix<Gecode::IntVarArray> resourceDistribution(mModelUsage, /*width --> col*/ mpMission->getAvailableResources().size(), /*height --> row*/ mResourceRequirements.size());
    // Example usage
    //
    //Gecode::IntVar v = resourceDistribution(0,0);
    //
    // Maximum upper bound
    //Gecode::IntVar v0 = resourceDistribution(0,1);
    //Gecode::IntVarArgs a;
    //a << v;
    //a << v0;

    // Bounding individual model count
    //rel(*this, v, Gecode::IRT_GQ, 0);
    //rel(*this, v, Gecode::IRT_LQ, 1);
    //rel(*this, sum(a) < 1);

    // Add explit constraints, i.e. set of model combinations can be added this
    // way
    // Gecode::TupleSet tupleSet;
    // tupleSet.add( Gecode::IntArgs::create( mission.getAvailableResources().size(), 1, 0) );
    // tupleSet.finalize();
    //
    // extensional(*this, resourceDistribution.row(0), tupleSet);

    // Outline:
    // (A) for each requirement add the min/max and existential constraints
    // for all overlapping requirements create maximum resource constraints
    //
    // (B) General resource constraints
    //     - identify overlapping fts, limit resources for these (TODO: better
    //     indentification of overlapping requirements)
    //
    //
    // (C) Minimal resource constraints associated with Time-Location
    // Part (A)
    {
        using namespace solvers::temporal;
        LOG_INFO_S << mAsk.toString();
        LOG_DEBUG_S << "Involved services: " << owlapi::model::IRI::toString(mServices, true);

        {
            std::vector<FluentTimeResource>::const_iterator fit = mResourceRequirements.begin();
            for(; fit != mResourceRequirements.end(); ++fit)
            {
                const FluentTimeResource& fts = *fit;
                LOG_DEBUG_S << "(A) Define requirement: " << fts.toString()
                            << "        available models: " << mAvailableModels << std::endl;

                // row: index of requirement
                // col: index of model type
                size_t requirementIndex = fit - mResourceRequirements.begin();
                for(size_t mi = 0; mi < mAvailableModels.size(); ++mi)
                {
                    Gecode::IntVar v = resourceDistribution(mi, requirementIndex);

                    {
                        // default min requirement is 0
                        uint32_t minCardinality = 0;
                        /// Consider additional resource cardinality constraint
                        LOG_DEBUG_S << "Check extra min cardinality for " << mAvailableModels[mi];
                        organization_model::ModelPool::const_iterator cardinalityIt = fts.minCardinalities.find( mAvailableModels[mi] );
                        if(cardinalityIt != fts.minCardinalities.end())
                        {
                            minCardinality = cardinalityIt->second;
                            LOG_DEBUG_S << "Found extra resource cardinality constraint: " << std::endl
                                << "    " << mAvailableModels[mi] << ": minCardinality " << minCardinality;
                        }
                        constraintMatrix.setMin(requirementIndex, mi, minCardinality);
                        rel(*this, v, Gecode::IRT_GQ, minCardinality);
                    }

                    uint32_t maxCardinality = mModelPool[ mAvailableModels[mi] ];
                    // setting the upper bound for this model and this service
                    // based on what the model pool can provide
                    LOG_DEBUG_S << "requirement: " << requirementIndex
                        << ", model: " << mi
                        << " IRT_GQ 0 IRT_LQ: " << maxCardinality;
                    constraintMatrix.setMax(requirementIndex, mi, maxCardinality);
                    rel(*this, v, Gecode::IRT_LQ, maxCardinality);
                }

                // Extensional constraints, i.e. specifying the allowed
                // combinations
                organization_model::ModelPoolSet allowedCombinations = fts.getDomain();
                appendToTupleSet(mExtensionalConstraints[requirementIndex], allowedCombinations);

                // there can be no empty assignment for a service
                rel(*this, sum( resourceDistribution.row(requirementIndex) ) > 0);

                // This can be equivalently modelled using a linear constraint
                // Gecode::IntArgs c(mAvailableModels.size());
                // for(size_t mi = 0; mi < mAvailableModels.size(); ++mi)
                //    c[mi] = 1;
                // linear(*this, c, resourceDistribution.row(requirementIndex), Gecode::IRT_GR, 0);
            }
        }

        for(auto pair : mExtensionalConstraints)
        {
            uint32_t requirementIndex = pair.first;
            Gecode::TupleSet& tupleSet = pair.second;

            tupleSet.finalize();
            extensional(*this, resourceDistribution.row(requirementIndex), tupleSet);
        }

        LOG_INFO_S << constraintMatrix.toString();
    }
    // Part (B) General resource constraints
    // - identify overlapping fts, limit resources for these
    {
        // Set of available models: mModelPool
        // Make sure the assignments are within resource bounds for concurrent requirements
        std::vector< std::vector<FluentTimeResource> > concurrentRequirements = FluentTimeResource::getConcurrent(mResourceRequirements, mIntervals);

        std::vector< std::vector<FluentTimeResource> >::const_iterator cit = concurrentRequirements.begin();
        for(; cit != concurrentRequirements.end(); ++cit)
        {
            LOG_DEBUG_S << "Concurrent requirements";
            const std::vector<FluentTimeResource>& concurrentFluents = *cit;
            for(size_t mi = 0; mi < mAvailableModels.size(); ++mi)
            {
                LOG_DEBUG_S << "    model: " << mAvailableModels[ mi ].toString();
                Gecode::IntVarArgs args;

                std::vector<FluentTimeResource>::const_iterator fit = concurrentFluents.begin();
                for(; fit != concurrentFluents.end(); ++fit)
                {
                    Gecode::IntVar v = resourceDistribution(mi, getFluentIndex(*fit));
                    args << v;

                    LOG_DEBUG_S << "    index: " << mi << "/" << getFluentIndex(*fit);
                }

                uint32_t maxCardinality = mModelPool[ mAvailableModels[mi] ];
                LOG_DEBUG_S << "Add general resource usage constraint: " << std::endl
                    << "     " << mAvailableModels[mi].toString() << "# <= " << maxCardinality;
                rel(*this, sum(args) <= maxCardinality);
            }
        }
    }


    // Role distribution
    Gecode::Matrix<Gecode::IntVarArray> roleDistribution(mRoleUsage, /*width --> col*/ mRoles.size(), /*height --> row*/ mResourceRequirements.size());
    for(size_t modelIndex = 0; modelIndex < mAvailableModels.size(); ++modelIndex)
    {
        for(uint32_t requirementIndex = 0; requirementIndex < mResourceRequirements.size(); ++requirementIndex)
        {
            Gecode::IntVar modelCount = resourceDistribution(modelIndex,requirementIndex);
            Gecode::IntVarArgs args;
            for(uint32_t roleIndex = 0; roleIndex < mRoles.size(); ++roleIndex)
            {
                if(isRoleForModel(roleIndex, modelIndex))
                {
                    Gecode::IntVar roleActivation = roleDistribution(roleIndex, requirementIndex);
                    args << roleActivation;
                }
            }
            rel(*this, sum(args) == modelCount);
        }
    }

    {
        // Set of available models: mModelPool
        // Make sure the assignments are within resource bounds for concurrent requirements
        std::vector< std::vector<FluentTimeResource> > concurrentRequirements = FluentTimeResource::getConcurrent(mResourceRequirements, mIntervals);

        std::vector< std::vector<FluentTimeResource> >::const_iterator cit = concurrentRequirements.begin();
        if(!concurrentRequirements.empty())
        {
            for(; cit != concurrentRequirements.end(); ++cit)
            {
                LOG_DEBUG_S << "Concurrent roles requirements: " << mRoles.size();
                const std::vector<FluentTimeResource>& concurrentFluents = *cit;

                for(size_t roleIndex = 0; roleIndex < mRoles.size(); ++roleIndex)
                {
                    Gecode::IntVarArgs args;

                    std::vector<FluentTimeResource>::const_iterator fit = concurrentFluents.begin();
                    for(; fit != concurrentFluents.end(); ++fit)
                    {
                        size_t row = getFluentIndex(*fit);
                        LOG_DEBUG_S << "    index: " << roleIndex << "/" << row;
                        Gecode::IntVar v = roleDistribution(roleIndex, row);
                        args << v;
                    }
                    rel(*this, sum(args) <= 1);
                }
            }
        } else {
            LOG_DEBUG_S << "No concurrent requirements found";
        }
    }

    // Avoid computation of solutions that are redunant
    // Gecode documentation says however in 8.10.2 that "Symmetry breaking by
    // LDSB is not guaranteed to be complete. That is, a search may still return
    // two distinct solutions that are symmetric."
    //
    Gecode::Symmetries symmetries;
    // define interchangeable columns for roles of the same model type
    owlapi::model::IRIList::const_iterator ait = mAvailableModels.begin();
    for(; ait != mAvailableModels.end(); ++ait)
    {
        const owlapi::model::IRI& currentModel = *ait;
        LOG_INFO_S << "Starting symmetry column for model: " << currentModel.toString();
        Gecode::IntVarArgs sameModelColumns;
        for(int c = 0; c < roleDistribution.width(); ++c)
        {
            if( mRoles[c].getModel() == currentModel)
            {
                LOG_INFO_S << "Adding column of " << mRoles[c].toString() << " for symmetry";
                sameModelColumns << roleDistribution.col(c);
            }
        }
        symmetries << VariableSequenceSymmetry(sameModelColumns, roleDistribution.height());
    }

    LOG_INFO_S << "TIMEPOINTS";

    //// Create a timeline for a given role
    //for(int c = 0; c < roleDistribution.width(); ++c)
    //{
    //    mRoles[c].getModel()

    //}

    branch(*this, mModelUsage, Gecode::INT_VAR_SIZE_MAX(), Gecode::INT_VAL_SPLIT_MIN());
    branch(*this, mModelUsage, Gecode::INT_VAR_MIN_MIN(), Gecode::INT_VAL_SPLIT_MIN());
    branch(*this, mModelUsage, Gecode::INT_VAR_NONE(), Gecode::INT_VAL_SPLIT_MIN());

    branch(*this, mRoleUsage, Gecode::INT_VAR_SIZE_MAX(), Gecode::INT_VAL_MIN(), symmetries);
    branch(*this, mRoleUsage, Gecode::INT_VAR_MIN_MIN(), Gecode::INT_VAL_MIN(), symmetries);
    branch(*this, mRoleUsage, Gecode::INT_VAR_NONE(), Gecode::INT_VAL_MIN(), symmetries);

    // see 8.14 Executing code between branchers
    branch(*this, &TransportNetwork::postRoleAssignments);

    //Gecode::Gist::Print<TransportNetwork> p("Print solution");
    //Gecode::Gist::Options o;
    //o.inspect.click(&p);
    //Gecode::Gist::bab(this, o);

}

TransportNetwork::TransportNetwork(bool share, TransportNetwork& other)
    : Gecode::Space(share, other)
    , mpMission(other.mpMission)
    , mModelPool(other.mModelPool)
    , mAsk(other.mAsk)
    , mServices(other.mServices)
    , mIntervals(other.mIntervals)
    , mTimepoints(other.mTimepoints)
    , mLocations(other.mLocations)
    , mResourceRequirements(other.mResourceRequirements)
    , mAvailableModels(other.mAvailableModels)
    , mRoles(other.mRoles)
{
    assert( mpMission->getOrganizationModel() );
    assert(!mIntervals.empty());
    mModelUsage.update(*this, share, other.mModelUsage);
    mRoleUsage.update(*this, share, other.mRoleUsage);

    for(size_t i = 0; i < other.mTimelines.size(); ++i)
    {
        Gecode::IntVarArray array;
        mTimelines.push_back(array);

        Gecode::IntVarArray t_array;
        mTimelineGraphs.push_back(t_array);

        mTimelines[i].update(*this, share, other.mTimelines[i]);
        mTimelineGraphs[i].update(*this, share, other.mTimelineGraphs[i]);
    }
}

Gecode::Space* TransportNetwork::copy(bool share)
{
    return new TransportNetwork(share, *this);
}

std::vector<TransportNetwork::Solution> TransportNetwork::solve(const templ::Mission::Ptr& mission)
{
    SolutionList solutions;
    mission->validateForPlanning();

    assert(mission->getOrganizationModel());
    assert(!mission->getTimeIntervals().empty());

    TransportNetwork* distribution = new TransportNetwork(mission);
    {
        Gecode::BAB<TransportNetwork> searchEngine(distribution);
        //Gecode::DFS<TransportNetwork> searchEngine(this);

        TransportNetwork* best = NULL;
        while(TransportNetwork* current = searchEngine.next())
        {
            delete best;
            best = current;

            using namespace organization_model;

            LOG_INFO_S << "Solution found:" << current->toString();
            solutions.push_back(current->getSolution());
            break;
        }

        if(best == NULL)
        {
            delete distribution;
            throw std::runtime_error("templ::solvers::csp::TransportNetwork::solve: no solution found");
        }
    //    delete best;
        best = NULL;
    }

    delete distribution;
    return solutions;
}

void TransportNetwork::appendToTupleSet(Gecode::TupleSet& tupleSet, const organization_model::ModelPoolSet& combinations) const
{
    std::set< std::vector<uint32_t> > csp = toCSP( combinations );
    std::set< std::vector<uint32_t> >::const_iterator cit = csp.begin();

    for(; cit != csp.end(); ++cit)
    {
        Gecode::IntArgs args;

        const std::vector<uint32_t>& tuple = *cit;
        std::vector<uint32_t>::const_iterator tit = tuple.begin();
        for(; tit != tuple.end(); ++tit)
        {
            args << *tit;
        }
        LOG_DEBUG_S << "TupleSet: intargs: " << args;

        tupleSet.add( args );
    }
}

size_t TransportNetwork::getFluentIndex(const FluentTimeResource& fluent) const
{
    std::vector<FluentTimeResource>::const_iterator ftsIt = std::find(mResourceRequirements.begin(), mResourceRequirements.end(), fluent);
    if(ftsIt != mResourceRequirements.end())
    {
        int index = ftsIt - mResourceRequirements.begin();
        assert(index >= 0);
        return (size_t) index;
    }

    throw std::runtime_error("templ::solvers::csp::TransportNetwork::getFluentIndex: could not find fluent index for '" + fluent.toString() + "'");
}

size_t TransportNetwork::getResourceModelIndex(const owlapi::model::IRI& model) const
{
    owlapi::model::IRIList::const_iterator cit = std::find(mAvailableModels.begin(), mAvailableModels.end(), model);
    if(cit != mAvailableModels.end())
    {
        int index = cit - mAvailableModels.begin();
        assert(index >= 0);
        return (size_t) index;
    }

    throw std::runtime_error("templ::solvers::csp::TransportNetwork::getResourceModelIndex: could not find model index for '" + model.toString() + "'");
}

const owlapi::model::IRI& TransportNetwork::getResourceModelFromIndex(size_t index) const
{
    if(index < mAvailableModels.size())
    {
        return mAvailableModels.at(index);
    }
    throw std::invalid_argument("templ::solvers::csp::TransportNetwork::getResourceModelIndex: index is out of bounds");
}

size_t TransportNetwork::getResourceModelMaxCardinality(size_t index) const
{
    organization_model::ModelPool::const_iterator cit = mModelPool.find(getResourceModelFromIndex(index));
    if(cit != mModelPool.end())
    {
        return cit->second;
    }
    throw std::invalid_argument("templ::solvers::csp::TransportNetwork::getResourceModelMaxCardinality: model not found");
}

std::vector<FluentTimeResource> TransportNetwork::getResourceRequirements() const
{
    if(mIntervals.empty())
    {
        throw std::runtime_error("solvers::csp::TransportNetwork::getResourceRequirements: no time intervals available"
                " -- make sure you called prepareTimeIntervals() on the mission instance");
    }


    std::vector<FluentTimeResource> requirements;

    using namespace templ::solvers::temporal;
    point_algebra::TimePointComparator timepointComparator(mpMission->getTemporalConstraintNetwork());

    // Iterate over all existing persistence conditions
    // -- pick the ones relating to location-cardinality function
    const std::vector<PersistenceCondition::Ptr>& conditions = mpMission->getPersistenceConditions();
    std::vector<PersistenceCondition::Ptr>::const_iterator cit = conditions.begin();
    for(; cit != conditions.end(); ++cit)
    {
        PersistenceCondition::Ptr p = *cit;


        symbols::StateVariable stateVariable = p->getStateVariable();
        if(stateVariable.getFunction() != symbols::ObjectVariable::TypeTxt[symbols::ObjectVariable::LOCATION_CARDINALITY] )
        {
            continue;
        }

        owlapi::model::IRI resourceModel(stateVariable.getResource());
        symbols::ObjectVariable::Ptr objectVariable = dynamic_pointer_cast<symbols::ObjectVariable>(p->getValue());
        symbols::object_variables::LocationCardinality::Ptr locationCardinality = dynamic_pointer_cast<symbols::object_variables::LocationCardinality>(objectVariable);

        Interval interval(p->getFromTimePoint(), p->getToTimePoint(),timepointComparator);
        {
            std::vector<Interval>::const_iterator iit = std::find(mIntervals.begin(), mIntervals.end(), interval);
            if(iit == mIntervals.end())
            {
                LOG_INFO_S << "Size of intervals: " << mIntervals.size();
                throw std::runtime_error("templ::solvers::csp::TransportNetwork::getResourceRequirements: could not find interval: '" + interval.toString() + "'");
            }

            owlapi::model::IRIList::const_iterator sit = std::find(mResources.begin(), mResources.end(), resourceModel);
            if(sit == mResources.end())
            {
                throw std::runtime_error("templ::solvers::csp::TransportNetwork::getResourceRequirements: could not find service: '" + resourceModel.toString() + "'");
            }

            symbols::constants::Location::Ptr location = locationCardinality->getLocation();
            std::vector<symbols::constants::Location::Ptr>::const_iterator lit = std::find(mLocations.begin(), mLocations.end(), location);
            if(lit == mLocations.end())
            {
                throw std::runtime_error("templ::solvers::csp::TransportNetwork::getResourceRequirements: could not find location: '" + location->toString() + "'");
            }

            // Map objects to numeric indices -- the indices can be mapped
            // backed using the mission they were created from
            uint32_t timeIndex = iit - mIntervals.begin();
            FluentTimeResource ftr(mpMission,
                    (int) (sit - mResources.begin())
                    , timeIndex
                    , (int) (lit - mLocations.begin()));

            if(mAsk.ontology().isSubClassOf(resourceModel, organization_model::vocabulary::OM::Functionality()))
            {
                // retrieve upper bound
                ftr.maxCardinalities = mAsk.getFunctionalSaturationBound(resourceModel);

            } else if(mAsk.ontology().isSubClassOf(resourceModel, organization_model::vocabulary::OM::Actor()))
            {
                ftr.minCardinalities[ resourceModel ] = locationCardinality->getCardinality();
            } else {
                LOG_WARN_S << "Unsupported state variable: " << resourceModel;
                continue;
            }

            ftr.maxCardinalities = organization_model::Algebra::max(ftr.maxCardinalities, ftr.minCardinalities);
            requirements.push_back(ftr);
            LOG_DEBUG_S << ftr.toString();
        }
    }

    compact(requirements);
    return requirements;
}

void TransportNetwork::compact(std::vector<FluentTimeResource>& requirements) const
{
    LOG_DEBUG_S << "BEGIN compact requirements";
    std::vector<FluentTimeResource>::iterator it = requirements.begin();
    for(; it != requirements.end(); ++it)
    {
        FluentTimeResource& fts = *it;

        std::vector<FluentTimeResource>::iterator compareIt = it + 1;
        for(; compareIt != requirements.end();)
        {
            FluentTimeResource& otherFts = *compareIt;

            if(fts.time == otherFts.time && fts.fluent == otherFts.fluent)
            {
                LOG_DEBUG_S << "Compacting: " << std::endl
                    << fts.toString() << std::endl
                    << otherFts.toString() << std::endl;

                // Compacting the resource list
                fts.resources.insert(otherFts.resources.begin(), otherFts.resources.end());

                // Use the functional saturation bound on all functionalities
                // after compacting the resource list
                std::set<organization_model::Functionality> functionalities;
                std::set<uint32_t>::const_iterator cit = fts.resources.begin();
                for(; cit != fts.resources.end(); ++cit)
                {
                    const owlapi::model::IRI& resourceModel = mResources[*cit];
                    using namespace owlapi;

                    if( mAsk.ontology().isSubClassOf(resourceModel, organization_model::vocabulary::OM::Functionality()))
                    {
                        organization_model::Functionality functionality(resourceModel);
                        functionalities.insert(functionality);
                    }
                }
                fts.maxCardinalities = mAsk.getFunctionalSaturationBound(functionalities);

                // MaxMin --> min cardinalities are a lower bound specified explicitely
                LOG_DEBUG_S << "Update Requirements: min: " << fts.minCardinalities.toString();
                LOG_DEBUG_S << "Update Requirements: otherMin: " << otherFts.minCardinalities.toString();

                fts.minCardinalities = organization_model::Algebra::max(fts.minCardinalities, otherFts.minCardinalities);
                LOG_DEBUG_S << "Result min: " << fts.minCardinalities.toString();

                // Resource constraints might enforce a minimum cardinality that is higher than the functional saturation bound
                // thus update the max cardinalities
                fts.maxCardinalities = organization_model::Algebra::max(fts.minCardinalities, fts.maxCardinalities);

                LOG_DEBUG_S << "Update requirement: " << fts.toString();

                requirements.erase(compareIt);
            } else {
                ++compareIt;
            }
        }
    }
    LOG_DEBUG_S << "END compact requirements";
}

bool TransportNetwork::isRoleForModel(uint32_t roleIndex, uint32_t modelIndex) const
{
    return mRoles.at(roleIndex).getModel() == mAvailableModels.at(modelIndex);
}

void TransportNetwork::postRoleAssignments(Gecode::Space& home)
{
    static_cast<TransportNetwork&>(home).postRoleAssignments();
}

void TransportNetwork::postRoleAssignments()
{
    (void) status();
    LOG_WARN_S << "Role usage: " << mRoleUsage;

    // Identify active roles
    Gecode::Matrix<Gecode::IntVarArray> roleDistribution(mRoleUsage, /*width --> col*/ mRoles.size(), /*height --> row*/ mResourceRequirements.size());
    for(size_t r = 0; r < mRoles.size(); ++r)
    {
        for(size_t i = 0; i < mResourceRequirements.size(); ++i)
        {
            Gecode::IntVar var = roleDistribution(r,i);
            if(!var.assigned())
            {
                throw std::runtime_error("templ::solvers::csp::TransportNetwork::postRoleAssignments: value has not been assigned for role: '" + mRoles[r].toString() + "'");
            }
            Gecode::IntVarValues v(var);
            if(v.val() == 1)
            {
                LOG_WARN_S << "Active role: " << mRoles[r].toString();
                mActiveRoles.push_back(r);
                break;
            }
        }
    }

    // construct timelines -- add an additional transfer location
    mLocations.push_back(symbols::constants::Location::Ptr(new symbols::constants::Location("transfer")));

    size_t locationTimeSize = mLocations.size()*mTimepoints.size();
    size_t timelineIndex = 0;
    LOG_INFO_S << "LocationTimeSize: " << locationTimeSize*locationTimeSize << " -- " << mRoles.size() << " roles";
    for(uint32_t roleIndex = 0; roleIndex < mRoles.size(); ++roleIndex)
    {
        // consider only active roles
        if(mActiveRoles.end() == std::find(mActiveRoles.begin(), mActiveRoles.end(), roleIndex))
        {
            continue;
        }

        Gecode::IntVarArray timeline(*this, locationTimeSize*locationTimeSize,0,1); // Domain is 0 or 1 to represent activation
        mTimelines.push_back(timeline);
        Gecode::Matrix<Gecode::IntVarArray> roleTimeline(mTimelines[timelineIndex], locationTimeSize, locationTimeSize);
        ++timelineIndex;

        for(uint32_t requirementIndex = 0; requirementIndex < mResourceRequirements.size(); ++requirementIndex)
        {
            Gecode::IntVar roleRequirement = roleDistribution(roleIndex, requirementIndex);

            // maps to the interval
            {
                const FluentTimeResource& fts = mResourceRequirements[requirementIndex];
                // index of the location is: fts.fluent
                //
                using namespace solvers::temporal;
                point_algebra::TimePoint::Ptr from = fts.getInterval().getFrom();
                point_algebra::TimePoint::Ptr to = fts.getInterval().getTo();
                std::vector<point_algebra::TimePoint::Ptr>::const_iterator fromIt = std::find(mTimepoints.begin(), mTimepoints.end(), from);
                std::vector<point_algebra::TimePoint::Ptr>::const_iterator toIt = std::find(mTimepoints.begin(), mTimepoints.end(), to);

                uint32_t fromIndex = fromIt - mTimepoints.begin();
                uint32_t toIndex = toIt - mTimepoints.begin();
                for(uint32_t timeIndex = fromIndex; timeIndex < toIndex; ++timeIndex)
                {
                    // index of the location is fts.fluent
                    // edge index:
                    // row = |timepoints|*location + timepoint-from
                    // col = |timepoints|*location + timepoint-to
                    size_t row = timeIndex*mLocations.size() + fts.fluent;
                    // Always connect to the next timestep
                    size_t col = (timeIndex + 1)*mLocations.size() + fts.fluent;
                    Gecode::IntVar edgeActivation = roleTimeline(col, row);

                    // constraint between roleTimeline and roleRequirement
                    // TODO: check if bool >= int does actually work
                    rel(*this, edgeActivation >= roleRequirement);
                }
            }
        }

        // maximum one outgoing edge per node only
        for(size_t index = 0; index < locationTimeSize; ++index)
        {
            rel(*this, sum( roleTimeline.col(index) ) <= 1);
            rel(*this, sum( roleTimeline.row(index) ) <= 1);
        }

        for(size_t t = 0; t < mTimepoints.size()-1; ++t)
        {
            Gecode::IntVarArgs args;
            size_t baseIndex = t*mLocations.size();
            // maxmimum one outgoing edge for same time nodes only
            for(size_t l = 0; l < mLocations.size(); ++l)
            {
                args << roleTimeline.row(baseIndex + l);
            }
            rel(*this, sum(args) <= 1);

            // forward in time only
            //        t0-l0 ... t0-l2   t1-l1 ...
            // t0-l0    x        x       ok
            // t0-l1    x        x       ok
            // t0-l2    x        x       ok
            // t1-l1    x        x       x        x
            // t1-l2    x        x       x        x
            //
            rel(*this, sum( roleTimeline.slice(
                            0, t*mLocations.size() + mLocations.size() -1,
                            t*mLocations.size(), t*mLocations.size()) ) == 0);

        }
    }
    // Construct the basic timeline
    //
    // Map role requirements back to activation in general network
    // requirement = location t0--tN, role-0, role-1
    //
    // foreach involved role
    //     foreach requirement
    //          from lX,t0 --> tN
    //              request edge activation (referring to the role is active during that interval)
    //              by >= value of the requirement( which is typically 0 or 1),
    //              whereas activation can be 0 or 1 as well
    //
    // Compute a network with proper activation

    branch(*this, &TransportNetwork::postRoleTimelines);

    for(size_t i = 0; i < mActiveRoles.size(); ++i)
    {
        branch(*this, mTimelines[i], Gecode::INT_VAR_SIZE_MAX(), Gecode::INT_VAL_SPLIT_MIN());
        branch(*this, mTimelines[i], Gecode::INT_VAR_MIN_MIN(), Gecode::INT_VAL_SPLIT_MIN());
        branch(*this, mTimelines[i], Gecode::INT_VAR_NONE(), Gecode::INT_VAL_SPLIT_MIN());
    }
}

void TransportNetwork::postRoleTimelines(Gecode::Space& home)
{
    static_cast<TransportNetwork&>(home).postRoleTimelines();
}

void TransportNetwork::postRoleTimelines()
{
    (void) status();

    // Check for each timeline -- which is associated with a role
    for(size_t i = 0;  i < mTimelines.size(); ++i)
    {
        size_t locationTimeSize = mLocations.size()*mTimepoints.size();
        LOG_DEBUG_S << "locations" << mLocations.size() << " timepoints " << mTimepoints.size();
        Gecode::Matrix<Gecode::IntVarArray> timeline(mTimelines[i], locationTimeSize, locationTimeSize);

        // compute the adjacency list from the existing matrix
        // identify the starting point and end point
        std::vector< std::pair<size_t,size_t> > path;
        Gecode::IntVarArgs adjacencyList;
        for(size_t row = 0; row < locationTimeSize; ++row)
        {
            std::vector<int> neighbours;
            for(size_t col = 0; col < locationTimeSize; ++col)
            {
                Gecode::IntVar var = timeline(col, row);
                if(var.assigned())
                {
                    Gecode::IntVarValues v(var);
                    if(v.val() == 1)
                    {
                        path.push_back( std::pair<size_t,size_t>(col,row) );
                        break; // break the loop since there can be only one connection, so goto the next row
                    }
                }
            }
        }

        // For each element in the path except for end and start there needs to
        // be a connection in the range of the two given edges
        if(path.size() >= 2)
        {
            if(path.size() > 2)
            {
                size_t startCol = path.front().first;
                size_t startRow = path.front().second - path.front().second%mLocations.size();
                size_t endCol = path.back().first;
                size_t endRow = path.back().second - path.back().second%mLocations.size();

                for(size_t rowIndex = startRow; rowIndex <= endRow; ++rowIndex)
                {
                    // requires a
                    // already specified rel(*this, sum( timeline.row(rowIndex)) <= 1);
                    if(rowIndex > startRow + mLocations.size())
                    {
                        // edges need to form a path
                        //rel(*this, sum( timeline.row(rowIndex)) == sum (timeline.col(rowIndex)) );
                    }

                    if(rowIndex != endRow)
                    {
                        // for each timestep there needs to be a path segment
                        if(rowIndex%mLocations.size() == 0)
                        {
                            // fc,tc, fr, tr: from column to column, from row to
                            // row
                            Gecode::IntVarArgs args;
                            args << timeline.slice(rowIndex+mLocations.size(), rowIndex + 2*mLocations.size() -1,
                                    rowIndex, rowIndex + mLocations.size() -1);
                            rel(*this, sum(args) == 1);
                        }
                    }
                }
            }

            for(size_t nodeIndex = 0; nodeIndex < path.size(); ++nodeIndex)
            {
                size_t col = path[nodeIndex].first;
                size_t row = path[nodeIndex].second;
                // last affected row that is temporally concurrent with the
                // current row
                size_t startRow = row - row%mLocations.size();
                size_t endRow = startRow + mLocations.size() -1;

                if(nodeIndex == 0)
                {
                    // No connection before this time-location
                    Gecode::IntVarArgs args;
                    for(size_t rowIndex = 0; rowIndex <= endRow; ++rowIndex)
                    {
                       args << timeline.row(rowIndex);
                    }
                    rel(*this, sum(args) == 1);

                } else {
                    // Require the node 'row' to be connected to 'col'
                    rel(*this, sum( timeline.col(row) ) == 1);
                }

                if(nodeIndex == path.size() -1)
                {
                    // No connection after this time-location
                    Gecode::IntVarArgs args;
                    for(size_t rowIndex = startRow; rowIndex < locationTimeSize; ++rowIndex)
                    {
                       args << timeline.row(rowIndex);
                    }
                    // no edge should leave from anywhere, since this is the
                    // last
                    rel(*this, sum(args) == 1);
                } else
                {
                    rel(*this, sum( timeline.row(col) ) == 1);
                }
            }
        }
    }

}

size_t TransportNetwork::getMaxResourceCount(const organization_model::ModelPool& pool) const
{
    size_t maxValue = std::numeric_limits<size_t>::min();
    organization_model::ModelPool::const_iterator cit = pool.begin();
    for(; cit != pool.end(); ++cit)
    {
        maxValue = std::max( maxValue, cit->second);
    }
    return maxValue;
}

std::string TransportNetwork::toString() const
{
    std::stringstream ss;
    ss << "TransportNetwork: #" << std::endl;
    Gecode::Matrix<Gecode::IntVarArray> resourceDistribution(mModelUsage, mModelPool.size(), mResourceRequirements.size());

    //// Check if resource requirements holds
    //for(size_t i = 0; i < mResourceRequirements.size(); ++i)
    //{
    //    ss << "assignment for:"
    //        << mResourceRequirements[i].toString()
    //        << " available resources: " << mMission.getAvailableResources().size()
    //        << std::endl;

    //    for(size_t mi = 0; mi < mMission.getAvailableResources().size(); ++mi)
    //    {
    //        Gecode::IntVar var = resourceDistribution(mi, i);
    //        var.assigned();
    //        Gecode::IntVarValues v( var );
    //        ss << "    "
    //            << mAvailableModels[mi].toString()
    //            << " : "
    //            << "assigned #" << v.val()
    //            << std::endl;
    //        ss << "Index: " << mi << "/" << i << " --> " << v.val() << std::endl;
    //    }
    //}
    // ss << "Array: " << resourceDistribution;
    ss << "Current model usage: " << mModelUsage << std::endl;
    ss << "Current model usage: " << resourceDistribution << std::endl;
    ss << "Current role usage: " << mRoleUsage << std::endl;
    ss << "Current timelines:" << std::endl;
    for(size_t i = 0; i < mTimelines.size(); ++i)
    {
        ss << "    #"<< i << ": " << mTimelines[i] << std::endl;
        std::string path;

        size_t locationTimeSize = mLocations.size()*mTimepoints.size();
        Gecode::Matrix<Gecode::IntVarArray> timeline(mTimelines[i], locationTimeSize, locationTimeSize);
        for(size_t row = 0; row < locationTimeSize; ++row)
        {
            size_t timeIndex = (row - row%mLocations.size())/mLocations.size();
            size_t locationIndex = row%mLocations.size();
            std::string label = "" + mTimepoints[timeIndex]->toString() + "-" + mLocations[locationIndex]->toString();
            ss << "#" << row << " " << std::setw(65) << label << " ";
            for(size_t col = 0; col < locationTimeSize; ++col)
            {
                Gecode::IntVar var = timeline(col,row);
                ss <<  var << " ";
                if(var.assigned())
                {
                    Gecode::IntVarValues v(var);
                    if(v.val() == 1)
                    {
                        path += "-->" + label;
                    }
                }
            }
            ss << std::endl;
        }
        ss << "    " << path << std::endl;
    }
    return ss.str();
}

std::ostream& operator<<(std::ostream& os, const TransportNetwork::Solution& solution)
{
    os << solution.toString();
    return os;
}

std::ostream& operator<<(std::ostream& os, const TransportNetwork::SolutionList& solutions)
{
    TransportNetwork::SolutionList::const_iterator cit = solutions.begin();
    os << std::endl << "BEGIN SolutionList (#" << solutions.size() << " solutions)" << std::endl;
    size_t count = 0;
    for(; cit != solutions.end(); ++cit)
    {
        os << "#" << count++ << " ";
        os << *cit;
    }
    os << "END SolutionList" << std::endl;
    return os;
}

void TransportNetwork::addFunctionRequirement(const FluentTimeResource& fts, owlapi::model::IRI& function)
{
    size_t index = 0;
    owlapi::model::IRIList::const_iterator cit = mResources.begin();
    for(; cit != mResources.end(); ++cit, ++index)
    {
        if(*cit == function)
        {
            break;
        }
    }
    if(index >= mResources.size())
    {
        if( mAsk.ontology().isSubClassOf(function, organization_model::vocabulary::OM::Functionality()) )
        {
            LOG_INFO_S << "AUTO ADDED '" << function << "' to required resources";
            mResources.push_back(function);
        } else {
            throw std::invalid_argument("templ::solvers::csp::TransportNetwork: could not find the resource index for: '" + function.toString() + "' -- which is not a service class");
        }
    }
    LOG_DEBUG_S << "Using index: " << index;

    std::vector<FluentTimeResource>::iterator fit = std::find(mResourceRequirements.begin(), mResourceRequirements.end(), fts);
    if(fit == mResourceRequirements.end())
    {
        throw std::invalid_argument("templ::solvers::csp::TransportNetwork: could not find the fluent time resource: '" + fts.toString() + "'");
    }
    LOG_DEBUG_S << "Fluent before adding function requirement: " << fit->toString();
    // insert the resource requirement
    fit->resources.insert(index);
    fit->maxCardinalities = organization_model::Algebra::max(fit->maxCardinalities, mAsk.getFunctionalSaturationBound(function) );
    LOG_DEBUG_S << "Fluent after adding function requirement: " << fit->toString();
}

} // end namespace csp
} // end namespace solvers
} // end namespace templ
