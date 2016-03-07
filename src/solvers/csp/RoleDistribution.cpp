#include "RoleDistribution.hpp"

#include <gecode/minimodel.hh>
#include <gecode/set.hh>
#include <gecode/search.hh>
#include <base/Logging.hpp>
#include <numeric/Combinatorics.hpp>

namespace templ {
namespace solvers {
namespace csp {

RoleDistribution::RoleDistribution(const Mission& mission, const ModelDistribution::Solution& modelDistribution)
    : Gecode::Space()
    , mRoleUsage(*this, /*width --> col */ mission.getRoles().size()* /*height --> row*/ modelDistribution.size(), 0, 1) // Domain 0,1 to represent activation
    , mRoles(mission.getRoles())
    , mIntervals(mission.getTimeIntervals().begin(), mission.getTimeIntervals().end())
    , mAvailableModels(mission.getModels())
{
    Gecode::Matrix<Gecode::IntVarArray> roleDistribution(mRoleUsage, /*width --> col*/ mRoles.size(), /*height --> row*/ modelDistribution.size());

    const owlapi::model::IRIList& models = mission.getModels();
    const organization_model::ModelPool& resources = mission.getAvailableResources();

    // foreach FluentTimeResource
    //     same role types -> sum <= modelbound given by solution
    ModelDistribution::Solution::const_iterator cit = modelDistribution.begin();
    size_t column = 0;
    for(; cit != modelDistribution.end(); ++cit, ++column)
    {
        const FluentTimeResource& fts = cit->first;
        const organization_model::ModelPool& solutionPool = cit->second;

        // build initial requirement list
        mRequirements.push_back(fts);

        // Set limits per model type
        size_t index = 0;
        for(size_t i = 0; i < models.size(); ++i)
        {
            Gecode::IntVarArgs args;
            const owlapi::model::IRI& currentModel = models[i];

            organization_model::ModelPool::const_iterator mit = resources.find(currentModel);
            if(mit == resources.end())
            {
                throw std::runtime_error("templ::solvers::csp::RoleDistribution "
                        " could not find model: '" + currentModel.toString() + "'");
            }
            size_t modelBound = mit->second;
            for(size_t t = 0; t < modelBound; ++t)
            {
                Gecode::IntVar v = roleDistribution(index, column);
                args << v;

                ++index;
            }

            mit = solutionPool.find(currentModel);
            if(mit == resources.end())
            {
                throw std::runtime_error("templ::solvers::csp::RoleDistribution "
                        " could not find model: '" + currentModel.toString() + "'");
            }
            size_t solutionModelBound = mit->second;

            rel(*this, sum(args) == solutionModelBound);
        }
    }

    assert(mRequirements.size() == modelDistribution.size());

    {
        // Set of available models: mModelPool
        // Make sure the assignments are within resource bounds for concurrent requirements
        std::vector< std::vector<FluentTimeResource> > concurrentRequirements = FluentTimeResource::getConcurrent(mRequirements, mIntervals);

        std::vector< std::vector<FluentTimeResource> >::const_iterator cit = concurrentRequirements.begin();
        if(concurrentRequirements.empty())
        {
            LOG_WARN_S << "No concurrent requirements found";
        } else {
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
        }
    }

    branch(*this, mRoleUsage, Gecode::INT_VAR_SIZE_MAX(), Gecode::INT_VAL_SPLIT_MIN());
    branch(*this, mRoleUsage, Gecode::INT_VAR_MIN_MIN(), Gecode::INT_VAL_SPLIT_MIN());
    branch(*this, mRoleUsage, Gecode::INT_VAR_NONE(), Gecode::INT_VAL_SPLIT_MIN());
}

RoleDistribution::RoleDistribution(bool share, RoleDistribution& other)
    : Gecode::Space(share, other)
    , mRoles(other.mRoles)
    , mRequirements(other.mRequirements)
    , mIntervals(other.mIntervals)
{
    mRoleUsage.update(*this, share, other.mRoleUsage);

}

Gecode::Space* RoleDistribution::copy(bool share)
{
    return new RoleDistribution(share, *this);
}

size_t RoleDistribution::getFluentIndex(const FluentTimeResource& fluent) const
{
    std::vector<FluentTimeResource>::const_iterator ftsIt = std::find(mRequirements.begin(), mRequirements.end(), fluent);
    if(ftsIt != mRequirements.end())
    {
        int index = ftsIt - mRequirements.begin();
        assert(index >= 0);
        return (size_t) index;
    }

    throw std::runtime_error("templ::solvers::csp::RoleDistribution::getFluentIndex: could not find fluent index for '" + fluent.toString() + "'");
}

RoleDistribution::SolutionList RoleDistribution::solve(const Mission& _mission, const ModelDistribution::Solution& modelDistribution)
{
    SolutionList solutions;

    Mission mission = _mission;
    mission.prepare();

    RoleDistribution* distribution = new RoleDistribution(mission, modelDistribution);
    RoleDistribution* solvedDistribution = NULL;
    {
        Gecode::BAB<RoleDistribution> searchEngine(distribution);
        //Gecode::DFS<ModelDistribution> searchEngine(this);

        RoleDistribution* best = NULL;
        while(RoleDistribution* current = searchEngine.next())
        {
            delete best;
            best = current;

            using namespace organization_model;

            LOG_INFO_S << "Solution found:" << current->toString();
            solutions.push_back(current->getSolution());
        }

        if(best == NULL)
        {
            throw std::runtime_error("templ::solvers::csp::ModelDistribution::solve: no solution found");
        }
    }
    delete solvedDistribution;
    solvedDistribution = NULL;

    return solutions;
}

RoleDistribution* RoleDistribution::nextSolution()
{
    Gecode::BAB<RoleDistribution> searchEngine(this);
    //Gecode::DFS<RoleDistribution> searchEngine(this);

    RoleDistribution* current = searchEngine.next();
    if(current == NULL)
    {
        throw std::runtime_error("templ::solvers::csp::RoleDistribution::solve: no solution found");
    } else {
        return current;
    }
}

RoleDistribution::Solution RoleDistribution::getSolution() const
{
    Solution solution;

    Gecode::Matrix<Gecode::IntVarArray> roleDistribution(mRoleUsage, /*width --> col*/ mRoles.size(), /*height --> row*/ mRequirements.size());

    // Check if resource requirements holds
    for(size_t i = 0; i < mRequirements.size(); ++i)
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

        solution[ mRequirements[i] ] = roles;
    }

    return solution;
}


std::string RoleDistribution::toString() const
{
    std::stringstream ss;
    ss << "ModelDistribution: #" << std::endl;
    ss << "    role usage: " << mRoleUsage;
    return ss.str();
}

std::ostream& operator<<(std::ostream& os, const RoleDistribution::Solution& solution)
{
    RoleDistribution::Solution::const_iterator cit = solution.begin();
    size_t count = 0;
    os << "Solution" << std::endl;
    for(; cit != solution.end(); ++cit)
    {
        const FluentTimeResource& fts = cit->first;
        os << "--- requirement #" << count++ << std::endl;
        os << fts.toString() << std::endl;

        const Role::List& roles = cit->second;
        os << Role::toString(roles) << std::endl;
    }
    return os;
}

std::ostream& operator<<(std::ostream& os, const RoleDistribution::SolutionList& solutions)
{
    RoleDistribution::SolutionList::const_iterator cit = solutions.begin();
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

void RoleDistribution::allDistinct(const FluentTimeResource& fts0, const FluentTimeResource& fts1, const owlapi::model::IRI& roleModel)
{
    Gecode::Matrix<Gecode::IntVarArray> roleDistribution(mRoleUsage, /*width --> col*/ mRoles.size(), /*height --> row*/ mRequirements.size());

    for(size_t roleIndex = 0; roleIndex < mRoles.size(); ++roleIndex)
    {
        const Role& role = mRoles[roleIndex];
        if(role.getModel() == roleModel);
        {
            Gecode::IntVarArgs args;
            {
                size_t fluent = getFluentIndex(fts0);
                Gecode::IntVar v = roleDistribution(roleIndex, fluent);
                args << v;
            }
            {
                size_t fluent = getFluentIndex(fts1);
                Gecode::IntVar v = roleDistribution(roleIndex, fluent);
                args << v;
            }

            rel(*this, sum(args) <= 1);
        }
    }
}

void RoleDistribution::minDistinct(const FluentTimeResource& fts0, const FluentTimeResource& fts1, const owlapi::model::IRI& roleModel, uint32_t minDistinctRoles)
{
    std::vector<size_t> indices;
    for(size_t roleIndex = 0; roleIndex < mRoles.size(); ++roleIndex)
    {
        const Role& role = mRoles[roleIndex];
        if(role.getModel() == roleModel)
        {
            indices.push_back(roleIndex);
        }
    }

    Gecode::Matrix<Gecode::IntVarArray> roleDistribution(mRoleUsage, /*width --> col*/ mRoles.size(), /*height --> row*/ mRequirements.size());
     Gecode::IntVarArgs args;
     for(size_t m = 0; m < indices.size(); ++m)
     {
         size_t roleIndex = indices[m];
         size_t fluent0 = getFluentIndex(fts0);
         Gecode::IntVar v0 = roleDistribution(roleIndex, fluent0);

         size_t fluent1 = getFluentIndex(fts1);
         Gecode::IntVar v1 = roleDistribution(roleIndex, fluent1);

         // Check if a role is part of the fulfillment of both requirements
         // if so -- sum equals to 0 thus there is no distinction
         Gecode::IntVar rolePresentInBoth = Gecode::expr(*this, abs(v0 - v1));
         args << rolePresentInBoth;
     }
     rel(*this, sum(args) >= minDistinctRoles);
}

void RoleDistribution::addDistinct(const FluentTimeResource& fts0, const FluentTimeResource& fts1, const owlapi::model::IRI& roleModel, uint32_t additional, const Solution& solution)
{
    Gecode::Matrix<Gecode::IntVarArray> roleDistribution(mRoleUsage, /*width --> col*/ mRoles.size(), /*height --> row*/ mRequirements.size());

    // Adding this constraint will only work to an already once solved instance
    // of the problem
    std::set<Role> uniqueRoles;
    {
        Solution::const_iterator sit = solution.find(fts0);
        if(sit == solution.end())
        {
            throw std::runtime_error("templ::solvers::csp::RoleDistribution: the given fluent-time-resource is not part of the solution: "
                    + fts0.toString());
        }

        const Role::List& roles = sit->second;
        Role::List::const_iterator rit = roles.begin();
        for(; rit != roles.end(); ++rit)
        {
            const Role& role = *rit;
            if(role.getModel() == roleModel)
            {
                uniqueRoles.insert(role);
            }
        }
    }
    {
        Solution::const_iterator sit = solution.find(fts1);
        if(sit == solution.end())
        {
            throw std::runtime_error("templ::solvers::csp::RoleDistribution: the given fluent-time-resource is not part of the solution"
                    + fts1.toString());
        }

        const Role::List& roles = sit->second;
        Role::List::const_iterator rit = roles.begin();
        for(; rit != roles.end(); ++rit)
        {
            const Role& role = *rit;
            if(role.getModel() == roleModel)
            {
                uniqueRoles.insert(role);
            }
        }
    }
    size_t numberOfUniqueRoles = uniqueRoles.size();
    LOG_INFO_S << "Previous number of unique roles: " << numberOfUniqueRoles << " -- should be increased with " << additional;
    RoleDistribution::minDistinct(fts0, fts1, roleModel, numberOfUniqueRoles + additional);
}

} // end namespace csp
} // end namespace solvers
} // end namespace templ
