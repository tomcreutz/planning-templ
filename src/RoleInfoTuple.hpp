#ifndef TEMPL_ROLE_INFO_TUPLE_HPP
#define TEMPL_ROLE_INFO_TUPLE_HPP

#include <set>
#include <templ/Role.hpp>
#include <templ/Tuple.hpp>

namespace templ {

class RoleInfo
{
public:
    typedef shared_ptr<RoleInfo> Ptr;

    void addRole(const Role& role, const std::string& tag = "");

    bool hasRole(const Role& role, const std::string& tag = "") const;

    const std::set<Role>& getRoles(const std::string& tag ="") const;

    std::string toString(uint32_t indent = 0) const
    {
        std::stringstream ss;
        std::string hspace(indent,' ');
        ss << hspace << "    roles:" << std::endl;
        {
            std::set<Role>::const_iterator it = mRoles.begin();
            for(; it != mRoles.end(); ++it)
            {
                ss << hspace << "        " << it->toString() << std::endl;
            }
        }

        std::map<std::string, std::set<Role> >::const_iterator rit = mTaggedRoles.begin();
        for(; rit != mTaggedRoles.end(); ++rit)
        {
            ss << hspace << "    roles (" << rit->first << "):" << std::endl;
            const std::set<Role>& roles = rit->second;
            std::set<Role>::const_iterator it = roles.begin();
            for(; it != roles.end(); ++it)
            {
                ss << hspace << "        " << it->toString() << std::endl;
            }
        }

        return ss.str();
    }

private:
    std::set<Role> mRoles;
    mutable std::map<std::string, std::set<Role> > mTaggedRoles;
};

template<typename A, typename B>
class RoleInfoTuple : public Tuple<A, B>, public RoleInfo
{
    typedef Tuple<A,B> BaseClass;

public:
    typedef shared_ptr< RoleInfoTuple<A,B> > Ptr;

    RoleInfoTuple(const typename BaseClass::a_t& a, const typename BaseClass::b_t& b)
        : BaseClass(a,b)
        , RoleInfo()
    {}

    std::string getClassName() const { return "RoleInfoTuple"; }

    std::string toString() const
    {
        std::stringstream ss;
        ss << BaseClass::toString() << std::endl;
        ss << RoleInfo::toString();

        return ss.str();
    }

protected:
    graph_analysis::Vertex* getClone() const { return new RoleInfoTuple(*this); }

};

} // end namespace templ
#endif // TEMPL_LOCATION_TIMEPOINT_TUPLE_HPP
