#include <templ/solvers/Solution.hpp>
#include <templ/Mission.hpp>
#include <templ/io/MissionWriter.hpp>
#include <moreorg/OrganizationModel.hpp>
#include <boost/test/unit_test.hpp>
#include <moreorg/vocabularies/VRP.hpp>

using namespace templ;
using namespace moreorg;
namespace pa = templ::solvers::temporal::point_algebra;
namespace sym = templ::symbols::constants;

struct SolutionFixture
{
    SolutionFixture()
    {}

    ~SolutionFixture()
    {}

    void prepareSolution(const moreorg::OrganizationModel::Ptr& om)
    {
        for(size_t i = 0; i < 10; ++i)
        {
            {
                std::stringstream ss;
                ss << "t" << i;
                timepoints.push_back( pa::TimePoint::create(ss.str()) );
            }
            {
                std::stringstream ss;
                ss << "l" << i;
                locations.push_back( sym::Location::create(ss.str()) );
            }
        }

        network = SpaceTime::Network(locations, timepoints);
        solution = solvers::Solution(network, om);
    }

    solvers::Solution solution;
    SpaceTime::Network network;

    pa::TimePoint::PtrList timepoints;
    sym::Location::PtrList locations;
};

BOOST_AUTO_TEST_SUITE(solution)

BOOST_FIXTURE_TEST_CASE(should_add_role, SolutionFixture)
{
    owlapi::model::IRI organizationModelIRI =
    "http://www.rock-robotics.org/2017/11/vrp";
    moreorg::OrganizationModel::Ptr om = moreorg::OrganizationModel::getInstance(organizationModelIRI);
    owlapi::model::IRI vehicle = vocabulary::VRP::resolve("Vehicle");
    owlapi::model::IRI commodity = vocabulary::VRP::resolve("Commodity");

    prepareSolution(om);
    Role vehicle0(0, vehicle);
    Role commodity0(0, commodity);

    {
        {
            SpaceTime::RoleInfoSpaceTimeTuple::Ptr tuple = solution.addRole( vehicle0,
                    timepoints[0], timepoints[3], locations[0], RoleInfo::ASSIGNED);
            BOOST_REQUIRE_MESSAGE(tuple->hasRole(vehicle0, RoleInfo::ASSIGNED), "Tuple has role added");
        }


        {
            SpaceTime::RoleInfoSpaceTimeTuple::Ptr tuple = solution.addRole( commodity0,
                    timepoints[0], timepoints[5], locations[0], RoleInfo::ASSIGNED);
            BOOST_REQUIRE_MESSAGE(tuple->hasRole(commodity0, RoleInfo::ASSIGNED), "Tuple has role added");
        }


        solution.save("/tmp/test-templ-solution-should_add_role-added_role.gexf");

        Mission::Ptr mission = solution.toMission(om, "test-mission");
        io::MissionWriter::write("/tmp/test-templ-solution-should_add_role-mission.xml", *mission.get());


        SpaceTime::RoleInfoSpaceTimeTuple::PtrList path = solution.getPath(commodity0);
        BOOST_REQUIRE_MESSAGE(path.size() == 5, "Path expected to have length 5");
        for(const SpaceTime::RoleInfoSpaceTimeTuple::Ptr& tuple : path)
        {
            symbols::constants::Location::Ptr location = tuple->first();
            BOOST_REQUIRE_MESSAGE(location == locations[0], "Location in path:"
                    "expected " << locations[0]->toString() << ", but was " <<
                    location->toString());
        }
    }

    {
        SpaceTime::RoleInfoSpaceTimeTuple::Ptr tuple = solution.removeRole( vehicle0,
                timepoints[0], locations[0]);

        BOOST_REQUIRE_MESSAGE(!tuple->hasRole(vehicle0, RoleInfo::ASSIGNED), "Tuple has no role");
        solution.save("/tmp/test-templ-solution-should_add_role-removed_role.gexf");
    }

}

BOOST_AUTO_TEST_SUITE_END()
