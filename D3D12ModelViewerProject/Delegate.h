#pragma once

#include <list>
#include <functional>

template <class... ARGS>
class SimpleDelegate
{
public:
    typedef typename std::list<std::function<void(ARGS...)>>::iterator iterator;

    void operator () (const ARGS... args)
    {
        for (auto& func : functions)
        {
            func(args...);
        }
    }

    SimpleDelegate& operator += (std::function<void(ARGS...)> const& func)
    {
        functions.push_back(func);
        return *this;
    }

    iterator begin() noexcept
    {
        return functions.begin();
    }
    iterator end() noexcept
    {
        return functions.end();
    }
    void clear()
    {
        functions.clear();
    }

private:
    std::list<std::function<void(ARGS...)>> functions;
};