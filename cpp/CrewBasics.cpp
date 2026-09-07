#include "CrewBasics.h"

OnSiteCrew::OnSiteCrew(const std::string& name) : crewName(name), present(true) {}
const std::string& OnSiteCrew::name() const { return crewName; }
bool OnSiteCrew::isPresent() const { return present; }
bool OnSiteCrew::isVip() const { return false; }
bool OnSiteCrew::requiredOnSite() const { return true; }
std::string OnSiteCrew::description() const { return "On-site crew"; }
void OnSiteCrew::setPresent(bool value) { present = value; }

OffSiteCrew::OffSiteCrew(const std::string& name) : crewName(name), present(true) {}
const std::string& OffSiteCrew::name() const { return crewName; }
bool OffSiteCrew::isPresent() const { return present; }
bool OffSiteCrew::isVip() const { return false; }
bool OffSiteCrew::requiredOnSite() const { return false; }
std::string OffSiteCrew::description() const { return "Off-site crew"; }
void OffSiteCrew::setPresent(bool value) { present = value; }