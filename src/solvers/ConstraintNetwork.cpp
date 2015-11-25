#include "ConstraintNetwork.hpp"

using namespace graph_analysis;

namespace templ {
namespace solvers {

ConstraintNetwork::ConstraintNetwork()
    : mGraph(BaseGraph::getInstance(BaseGraph::BOOST_DIRECTED_GRAPH))
{}

void ConstraintNetwork::addVariable(Variable::Ptr variable)
{
    mGraph->addVertex(variable);
}

void ConstraintNetwork::addConstraint(Constraint::Ptr constraint)
{
    mGraph->addEdge(constraint);
}

void ConstraintNetwork::removeConstraint(Constraint::Ptr constraint)
{
    mGraph->removeEdge(constraint);
}

graph_analysis::VertexIterator::Ptr ConstraintNetwork::getVariableIterator() const
{
    return mGraph->getVertexIterator();
}

graph_analysis::EdgeIterator::Ptr ConstraintNetwork::getConstraintIterator() const
{
    return mGraph->getEdgeIterator();
}

} // end namespace solvers
} // end namespace templ
