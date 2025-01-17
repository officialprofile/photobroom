
#ifndef TASK_EXECUTOR_UTILS
#define TASK_EXECUTOR_UTILS

#include <deque>
#include <mutex>
#include <future>
#include <condition_variable>
#include <source_location>
#include <QFuture>
#include <QPromise>

#include "itask_executor.hpp"

#include "core_export.h"


template<typename T, typename E>
struct ExecutorTraits
{
    static void exec(E &, T &&);
};


class abort_exception: public std::exception {};


class CORE_EXPORT WorkState
{
    public:
        WorkState() = default;
        WorkState(const WorkState &) = delete;
        WorkState& operator=(const WorkState &) = delete;

        /**
         * @brief Set state to abort
         */
        void abort();

        /**
         * @brief Throw exception if state was set to abort
         * @throw abort_exception
         */
        void throwIfAbort();

    private:
        bool m_abort = false;
};


// Helper function.
// Run a task and wait for it to be finished.
template<typename R, typename E, typename T>
auto evaluate(E& executor, const T& task)
{
    typedef std::packaged_task<R> PTask;

    PTask p_task(task);

    auto result_future = p_task.get_future();
    ExecutorTraits<E, PTask>::exec(executor, std::move(p_task));

    result_future.wait();

    const auto result = result_future.get();

    return result;
}


// Helper function.
// Run a task and do not wait for it to be finished.
template<typename E, typename T>
void execute(E& executor, T&& task)
{
    ExecutorTraits<E, T>::exec(executor, std::move(task));
}


// Run callable as a task
template<typename Callable>
void runOn(ITaskExecutor& executor, Callable&& callable, const std::string& taskName = std::source_location::current().function_name())
    requires std::is_invocable_v<Callable>
{
    struct GenericTask: ITaskExecutor::ITask
    {
        GenericTask(const std::string& name, Callable&& callable)
            : m_callable(std::forward<Callable>(callable))
            , m_name(name)
        {

        }

        std::string name() const override
        {
            return m_name;
        }

        void perform() override
        {
            m_callable();
        }

        private:
            typename std::remove_reference<Callable>::type m_callable;
            std::string m_name;
    };

    auto task = std::make_unique<GenericTask>(taskName, std::forward<Callable>(callable));
    executor.add(std::move(task));
}


// Run callable as a task
template<typename R, typename Callable>
QFuture<R> runOn(ITaskExecutor& executor, Callable&& callable, const std::string& taskName = std::source_location::current().function_name())
{
    struct GenericTask: ITaskExecutor::ITask
    {
        GenericTask(const std::string& name, Callable&& callable, QPromise<R>&& p)
            : m_callable(std::forward<Callable>(callable))
            , m_name(name)
            , m_promise(std::move(p))
        {

        }

        std::string name() const override
        {
            return m_name;
        }

        void perform() override
        {
            m_promise.start();
            m_callable(m_promise);
            m_promise.finish();
        }

        private:
            typename std::remove_reference<Callable>::type m_callable;
            std::string m_name;
            QPromise<R> m_promise;
    };

    QPromise<R> promise;
    auto future = promise.future();

    auto task = std::make_unique<GenericTask>(taskName, std::forward<Callable>(callable), std::move(promise));
    executor.add(std::move(task));

    return future;
}


/**
 * @brief A subqueue for ITaskExecutor.
 *
 * Purpose of this class is to have a queue of tasks to be executed by executor
 * but controled by client (can be cleaned)
 *
 * @see clear
 */
class CORE_EXPORT TasksQueue: public ITaskExecutor
{
    public:

        /// Determines queue behavior
        enum class Mode
        {
            Fifo,               ///< Execute tasks in order of insertion. First inserted will be first to execute.
            Lifo,               ///< Execute tasks in reversed order of insertion. Last inserted will be first to execute.
        };

        TasksQueue(ITaskExecutor *, Mode = Mode::Fifo);
        ~TasksQueue();

        /**
         * @brief Insert new task to queue.
         */
        void push(std::unique_ptr<ITaskExecutor::ITask> &&);

        /**
         * @brief Remove queued items.
         *
         * Remove tasks staying in queue. Tasks already being executed are not touched.
         */
        void clear();

        /**
         * @returns number of items wating in queue. Tasks being executed are not counted in.
         */
        std::size_t size() const;

        /**
         * @brief Wait for tasks being executed.
         *
         * Blocks current thread until all tasks being executed are done.
         */
        void waitForPendingTasks();

        void add(std::unique_ptr<ITask> &&) override;
        void addLight(std::unique_ptr<ITask> &&) override;
        int heavyWorkers() const override;

    private:
        friend struct IntTask;
        struct IntTask;

        std::recursive_mutex m_tasksMutex;
        std::deque<std::unique_ptr<ITaskExecutor::ITask>> m_waitingTasks;
        std::condition_variable_any m_noWork;
        ITaskExecutor* m_tasksExecutor;
        int m_maxTasks;
        int m_executingTasks;
        const Mode m_mode;

        void try_to_fire();
        void fire();
        void task_finished();
};


CORE_EXPORT std::unique_ptr<ITaskExecutor::ITask> inlineTask(const std::string& name, std::function<void()>&& task);

#endif
