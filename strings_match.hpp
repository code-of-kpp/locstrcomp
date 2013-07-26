
#ifndef STRINGS_MATCH_HPP
#define STRINGS_MATCH_HPP

#include <locale>
#include <string>


class strings_matcher
{
public:
    strings_matcher(const char* locale);
    ~strings_matcher();
    unsigned int edit_distance(
            const std::string& referent,
            const std::string& sample,
            unsigned long* referent_size=0,
            unsigned long* sample_size=0) const;
    float norm_edit_distance(
            const std::string& referent,
            const std::string& sample) const;
    float match_factor(
            const std::string& referent,
            const std::string& sample) const;
private:
    std::locale m_locale;
    strings_matcher(const strings_matcher&);

    bool equal(const std::string&, const std::string&) const;
    bool ignore(const std::string&, const std::string&) const;
};

#endif // STRINGS_MATCH_HPP
