#include "MiscJsonParser.h"

nlohmann::json nlohmann::sax_parse_json_object(std::istream& stream, std::string const& key, std::size_t level)
{
    using dom_parser = detail::json_sax_dom_parser<json>;
    class sax_object_parser
    : public dom_parser
    {
    public:
        sax_object_parser(json& j, std::string const& key, std::size_t level)
        : dom_parser(j, false)
        , m_key(key)
        , m_level(level)
        {
        }

        bool null()
        {
            return m_should_proceed ? dom_parser::null() : true;
        }

        bool boolean(bool val)
        {
            return m_should_proceed ? dom_parser::boolean(val) : true;
        }

        bool number_integer(number_integer_t val)
        {
            return m_should_proceed ? dom_parser::number_integer(val) : true;
        }

        bool number_unsigned(number_unsigned_t val)
        {
            return m_should_proceed ? dom_parser::number_unsigned(val) : true;
        }

        bool number_float(number_float_t val, const string_t& s)
        {
            return m_should_proceed ? dom_parser::number_float(val, s) : true;
        }

        bool string(string_t& val)
        {
            return m_should_proceed ? dom_parser::string(val) : true;
        }

        bool binary(binary_t& val)
        {
            return m_should_proceed ? dom_parser::binary(val) : true;
        }

        bool start_object(std::size_t elements)
        {
            ++m_current_level;
            return (m_should_proceed || m_current_level == m_level) ? dom_parser::start_object(elements) : true;
        }

        bool key(string_t& val)
        {
            m_should_proceed = (m_current_level == m_level) ? val == m_key : m_should_proceed;
            return m_should_proceed ? dom_parser::key(val) : true;
        }

        bool end_object()
        {
            --m_current_level;
            return (m_should_proceed || m_current_level == m_level - 1_z) ? dom_parser::end_object() : true;
        }

        bool start_array(std::size_t len)
        {
            return m_should_proceed ? dom_parser::start_array(len) : true;
        }

        bool end_array()
        {
            return m_should_proceed ? dom_parser::end_array() : true;
        }

        bool parse_error(std::size_t position, std::string const& last_token, json::exception const& ex)
        {
            std::cerr << "parse error at input byte " << position << "\n"
                      << ex.what() << "\n"
                      << "last read: \"" << last_token << "\"" << std::endl;
            return false;
        }

    private:
        std::string const m_key;
        std::size_t const m_level;
        bool m_should_proceed = false;
        size_t m_current_level = 0_z;
    };

    json result;
    sax_object_parser sax(result, key, level);
    if(json::sax_parse(stream, &sax))
    {
        return result;
    }
    return {};
}
