#include <iomanip>
#include <iostream>
#include <mutex>
#include <atomic>
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


/// @brief Generator of sequential ID
template<typename id_type = unsigned>
class ID_generator
{
public:
    ID_generator()
        : m_id(0)
    {}
    id_type
        operator()()
    {
        return m_id++;
    }

private:
    std::atomic<id_type> m_id;
};

/// @brief Objects with sequential ID
template<typename Tag, typename id_type = unsigned>
class Object_with_id
{
public:
    Object_with_id()
        : m_id(m_generator())
    {}
    id_type id()const
    {
        return m_id;
    }

private:
    static ID_generator<id_type> m_generator;
    id_type m_id;
};

template<typename Tag, typename id_type>
ID_generator<id_type> Object_with_id<Tag, id_type>::m_generator;


/// Fork is simple mutex
typedef std::mutex Fork;

class Canteen;

class Philosopher
    : public Object_with_id<Philosopher>
{
public:
    enum class States
    {
        thinks,
        hungry,
        dines
    };
    Philosopher(Canteen* _p_canteen, Fork* _p_left, Fork* _p_right)
        : m_state(States::thinks)
        , m_p_canteen(_p_canteen)
        , m_p_left(_p_left)
        , m_p_right(_p_right)
    {}

    States state()const
    {
        return m_state;
    }

    void operator()()
    {
        for (;;) {
            state(States::thinks);
            std::this_thread::sleep_for(random_interval());

            {
                state(States::hungry);
                std::unique_lock<decltype(m_mutex)> locker(m_mutex);
                while (std::try_lock(*m_p_left, *m_p_right) != -1) {
                    m_cv.wait(locker);
                }
            }
            state(States::dines);
            std::this_thread::sleep_for(random_interval());
            m_p_left->unlock();
            m_p_right->unlock();
        }
    }
    static void worker(Philosopher* p_philosopfer)
    {
        (*p_philosopfer)();
    }
private:
    static std::chrono::milliseconds
        random_interval()
    {
        static std::uniform_int_distribution<unsigned> distrigution(1, 10000);
        auto const rnd = distrigution(g_rnd);
        return std::chrono::milliseconds(rnd);
    }
    void state(States _state);

    std::atomic<States> m_state;
    static std::mutex m_mutex;
    static std::condition_variable m_cv;
    Canteen* m_p_canteen;
    Fork* m_p_left;
    Fork* m_p_right;
};
std::mutex Philosopher::m_mutex;
std::condition_variable Philosopher::m_cv;

/// Canteen with simple text log
class Canteen
{
    typedef std::pair<unsigned, Philosopher::States> state_log_element_type;
public:
    explicit Canteen(unsigned _number_of_philosophers = 5)
        : m_number_of_philosophers(_number_of_philosophers)
    {}
    void log_state(Philosopher const* _p_philosopher)
    {
        std::unique_lock<decltype(m_log_queue_mutex)> locker(m_log_queue_mutex);
        m_log_queue.emplace_back(_p_philosopher->id(), _p_philosopher->state());
        m_cv.notify_one();
    }
    unsigned number_of_philosophers() const
    {
        return m_number_of_philosophers;
    }
    void operator()()
    {
        std::vector<std::unique_ptr<Fork> > forks;
        std::generate_n(std::back_inserter(forks), number_of_philosophers(), []() {return std::make_unique<Fork>(); });
        std::vector<std::unique_ptr<Philosopher> > philosophers;
        for (unsigned i = 0; i < number_of_philosophers(); ++i) {
            Fork* const p_left = forks[i].get();
            Fork* const p_right = forks[(i + 1) % number_of_philosophers()].get();
            philosophers.push_back(std::make_unique<Philosopher>(this, p_left, p_right));
        }

        std::vector<std::thread> threads;
        threads.reserve(number_of_philosophers());
        std::transform(philosophers.cbegin(), philosophers.cend(), std::back_inserter(threads),
                       [](decltype(philosophers)::value_type const& ptr) {return std::thread(Philosopher::worker, ptr.get()); });
        log_worker();
    }

protected:
    typedef std::vector<state_log_element_type> log_queue_type;
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

private:
    void log_worker()
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
    
    unsigned m_number_of_philosophers;
    std::mutex m_log_queue_mutex;
    std::condition_variable m_cv;
    log_queue_type m_log_queue;
};

/// Canteen with waterfall states log
class Canteen_waterfall
    : public Canteen
{
public:
    Canteen_waterfall(unsigned _number_of_philosophers = 5)
        : Canteen(_number_of_philosophers)
        , m_buffer(_number_of_philosophers, symb(Philosopher::States::thinks))
    {}
    virtual void monitor(log_queue_type const& work_log)override
    {
        for (auto el : work_log) {
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
            return '*';
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

void Philosopher::state(States _state)
{
    std::unique_lock<decltype(m_mutex)> locker(m_mutex);
    m_state = _state;
    m_p_canteen->log_state(this);
    m_cv.notify_all();
}
}  // namespace philosophers

int main(int argc, char* argv[])
{
    try {
        unsigned const num_ph = argc < 2 ? 5 : atoi(argv[1]);
        philosophers::Canteen_waterfall canteen(num_ph);
        canteen();
        return 0;
    } catch (std::exception const& exc) {
        std::cerr << "Unhandled std::exception: " << exc.what() << std::endl;
    } catch (...) {
        std::cerr << "Unhandled unknown exception" << std::endl;
    }
    return 1;
}
