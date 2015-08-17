/*
 * Tree container - base for tree iterators
 * Copyright (C) 2015  Michał Walenciak <MichalWalenciak@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef TREE_ITERATOR_BASE_HPP
#define TREE_ITERATOR_BASE_HPP

#include <cassert>
#include <stack>

#include "tree_private.hpp"

template<typename T> class tree;


namespace tree_private
{

    template<IteratorType iteratorType, typename T>
    class iterator_base: public std::iterator<std::bidirectional_iterator_tag, T>
    {
            typedef tree_private::node_pointer<T> node_pointer;

        protected:
            struct Empty {};

            // copy constructors types:

            // copy from NonConst is always ok
            typedef iterator_base<IteratorType::NonConst, T> CopyT1;

            // copy from Const is only allowed to another Const. For NonConst use Empty
            typedef typename std::conditional<iteratorType == IteratorType::Const, iterator_base<IteratorType::Const, T>, Empty>::type CopyT2;

        public:
            typedef typename std::conditional<iteratorType == IteratorType::Const, const T, T>::type ValueType;

            iterator_base(const node_pointer& b): m_iterators()
            {
                m_iterators.push(b);
            }

            // copy from NonConst to NonConst and from NonConst to Const
            iterator_base(const CopyT1& other): m_iterators(other.m_iterators) { }

            iterator_base(const CopyT2& other): m_iterators(other.m_iterators) { }

            ~iterator_base() {}

            iterator_base& operator=(const iterator_base& other)
            {
                m_iterators = other.m_iterators;

                return *this;
            }

            bool operator==(const iterator_base& other) const
            {
                return same(other);
            }

            bool operator!=(const iterator_base& other) const
            {
                return !same(other);
            }

            const ValueType& operator*() const
            {
                assert(valid());
                return **current();
            }

            ValueType& operator*()
            {
                assert(valid());
                return **current();
            }

            const ValueType& operator&() const
            {
                assert(valid());
                return **current();
            }

            const ValueType* operator->() const
            {
                assert(valid());
                return &(**current());
            }

            ValueType* operator->()
            {
                assert(valid());
                return &(**current());
            }

            bool valid() const
            {
                const bool status = current() != nodes_end();

                return status;
            }

        protected:
            template<typename> friend class tree;
            template<IteratorType, typename> friend class iterator_base;

            std::stack<node_pointer> m_iterators;

            node_pointer nodes_begin() const
            {
                auto ns = current().get_nodes_list();
                node_pointer result(ns, ns->begin());
                return result;
            }

            const node_pointer& current() const
            {
                return m_iterators.top();
            }

            node_pointer& current()
            {
                return m_iterators.top();
            }

            node_pointer nodes_end() const
            {
                auto ns = current().get_nodes_list();
                node_pointer result(ns, ns->end());
                return result;
            }

            bool same(const iterator_base& other) const
            {
                bool status = true;

                //same depth?
                if (m_iterators.size() != other.m_iterators.size())
                    status = false;
                else
                {
                    //check top items (most deep ones) if they point to the same list of nodes and to the same position
                    const node_pointer& t_i = m_iterators.top();
                    const node_pointer& o_i = other.m_iterators.top();

                    status = t_i.m_nodes == o_i.m_nodes &&
                             t_i.m_node  == o_i.m_node;
                }

                return status;
            }
    };

}

#endif // TREE_ITERATOR_BASE_HPP
