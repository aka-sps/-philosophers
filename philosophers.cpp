#include <iomanip>
#include <iostream>
#include <mutex>
#include <random>
#include <chrono>
#include <algorithm>
#include <queue>
#include <condition_variable>
#include <memory>
#include <thread>
#include <iterator>
#include <string>

namespace philosophers {

/// @brief Multithread random generator protected by mutex
class Random_generator
    : public std::default_random_engine
{
    typedef std::default_random_engine Base_class;
public:
    Random_generator()
        : std::default_random_engine(result_type(std::chrono::system_clock::now().time_since_epoch().count()))
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

Random_generator g_rnd;

/// Fork is simple mutex
typedef std::mutex Fork;

class Monitor;

class Philosopher
{
public:
    enum class States
    {
        thinks,
        hungry,
        dines
    };
    Philosopher(unsigned _id, std::shared_ptr<Fork> const& _p_left, std::shared_ptr<Fork> const& _p_right, Monitor* _p_canteen = nullptr)
        : m_id(_id)
        , m_p_left(_p_left)
        , m_p_right(_p_right)
        , m_state(States::thinks)
        , m_p_monitor(_p_canteen)
    {}

    unsigned id()const
    {
        return m_id;
    }
    void operator()()
    {
        for (;;) {
            try {
                thinks();
                aquire_forks();
                eating();
            } catch (...) {
                /// ignored
            }
        }
    }

    /// common thread worker
    static void worker(std::shared_ptr<Philosopher> const& _p_philosopfer)
    {
        (*_p_philosopfer)();
    }

    States state()const
    {
        return m_state;
    }
private:
    void thinks()
    {
        state(States::thinks);
        std::this_thread::sleep_for(random_interval());
    }

    void aquire_forks()
    {
        state(States::hungry);
        std::unique_lock<decltype(m_mutex)> locker(m_mutex);
        while (-1 != std::try_lock(*m_p_left, *m_p_right)) {
            m_cv.wait(locker);
        }
    }

    void eating()
    {
        state(States::dines);
        std::this_thread::sleep_for(random_interval());
        m_p_left->unlock();
        m_p_right->unlock();
    }
    static std::chrono::milliseconds random_interval()
    {
        static std::uniform_int_distribution<unsigned> distrigution(1, 10000);
        auto const rnd = distrigution(g_rnd);
        return std::chrono::milliseconds(rnd);
    }
    inline void state(States _state);

    unsigned m_id;
    std::shared_ptr<Fork> m_p_left;
    std::shared_ptr<Fork> m_p_right;
    States m_state;
    Monitor* m_p_monitor;

    static std::mutex m_mutex;
    static std::condition_variable m_cv;
};

std::mutex Philosopher::m_mutex;
std::condition_variable Philosopher::m_cv;

class Monitor
{
public:
    typedef std::pair<unsigned, Philosopher::States> state_log_element_type;
    void log_state(Philosopher const* _p_philosopher)
    {
        if (_p_philosopher) {
            std::unique_lock<decltype(m_log_queue_mutex)> locker(m_log_queue_mutex);
            m_log_queue.emplace_back(_p_philosopher->id(), _p_philosopher->state());
            m_cv.notify_one();
        }
    }
    void monitor_worker()
    {
        decltype(m_log_queue) work_log;
        for (;;) {
            {
                std::unique_lock<decltype(m_log_queue_mutex)> locker(m_log_queue_mutex);
                while (m_log_queue.empty()) {
                    m_cv.wait(locker);
                }
                std::swap(work_log, m_log_queue);
            }
            monitor(work_log);
            work_log.clear();
        }
    }

protected:
    typedef std::vector<state_log_element_type> log_queue_type;
    virtual void monitor(log_queue_type const& work_log) = 0;

    std::mutex m_log_queue_mutex;
    std::condition_variable m_cv;
    log_queue_type m_log_queue;
};

void Philosopher::state(States _state)
{
    std::unique_lock<decltype(m_mutex)> locker(m_mutex);
    m_state = _state;
    if (m_p_monitor) {
        m_p_monitor->log_state(this);
    }
    m_cv.notify_all();
}

template<class TyMonitor>
class Canteen
{
public:
    explicit Canteen(unsigned _number_of_philosophers = 5)
    {
        if (_number_of_philosophers < 2) {
            throw std::invalid_argument("Invalid number of philosophers (<2)");
        }
        std::vector<std::shared_ptr<Fork> > forks;
        forks.reserve(_number_of_philosophers);
        std::generate_n(std::back_inserter(forks), _number_of_philosophers, []() {return std::make_shared<Fork>(); });

        m_philosophers.reserve(_number_of_philosophers);
        for (unsigned i = 0; i < _number_of_philosophers; ++i) {
            m_philosophers.push_back(std::make_shared<Philosopher>(
                i,
                forks[i],
                forks[(i + 1) % _number_of_philosophers],
                &m_monitor));
        }
    }
    void operator()()
    {
        std::vector<std::thread> threads;
        threads.reserve(m_philosophers.size());
        std::transform(m_philosophers.cbegin(), m_philosophers.cend(), std::back_inserter(threads),
                       [](std::shared_ptr<Philosopher> const& ptr) {return std::thread(Philosopher::worker, ptr); });
        m_monitor.monitor_worker();
    }

private:
    std::vector<std::shared_ptr<Philosopher> > m_philosophers;
    TyMonitor m_monitor;
};

class Simple_log_monitor
    : public Monitor
{
protected:
    virtual void monitor(log_queue_type const& work_log)
    {
        for (auto el : work_log) {
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
            default:
                std::cout << "?????";
                break;
            }
            std::cout << std::endl;
        }
    }
};

class Waterfall_monitor
    : public Monitor
{
protected:
    virtual void monitor(log_queue_type const& work_log)override
    {
        for (auto el : work_log) {
            if (m_buffer.size() <= el.first) {
                m_buffer.append(el.first + 1 - m_buffer.size(), symb(Philosopher::States::thinks));
            }
            m_buffer[el.first] = symb(el.second);
        }
        std::cout << m_buffer << std::endl;
    }
private:
    static char symb(Philosopher::States _state)
    {
        switch (_state) {
        case Philosopher::States::thinks:
            return ' ';
            break;
        case Philosopher::States::hungry:
            return '-';
            break;
        case Philosopher::States::dines:
            return '|';
            break;
        default:
            return '?';
            break;
        }
    }
    std::string m_buffer;

};

}  // namespace philosophers

int main(int argc, char* argv[])
{
    try {
        unsigned const num_ph = argc < 2 ? 64 : atoi(argv[1]);
        using namespace philosophers;
        Canteen<Waterfall_monitor> canteen(num_ph);
        canteen();
        return 0;
    } catch (std::exception const& exc) {
        std::cerr << "Unhandled std::exception: " << exc.what() << std::endl;
    } catch (...) {
        std::cerr << "Unhandled unknown exception" << std::endl;
    }
    return 1;
}
