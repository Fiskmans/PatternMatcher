
#include "catch_pattern_matcher/JSON.h"

#include <filesystem>
#include <fstream>

#include "catch2/catch_all.hpp"
#include "pattern_matcher/PatternBuilder.h"

namespace
{

    // This class shows how to implement a simple generator for Catch tests
    class FilesGenerator final : public Catch::Generators::IGenerator<std::filesystem::path>
    {
    public:
        FilesGenerator(std::filesystem::path aRoot, std::string aStartAt = "") : myIterator(aRoot)
        {
            assert(myIterator != std::filesystem::directory_iterator{});

            if (!aStartAt.empty())
            {
                while (myIterator->path().filename() != aStartAt)
                {
                    assert(next());
                }
            }
        }

        std::filesystem::path const& get() const override;
        bool next() override
        {
            myIterator++;
            return myIterator != std::filesystem::directory_iterator{};
        }

    private:
        std::filesystem::directory_iterator myIterator;
    };

    std::filesystem::path const& FilesGenerator::get() const { return myIterator->path(); }

    Catch::Generators::GeneratorWrapper<std::filesystem::path> Files(std::filesystem::path aRoot,
                                                                     std::string aStartAt = "")
    {
        return Catch::Generators::GeneratorWrapper<std::filesystem::path>(new FilesGenerator(aRoot, aStartAt));
    }

}  // namespace

TEST_CASE("integration::json", "")
{
    pattern_matcher::PatternMatcher matcher = MakeJsonParser().Finalize();

    std::filesystem::path file = GENERATE(Files(CATCH_JSON_TEST_CASES_PATH));

    std::ifstream in(file);

    std::string all;
    std::string line;
    while (std::getline(in, line))
    {
        if (!all.empty())
            all += "\n";
        all += line;
    }

    char type = file.filename().c_str()[0];

    CAPTURE(file.filename());

    try
    {
        switch (type)
        {
            case 'i':
                break;
            case 'n': {
                auto match = matcher.Match("value", all);
                if (match)
                    REQUIRE(!(*match == all));
            }
            break;
            case 'y':
                auto result = matcher.Match("value", all);
                REQUIRE(result);
                break;
        }
    }
    catch (const std::exception& e)
    {
        REQUIRE_FALSE(e.what());
    }
}