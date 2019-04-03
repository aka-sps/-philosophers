#include <iomanip>
#include <iostream>
#include <mutex>
#include <random>
#include <chrono>
#include <algorithm>
#include <condition_variable>
#include <thread>
#include <iterator>
#include <string>
#include <cstdint>
#include <atomic>

namespace {
unsigned g_max_interval_ms = 10000;
}
namespace philosophers {

class Fork
{
public:
    Fork(unsigned _id)
        : m_id(_id)
        , m_is_available(true)
    {}

    unsigned
        id() const
    {
        return m_id;
    }

    bool
        try_to_get()
    {
        std::unique_lock<std::mutex> lock(m_mutex);

        if (this->m_is_available) {
            this->m_is_available = false;
            return true;
        }

        return false;
    }

    bool
        wait_until_available()
    {
        std::unique_lock<std::mutex> lock(m_mutex);

        if (this->m_is_available) {
            this->m_is_available = false;
            return true;
        }

        using namespace std::chrono;
        this->m_conditional_variable.wait_for(lock, milliseconds(g_max_interval_ms));

        if (this->m_is_available) {
            this->m_is_available = false;
            return true;
        }

        return false;
    }

    void
        free()
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        this->m_is_available = true;
        this->m_conditional_variable.notify_one();
    }

private:
    unsigned m_id;
    bool volatile m_is_available;
    std::mutex m_mutex;
    std::condition_variable m_conditional_variable;
};

using std::chrono::steady_clock;

class Monitor;

class Philosopher
{
public:
    enum class States
    {
        thinks,
        hungry,
        dines,
#ifdef PHILOSOPHERS_STARVATION
        dead
#endif
    };

    Philosopher(unsigned _id, std::shared_ptr<Fork> const& _p_left, std::shared_ptr<Fork> const& _p_right, Monitor* _p_canteen)
        : m_id(_id)
        , m_state(States::thinks)
        , m_p_left_fork(_p_left)
        , m_p_right_fork(_p_right)
        , m_p_monitor(_p_canteen)
#ifdef PHILOSOPHERS_STARVATION
        , m_last_eating(steady_clock::now())
#endif
    {}

    unsigned
        id()const
    {
        return m_id;
    }

    void
        operator()()
    {
        try {
            m_thread_id = std::this_thread::get_id();

            for (;;) {
                thinking();
                aquire_forks();
                eating();
            }
        } catch (...) {
            std::cerr << "Catch unhandled exception in philosopher id=" << id() << std::endl;
        }
    }

    /// common thread worker
    static void
        worker(std::shared_ptr<Philosopher> const& _p_philosopfer)
    {
        (*_p_philosopfer)();
    }

    States
        state()const
    {
        return m_state;
    }

    std::thread::id
        thread_id()const
    {
        return m_thread_id;
    }

private:
    void
        thinking()
    {
        state(States::thinks);
        std::this_thread::sleep_for(random_interval());
    }

    void
        aquire_forks()
    {
        state(States::hungry);

        for (;;) {
            while (!this->m_p_left_fork->wait_until_available()) {
                check_for_death();
            }

            if (this->m_p_right_fork->try_to_get()) {
                break;
            }

            this->m_p_left_fork->free();

            while (!this->m_p_right_fork->wait_until_available()) {
                check_for_death();
            }

            if (this->m_p_left_fork->try_to_get()) {
                break;
            }

            this->m_p_right_fork->free();
        }
    }

    void
        check_for_death()
    {
#ifdef PHILOSOPHERS_STARVATION
        using namespace std::chrono;
        milliseconds const time_span = duration_cast<milliseconds>(steady_clock::now() - this->m_last_eating);
        if (milliseconds(m_death_threshold * g_max_interval_ms) < time_span) {
            state(States::dead);
            for (;;) {
                std::this_thread::sleep_for(milliseconds(g_max_interval_ms));
            }
        }
#endif            
    }

    void
        eating()
    {
        state(States::dines);
        std::this_thread::sleep_for(random_interval());
        this->m_p_right_fork->free();
        this->m_p_left_fork->free();
#ifdef PHILOSOPHERS_STARVATION
        this->m_last_eating = steady_clock::now();
#endif
    }

    std::chrono::milliseconds
        random_interval()const
    {
        /// @brief mutex protected random generator
        class Random_generator
            : public std::default_random_engine
        {
            typedef std::default_random_engine Base_class;
        public:
            Random_generator()
                : Base_class(result_type(std::chrono::system_clock::now().time_since_epoch().count()))
            {}
            result_type
                operator()()
            {
                std::lock_guard<decltype(m_mutex)> guard(m_mutex);
                return Base_class::operator()();
            }

        private:
            std::mutex m_mutex;
        };
        static Random_generator g_rnd;
        static std::uniform_int_distribution<unsigned> distribution(1, g_max_interval_ms);
        return std::chrono::milliseconds(distribution(g_rnd));
    }

    inline void
        state(States _state);

    unsigned m_id;
    States m_state;

public:
    std::shared_ptr<Fork> m_p_left_fork;
    std::shared_ptr<Fork> m_p_right_fork;

private:
    Monitor* m_p_monitor;
    std::thread::id m_thread_id;

#ifdef PHILOSOPHERS_STARVATION
    steady_clock::time_point m_last_eating;
    static unsigned const m_death_threshold = 4;
#endif
};

class Monitor
{
public:
    typedef std::pair<unsigned, Philosopher::States> state_log_element_type;

    void
        log_state(Philosopher const* _p_philosopher)
    {
        if (!_p_philosopher) {
            return;
        }

        {
            std::lock_guard<decltype(this->m_log_queue_mutex)> locker(this->m_log_queue_mutex);
            m_log_queue.emplace_back(_p_philosopher->id(), _p_philosopher->state());
        }

        this->m_state_logged_event.notify_one();
    }

    void
        monitor_worker()
    {
        std::mutex event_mutex;
        std::unique_lock<decltype(event_mutex)> locker(event_mutex);
        decltype(m_log_queue) work_log;

        for (;;) {
            if (!m_log_queue.empty()) {
                {
                    std::lock_guard<decltype(this->m_log_queue_mutex)> locker(this->m_log_queue_mutex);
                    std::swap(work_log, m_log_queue);
                }
                events_logger(work_log);
                work_log.clear();
            } else if (std::cv_status::timeout == this->m_state_logged_event.wait_for(locker, std::chrono::milliseconds(10 * g_max_interval_ms))) {
                throw std::runtime_error("No events for a long time");
            }
        }
    }

protected:
    typedef std::vector<state_log_element_type> log_queue_type;
    virtual void
        events_logger(log_queue_type const& work_log) = 0;

    std::mutex mutable m_log_queue_mutex;
    std::condition_variable m_state_logged_event;
    log_queue_type m_log_queue;
};

void
Philosopher::state(States _state)
{
    this->m_state = _state;

    if (this->m_p_monitor) {
        this->m_p_monitor->log_state(this);
    }
}

class Canteen
{
public:
    explicit
        Canteen(Monitor& _monitor, unsigned _number_of_philosophers)
        : m_p_monitor(&_monitor)
    {
        if (_number_of_philosophers < 2) {
            throw std::invalid_argument("Invalid number of philosophers (<2)");
        }

        std::vector<std::shared_ptr<Fork>> forks;
        forks.reserve(_number_of_philosophers);

        for (unsigned i = 0; i < _number_of_philosophers; ++i) {
            forks.push_back(std::make_shared<Fork>(i));
        }

        this->m_philosophers.reserve(_number_of_philosophers);

        for (unsigned i = 0; i < _number_of_philosophers; ++i) {
            this->m_philosophers.push_back(std::make_shared<Philosopher>(
                i,
                forks[i],
                forks[(i + 1) % _number_of_philosophers],
                this->m_p_monitor));
        }
    }

    void
        operator()()
    {
        try {
            std::vector<std::thread> threads;
            threads.reserve(this->m_philosophers.size());
            auto const thread_creator = [](std::shared_ptr<Philosopher> const & ptr) {return std::thread(Philosopher::worker, ptr); };
            std::transform(this->m_philosophers.cbegin(), m_philosophers.cend(),
                           std::back_inserter(threads),
                           thread_creator);
            this->m_p_monitor->monitor_worker();
        } catch (std::exception const& _excp) {
            std::cerr << "Catch std::exception:" << _excp.what() << std::endl;
        } catch (...) {
            std::cerr << "Catch Unknown exception!" << std::endl;
        }

        throw std::logic_error("Unexpected exit");
    }

private:
    std::vector<std::shared_ptr<Philosopher>> m_philosophers;
    Monitor* const m_p_monitor;
};

class Simple_log_monitor
    : public Monitor
{
protected:
    void
        events_logger(log_queue_type const& work_log) override
    {
        auto const log_event = [](log_queue_type::value_type const & el) {
            std::cout << "Philosopher #" << el.first << " ";

            switch (el.second) {
            case Philosopher::States::thinks:
                std::cout << "thinks";
                break;

            case Philosopher::States::hungry:
                std::cout << "hungry";
                break;

            case Philosopher::States::dines:
                std::cout << "dines";
                break;

#ifdef PHILOSOPHERS_STARVATION
            case Philosopher::States::dead:
                std::cout << "die";
                break;
#endif

            default:
                std::cout << "?????";
                break;
            }

            std::cout << std::endl;
        };

        std::for_each(std::begin(work_log), std::end(work_log), log_event);
    }
};

class Waterfall_monitor
    : public Monitor
{
protected:
    void
        events_logger(log_queue_type const& work_log)override
    {
        auto const log_event = [this](log_queue_type::value_type const & el) {
            if (this->m_buffer.size() <= el.first) {
                this->m_buffer.append(el.first + 1 - this->m_buffer.size(), symb(Philosopher::States::thinks));
            }

            this->m_buffer[el.first] = symb(el.second);
        };
        std::for_each(std::begin(work_log), std::end(work_log), log_event);
        std::cout << this->m_buffer << std::endl;
    }

private:
    static char
        symb(Philosopher::States _state)
    {
        switch (_state) {
        case Philosopher::States::thinks:
            return ' ';
            break;

        case Philosopher::States::hungry:
            return '.';
            break;

        case Philosopher::States::dines:
            return '|';
            break;

#ifdef PHILOSOPHERS_STARVATION
        case Philosopher::States::dead:
            return '#';
            break;
#endif
        default:
            return '?';
            break;
        }
    }

    std::string m_buffer;
};

}  // namespace philosophers

int
main(int argc, char* argv[])
{
    try {
        std::cout << "Dining philosophers problem " << GIT_DESCRIBE << std::endl;
        using namespace philosophers;
        unsigned const num_ph = argc < 2 ? 64 : std::max(2, atoi(argv[1]));
        g_max_interval_ms = argc < 3 ? 10000 : std::max(2, atoi(argv[2]));
        Waterfall_monitor wf_monitor;
        Canteen canteen(wf_monitor, num_ph);
        canteen();
        return 0;
    } catch (std::exception const& exc) {
        std::cerr << "Unhandled std::exception: " << exc.what() << std::endl;
    } catch (...) {
        std::cerr << "Unhandled unknown exception" << std::endl;
    }

    return EXIT_FAILURE;
}
