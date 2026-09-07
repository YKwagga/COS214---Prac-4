#ifndef AREA_H
#define AREA_H

#include "FilmElement.h"
#include <functional>
#include <vector>

class Area : public FilmElement {
public:
    explicit Area(const std::string& name,
                  const std::shared_ptr<std::size_t>& versionToken = std::shared_ptr<std::size_t>(),
                  const std::shared_ptr<std::function<void()> >& mutationObserver =
                      std::shared_ptr<std::function<void()> >());
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
    void setMutationContext(const std::shared_ptr<std::size_t>& versionToken,
                            const std::shared_ptr<std::function<void()> >& mutationObserver);

private:
    std::string areaName;
    std::vector<std::unique_ptr<FilmElement> > children;
    std::shared_ptr<std::size_t> version;
    std::shared_ptr<std::function<void()> > observer;
};

#endif