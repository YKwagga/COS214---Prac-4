#include "CrewDecorator.h"

CrewDecorator::CrewDecorator(std::unique_ptr<CrewMember> wrapped)
    : component(std::move(wrapped)) {}
const std::string& CrewDecorator::name() const { return component->name(); }
bool CrewDecorator::isPresent() const { return component->isPresent(); }
bool CrewDecorator::isVip() const { return component->isVip(); }
bool CrewDecorator::requiredOnSite() const { return component->requiredOnSite(); }
std::string CrewDecorator::description() const { return component->description(); }
void CrewDecorator::setPresent(bool present) { component->setPresent(present); }

RoleDecorator::RoleDecorator(std::unique_ptr<CrewMember> wrapped, const std::string& roleName, bool vipRole)
    : CrewDecorator(std::move(wrapped)), role(roleName), vip(vipRole) {}
bool RoleDecorator::isVip() const { return vip || component->isVip(); }
std::string RoleDecorator::description() const {
    return component->description() + ", role: " + role;
}