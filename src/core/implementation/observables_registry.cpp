
#include "observables_registry.hpp"


const QSet<ObservableExecutor *>& ObservablesRegistry::executors() const
{
    return m_executors;
}


void ObservablesRegistry::add(ObservableExecutor* executor)
{
    m_executors.insert(executor);
}



void ObservablesRegistry::remove(ObservableExecutor* executor)
{
    m_executors.remove(executor);
}
