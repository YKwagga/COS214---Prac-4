#ifndef CREW_BASICS_H
#define CREW_BASICS_H

#include "CrewMember.h"

class OnSiteCrew : public CrewMember {
public:
    explicit OnSiteCrew(const std::string& name);
    virtual ~OnSiteCrew() {}
    const std::string& name() const;
    bool isPresent() const;
    bool isVip() const;
    bool requiredOnSite() const;
    std::string description() const;
    void setPresent(bool present);
private:
    std::string crewName;
    bool present;
};

class OffSiteCrew : public CrewMember {
public:
    explicit OffSiteCrew(const std::string& name);
    virtual ~OffSiteCrew() {}
    const std::string& name() const;
    bool isPresent() const;
    bool isVip() const;
    bool requiredOnSite() const;
    std::string description() const;
    void setPresent(bool present);
private:
    std::string crewName;
    bool present;
};

#endif