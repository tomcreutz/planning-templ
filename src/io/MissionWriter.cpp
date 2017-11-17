#include "MissionWriter.hpp"
#include "../utils/XMLUtils.hpp"
#include "../solvers/csp/FluentTimeResource.hpp"
#include "../solvers/temporal/point_algebra/QualitativeTimePointConstraint.hpp"
#include "../io/MissionRequirements.hpp"

using namespace templ::utils;
using namespace templ::solvers::csp;
namespace pa = templ::solvers::temporal::point_algebra;

namespace templ {
namespace io {

void MissionWriter::write(const std::string& path, const Mission& mission, const std::string& encoding)
{
    xmlTextWriterPtr writer;
    xmlDocPtr doc;

    doc = xmlNewDoc(BAD_CAST XML_DEFAULT_VERSION);
    if(doc == NULL)
    {
        throw std::runtime_error("templ::io::MissionWriter::write: error creating"
                " xml document");
    }

    // Create a new XmlWriter for uri with no compression
    writer = xmlNewTextWriterDoc(&doc, 0);
    if(writer == NULL)
    {
        throw std::runtime_error("templ::io::MissionWriter::write: failed to open file '"
                + path + "'");
    }

    // Start the document with the xml default for the version and given encoding
    int rc = xmlTextWriterStartDocument(writer, NULL, encoding.c_str(), NULL);
    if(rc < 0)
    {
        throw std::runtime_error("templ::io::MissionWriter::write: failed to"
                " start document:  '" + path + "'");
    }

    Mission::Ptr mission_p(new Mission(mission));
    mission_p->prepareTimeIntervals();

    XMLUtils::startElement(writer, "mission");

    XMLUtils::startElement(writer, "name");
    XMLUtils::writeString(writer, mission.getName());
    XMLUtils::endElement(writer); // end name

    XMLUtils::startElement(writer, "description");
    XMLUtils::writeString(writer, mission.getDescription());
    XMLUtils::endElement(writer); // end description

    XMLUtils::startElement(writer, "organization_model");
    XMLUtils::writeString(writer, mission.getOrganizationModel()->ontology()->getIRI().toString());
    XMLUtils::endElement(writer); // end organization_model

    XMLUtils::startElement(writer, "resources");
    XMLUtils::endElement(writer); // end resources;

    XMLUtils::startElement(writer, "constants");
    // location(id,radius,latitude,longitude) || location(id,x,y,z)
    XMLUtils::endElement(writer); // end constants

    XMLUtils::startElement(writer, "requirements");
    std::vector<FluentTimeResource> resources = Mission::getResourceRequirements(mission_p);
    int requirementId = 0;
    for(const FluentTimeResource& ftr : resources)
    {
        XMLUtils::startElement(writer, "requirement");
        std::stringstream ss;
        ss << requirementId++;
        XMLUtils::writeAttribute(writer, "id", ss.str());

        XMLUtils::startElement(writer, "spatial-requirement");
        XMLUtils::startElement(writer, "location");
        XMLUtils::startElement(writer, "id");
        XMLUtils::writeString(writer, ftr.getLocation()->getInstanceName().c_str());
        XMLUtils::endElement(writer); // end location id
        XMLUtils::endElement(writer); // end location
        XMLUtils::endElement(writer); // end spatial requirement

        XMLUtils::startElement(writer, "temporal-requirement");
        XMLUtils::startElement(writer, "from");
        XMLUtils::writeString(writer, ftr.getInterval().getFrom()->getLabel());
        XMLUtils::endElement(writer);
        XMLUtils::startElement(writer, "to");
        XMLUtils::writeString(writer, ftr.getInterval().getTo()->getLabel());
        XMLUtils::endElement(writer);
        XMLUtils::endElement(writer); // end temporal requirement

        XMLUtils::startElement(writer, "resource-requirement");
        std::set<owlapi::model::IRI> models;
        {
            organization_model::ModelPool::const_iterator cit = ftr.minCardinalities.begin();
            for(; cit != ftr.minCardinalities.end(); ++cit)
            {
                models.insert(cit->first);
            }
        }
        {
            organization_model::ModelPool::const_iterator cit = ftr.maxCardinalities.begin();
            for(; cit != ftr.maxCardinalities.end(); ++cit)
            {
                models.insert(cit->first);
            }
        }
        for(const owlapi::model::IRI& model : models)
        {
            XMLUtils::startElement(writer, "resource");

            XMLUtils::startElement(writer, "model");
            XMLUtils::writeString(writer, model.toString());
            XMLUtils::endElement(writer); // end model;

            // minCardinalities
            organization_model::ModelPool::const_iterator minIt =  ftr.minCardinalities.find(model);
            if(minIt != ftr.minCardinalities.end())
            {
                std::stringstream sm;
                sm << minIt->second;
                XMLUtils::startElement(writer, "minCardinality");
                XMLUtils::writeString(writer, sm.str());
                XMLUtils::endElement(writer); // end minCardinality
            }

            organization_model::ModelPool::const_iterator maxIt =  ftr.maxCardinalities.find(model);
            if(maxIt != ftr.maxCardinalities.end())
            {
                std::stringstream sm;
                sm << maxIt->second;
                XMLUtils::startElement(writer, "maxCardinality");
                XMLUtils::writeString(writer, sm.str());
                XMLUtils::endElement(writer); // end maxCardinality
            }

            XMLUtils::endElement(writer); // end resource
        }
        XMLUtils::endElement(writer); // end requirement
    }
    // each do
    XMLUtils::endElement(writer); // end requirements


    std::vector<solvers::Constraint::Ptr> constraints = mission.getConstraints();
    XMLUtils::startElement(writer, "constraints");
    XMLUtils::startElement(writer, "temporal-constraints");
    for(const solvers::Constraint::Ptr& c : constraints)
    {
        pa::QualitativeTimePointConstraint::Ptr qtpc = dynamic_pointer_cast<pa::QualitativeTimePointConstraint>(c);
        std::string tag = io::TemporalConstraint::toXML(qtpc->getType());
        XMLUtils::startElement(writer, tag);
        XMLUtils::writeAttribute(writer, "lval", qtpc->getSourceVariable()->getLabel());
        XMLUtils::writeAttribute(writer, "rval", qtpc->getTargetVariable()->getLabel());
        XMLUtils::endElement(writer);
    }

    XMLUtils::endElement(writer); // end temporal-constraints
    XMLUtils::endElement(writer); // end constraints

    XMLUtils::endElement(writer);  // end mission

    xmlTextWriterEndDocument(writer);

    xmlFreeTextWriter(writer);
    xmlSaveFileEnc(path.c_str(), doc, encoding.c_str());
    xmlFreeDoc(doc);

    XMLUtils::lint(path);
}

} // end namespace io
} // end namespace templ