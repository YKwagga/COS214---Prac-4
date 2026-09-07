#ifndef FILM_ELEMENT_H
#define FILM_ELEMENT_H

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

class FilmElement {
public:
    virtual ~FilmElement() {}
    virtual const std::string& name() const = 0;
    virtual bool isLocation() const = 0;
    virtual bool needsClearance() const = 0;
    virtual void clearForShooting() = 0;
    virtual void markFilmed() = 0;
    virtual bool isFilmed() const = 0;
    virtual void print(std::size_t depth) const = 0;
    virtual std::shared_ptr<std::size_t> versionToken() const = 0;
    virtual void appendChildren(std::vector<FilmElement*>& output) const = 0;
};

#endif