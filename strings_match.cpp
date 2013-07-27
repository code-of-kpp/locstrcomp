#include <string>
#include <vector>
#include <algorithm>
#include <functional>
#include <stdexcept>

#include <boost/locale.hpp>
#include <boost/locale/collator.hpp>
#include <boost/locale/boundary.hpp>

#include "strings_match.hpp"


template<class T>
inline
T min(const T t1, const T t2, const T t3)
{
    return std::min(t1, std::min(t2, t3));
}


template<class T>
inline
T max(const T t1, const T t2, const T t3)
{
    return std::max(t1, std::max(t2, t3));
}


template<class T, class C>
unsigned int
levenshtein_distance(const T& a, const T& b, const C& equal)
{
    std::vector<unsigned int> current(b.size() + 1);
    std::vector<unsigned int> previous(b.size() + 1);

    for (unsigned int i = 0; i < previous.size(); i++)
    {
        previous[i] = i;
    }
    for (unsigned int i = 0; i < a.size(); i++)
    {
        current[0] = i + 1;
        for (unsigned int j = 0; j < b.size(); j++)
        {
            current[j + 1] = min(
                1 + current[j],
                1 + previous[j + 1],
                previous[j] + !equal(a[i], b[j])
            );
        }
        current.swap(previous);
    }
    return previous.back();
}


template <class RET, class T, class ARG>
class functor
{
public:
    typedef RET (T::*f) (ARG, ARG) const;

    functor(const f op, const T* ob):m_op(op), m_ob(ob) {}

    RET operator() (ARG arg1, ARG arg2) const
    {
        return ((this->m_ob)->*(this->m_op))(arg1, arg2);
    }

private:
    const f m_op;
    const T* m_ob;
};


template <class RET, class T, class ARG>
functor<RET, T, ARG>
make_functor(RET (T::*op) (ARG, ARG) const, const T* ob)
{
    return functor<RET, T, ARG>(op, ob);
}


strings_matcher::strings_matcher(const char* locale)
{
    m_locale = boost::locale::generator().generate(locale);
}


strings_matcher::~strings_matcher()
{}


unsigned int
strings_matcher::edit_distance(const std::string& referent,
                               const std::string& sample,
                               unsigned long* referent_size,
                               unsigned long* sample_size) const
{
    namespace boundary = boost::locale::boundary;

    int result;

    try
    {
        boundary::ssegment_index map_referent(
            boundary::character,
            referent.begin(), referent.end(),
            m_locale);

        boundary::ssegment_index map_sample(
            boundary::character,
            sample.begin(), sample.end(),
            m_locale);

        std::string last;

        std::vector<std::string> referent_symbols;
        referent_symbols.reserve(referent.size());
        for (boundary::ssegment_index::iterator
            i=map_referent.begin(); i != map_referent.end(); i++)
        {
            if (!ignore(*i, last))
            {
                referent_symbols.push_back(*i);
            }
            last = *i;
        }

        std::vector<std::string> sample_symbols;
        sample_symbols.reserve(sample.size());
        last.clear();
        for (boundary::ssegment_index::iterator
             i=map_sample.begin(); i != map_sample.end(); i++)
        {
            if (!ignore(*i, last)) sample_symbols.push_back(*i);
            last = *i;
        }

        result = levenshtein_distance(referent_symbols,
            sample_symbols,
            make_functor(&strings_matcher::equal, this));

        if(referent_size)
        {
            *referent_size = referent_symbols.size();
        }
        if(sample_size)
        {
            *sample_size = sample_symbols.size();
        }
    }
    catch(std::bad_cast)
    {
        result = -1;
    }
    catch(std::runtime_error)
    {
        result = -1;
    }

    if(result < 0)
    {
        result = levenshtein_distance(referent, sample,
                    std::equal_to<std::string::value_type>());
        if(referent_size) *referent_size = referent.size();
        if(sample_size) *sample_size = sample.size();
    }

    return result;
}


float
strings_matcher::norm_edit_distance(
        const std::string& referent,
        const std::string& sample) const
{
    unsigned long referent_size;
    unsigned long sample_size;
    float distance = 1.0f * this->edit_distance(referent, sample,
            &referent_size, &sample_size);
    return distance / max(1ul, referent_size, sample_size);
}


float strings_matcher::match_factor(
        const std::string& referent,
        const std::string& sample) const
{
    return 1.0f - this->norm_edit_distance(referent, sample);
}


bool strings_matcher::equal(const std::string& a,
                            const std::string& b) const
{
    using boost::locale::collator;
    using boost::locale::collator_base;
    using std::use_facet;
    using std::string;

    return (0 ==
        use_facet<collator<string::value_type> >(this->
            m_locale).compare(collator_base::primary, a, b));
}


bool strings_matcher::ignore(const std::string& s,
                             const std::string& prev) const
{
    if (s.size() > 1) return false; // TODO: Unicode punct
    if (s.size() == 0) return false;
    if (prev.size() == 1) // TODO: Unicode witespace
    {
        if (std::isspace(prev[0], this->m_locale) &&
            std::isspace(s[0], this->m_locale))
        {
            return true;
        }
    }

    return std::ispunct(s[0], this->m_locale);
}
