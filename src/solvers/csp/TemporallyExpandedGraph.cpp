#include <templ/solvers/csp/TemporallyExpandedGraph.hpp>
#include <gecode/int.hh>
#include <gecode/int/rel.hh>
#include <gecode/minimodel.hh>
#include <base/Logging.hpp>

using namespace Gecode;

namespace templ {
namespace solvers {
namespace csp {

void isPath(Gecode::Space& home, const Gecode::IntVarArgs& x, uint32_t numberOfTimepoints, uint32_t numberOfFluents)
{
    ViewArray<Int::IntView> xv(home, x);
    LOG_WARN_S << "POST: " << numberOfTimepoints << " and " << numberOfFluents;
    if(IsPath::post(home, xv, numberOfTimepoints, numberOfFluents) != ES_OK)
    {
        home.fail();
    }

    if(home.failed())
    {
        return;
    }
}

IsPath::IsPath(Gecode::Space& home, ViewArray<Int::IntView>& xv, uint32_t numberOfTimepoints, uint32_t numberOfFluents)
    : NaryPropagator<Int::IntView, Int::PC_INT_BND>(home, xv)
    , mNumberOfTimepoints(numberOfTimepoints)
    , mNumberOfFluents(numberOfFluents)
    , mNumberOfVertices(numberOfTimepoints*numberOfFluents)
    , mGraph(xv)
{
}

IsPath::IsPath(Gecode::Space& home, bool share, IsPath& p)
    : NaryPropagator<Int::IntView, Int::PC_INT_BND>(home, share, p)
{
    mGraph.update(home, share, p.mGraph);
}

Gecode::ExecStatus IsPath::post(Gecode::Space& home, ViewArray<Int::IntView>& xv, uint32_t numberOfTimepoints, uint32_t numberOfFluents)
{
    Gecode::IntConLevel intConLevel = Gecode::ICL_DEF;

    uint32_t numberOfVertices = numberOfTimepoints*numberOfFluents;
    std::vector<Gecode::LinIntExpr> cols;
    std::vector<Gecode::LinIntExpr> rows;
    for(uint32_t i = 0; i < numberOfVertices; ++i)
    {
        Gecode::LinIntExpr expr = 0;
        cols.push_back(expr);
        rows.push_back(expr);
    }

    // compute sums of columns and rows
    //
    for(uint32_t col = 0; col < numberOfVertices; ++col)
    {
        for(uint32_t row = 0; row < numberOfVertices; ++row)
        {
            {
                Gecode::Int::IntView& elementView = xv[row*numberOfVertices + col];
                {
                    Gecode::LinIntExpr& expr = rows.at(row);
                    expr = expr + elementView;
                }
                {
                    Gecode::LinIntExpr& expr = cols.at(col);
                    expr = expr + elementView;
                }

                //############################
                //    forward in time only
                //###########################
                //        t0-l0 ... t0-l2   t1-l1 ...
                // t0-l0    x        x       ok
                // t0-l1    x        x       ok
                // t0-l2    x        x       ok
                // t1-l1    x        x       x        x
                // t1-l2    x        x       x        x
                uint32_t timepointTarget = col/numberOfFluents;
                uint32_t timepointSource = row/numberOfFluents;
                if(timepointTarget <= timepointSource)
                {
                    Gecode::LinIntExpr expr = 0 + elementView;
                    Gecode::LinIntRel r(expr,Gecode::IRT_EQ,0);
                    r.post(home, true, intConLevel);
                }
            }
        }
    }

    // Sum of column or row should be 1 to form a path
    for(uint32_t index = 0; index < numberOfVertices; ++index)
    {
        {
            Gecode::LinIntRel r(cols[index],Gecode::IRT_LQ,1);
            r.post(home,true, intConLevel);
        }
        {
            // alternative: r = (rows[index] <= 1);
            Gecode::LinIntRel r(rows[index], Gecode::IRT_LQ,1);
            r.post(home,true, intConLevel);
        }
        //  one outgoing edge for the same time nodes only
        //  sum of rows <= 1
        if(index%numberOfFluents == 0)
        {
            Gecode::LinIntExpr expr = 0;
            for(size_t offset = 0; offset < numberOfFluents; ++offset)
            {
                expr = expr + rows[index + offset];
            }
            Gecode::LinIntRel r(expr, Gecode::IRT_LQ, 1);
            r.post(home, true, intConLevel);
        }
    }

    // documentation. 4.3.1 Post functions are clever
    // A constraint post function carefully analyzes its arguments. Based on
    // this analysis, the constraint post function chooses the best possible propagator for the constraint
    (void) new (home) IsPath(home, xv, numberOfTimepoints, numberOfFluents);
    return ES_OK;
}

size_t IsPath::dispose(Gecode::Space& home)
{
    mGraph.cancel(home, *this, Int::PC_INT_DOM);
    (void) Propagator::dispose(home);
    return sizeof(*this);
}

Gecode::Propagator* IsPath::copy(Gecode::Space& home, bool share)
{
    return new (home) IsPath(home, share, *this);
}

Gecode::PropCost IsPath::cost(const Gecode::Space&, const Gecode::ModEventDelta&) const
{
    return Gecode::PropCost::quadratic(PropCost::LO, mGraph.size());
}


Gecode::ExecStatus IsPath::propagate(Gecode::Space& home, const Gecode::ModEventDelta&)
{
    if(mGraph.assigned())
    {
        return home.ES_SUBSUMED(*this);
    } else {
        // the propagator will be scheduled if one of its views have been modified
        return ES_NOFIX;
    }
}

} // end namespace csp
} // end namespace solvers
} // end namespace templ

