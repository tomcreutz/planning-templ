#ifndef TEMPL_SOLVERS_TRANSSHIPMENT_TRANSPORT_NETWORK_HPP
#define TEMPL_SOLVERS_TRANSSHIPMENT_TRANSPORT_NETWORK_HPP

#include <templ/Mission.hpp>
#include <templ/SpaceTimeNetwork.hpp>
#include <templ/solvers/csp/RoleTimeline.hpp>

namespace templ {
namespace solvers {
namespace transshipment {

class TransportNetwork
{
public:
    TransportNetwork(const Mission& mission,
        const std::map<Role, csp::RoleTimeline>& timelines);

    const Mission& getMission() const { return mMission; }
    SpaceTimeNetwork& getSpaceTimeNetwork() { return mSpaceTimeNetwork; }
    const std::map<Role, csp::RoleTimeline> getTimeslines() const { return mTimelines; }

protected:
    void initialize();

    void save();

private:
    Mission mMission;
    SpaceTimeNetwork mSpaceTimeNetwork;
    std::map<Role, csp::RoleTimeline> mTimelines;

};


} // end namespace transsshipment
} // end namespace solvers
} // end namespace templ
#endif // TEMPL_SOLVERS_TRANSSHIPMENT_TRANSPORT_NETWORK_HPP