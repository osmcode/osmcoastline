#ifndef VERBOSE_OUTPUT_HPP
#define VERBOSE_OUTPUT_HPP

class VerboseOutput {

    time_t m_start;
    bool m_verbose;
    bool m_newline;

public:

    VerboseOutput(bool verbose) :
        m_start(time(nullptr)),
        m_verbose(verbose),
        m_newline(true) {
    }

    int runtime() const {
        return time(nullptr) - m_start;
    }

    void start_line() {
        if (m_newline) {
            int elapsed = time(nullptr) - m_start;
            char timestr[20];
            snprintf(timestr, sizeof(timestr)-1, "[%2d:%02d] ", elapsed / 60, elapsed % 60);
            std::cerr << timestr;
            m_newline = false;
        }
    }

    template<typename T>
    friend VerboseOutput& operator<<(VerboseOutput& out, T t) {
        if (out.m_verbose) {
            std::ostringstream o;
            o << t;
            out.start_line();
            std::cerr << o.str();
            if (o.str()[o.str().size()-1] == '\n') {
                out.m_newline = true;
            }
        }
        return out;
    }

}; // class VerboseOutput

#endif // VERBOSE_OUTPUT_HPP
