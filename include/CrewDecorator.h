#ifndef CREW_DECORATOR_H
#define CREW_DECORATOR_H

#include "CrewMember.h"
#include <memory>

class CrewDecorator : public CrewMember {
public:
    explicit CrewDecorator(std::unique_ptr<CrewMember> component);
    virtual ~CrewDecorator() {}
    const std::string& name() const;
    bool isPresent() const;
    bool isVip() const;
    bool requiredOnSite() const;
    std::string description() const;
    void setPresent(bool present);
protected:
    std::unique_ptr<CrewMember> component;
};

class RoleDecorator : public CrewDecorator {
public:
    RoleDecorator(std::unique_ptr<CrewMember> component, const std::string& role, bool vip);
    virtual ~RoleDecorator() {}
    bool isVip() const;
    std::string description() const;
private:
    std::string role;
    bool vip;
};

#endif