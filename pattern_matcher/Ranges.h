#pragma once

#include <fstream>
#include <ranges>

class StreamIterator
{
public:
    StreamIterator(std::ifstream& aStream);

    char operator*() { return char{myCurrent}; }

    bool operator==(std::nullptr_t)
    {
        myCurrent = myStream.get();
        return myStream.eof();
    }

    bool operator!=(std::nullptr_t) { return !(*this == nullptr); }

private:
    int myCurrent;
    std::ifstream& myStream;
};

class StreamedRange
{
public:
    StreamedRange(std::ifstream& aStream);

    StreamIterator begin() { return myStream; }
    std::nullptr_t end() { return nullptr; }

private:
    std::ifstream& myStream;
};