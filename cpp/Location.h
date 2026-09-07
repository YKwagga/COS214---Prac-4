#ifndef LOCATION_H
#define LOCATION_H

#include "FilmElement.h"

class Location : public FilmElement {
public:
    Location(const std::string& name, bool cleared = false);
    virtual ~Location() {}

    const std::string& name() const;
    bool isLocation() const;
    bool needsClearance() const;
    void clearForShooting();
    void markFilmed();
    bool isFilmed() const;
    void print(std::size_t depth) const;
    std::shared_ptr<std::size_t> versionToken() const;
    void appendChildren(std::vector<FilmElement*>& output) const;

private:
    std::string locationName;
    bool cleared;
    bool filmed;
};

#endif