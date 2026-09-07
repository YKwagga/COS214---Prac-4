#ifndef CREW_MEMBER_H
#define CREW_MEMBER_H

#include <string>

class CrewMember {
public:
    virtual ~CrewMember() {}
    virtual const std::string& name() const = 0;
    virtual bool isPresent() const = 0;
    virtual bool isVip() const = 0;
    virtual bool requiredOnSite() const = 0;
    virtual std::string description() const = 0;
    virtual void setPresent(bool present) = 0;
};

#endif