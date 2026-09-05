#ifndef AREA_H
#define AREA_H

#include "FilmElement.h"
#include <vector>

class Area : public FilmElement {
public:
    explicit Area(const std::string& name,
                  const std::shared_ptr<std::size_t>& versionToken = std::shared_ptr<std::size_t>());
    virtual ~Area() {}

    void add(std::unique_ptr<FilmElement> child);
    bool remove(const std::string& childName);
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
    std::string areaName;
    std::vector<std::unique_ptr<FilmElement> > children;
    std::shared_ptr<std::size_t> version;
};

#endif