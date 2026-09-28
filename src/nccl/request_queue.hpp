/*
 * ghex-org
 *
 * Copyright (c) 2014-2026, ETH Zurich
 * All rights reserved.
 *
 * Please, refer to the LICENSE file in the root directory.
 * SPDX-License-Identifier: BSD-3-Clause
 */
#pragma once

#include <vector>

#include <boost/lockfree/queue.hpp>

// paths relative to backend
#include "request_state.hpp"

namespace oomph
{
class request_queue
{
  private:
    using element_type = detail::request_state;
    using queue_type = std::vector<element_type*>;

  private: // members
    queue_type m_queue;
    queue_type m_ready_queue;
    bool       in_progress = false;

  public: // ctors
    request_queue()
    {
        m_queue.reserve(256);
        m_ready_queue.reserve(256);
    }

  public: // member functions
    std::size_t size() const noexcept { return m_queue.size(); }

    void enqueue(element_type* e)
    {
        m_queue.push_back(e);
    }

    int progress()
    {
        if (in_progress) return 0;
        in_progress = true;

        const auto qs = size();
        if (qs == 0)
        {
            in_progress = false;
            return 0;
        }

        m_ready_queue.clear();
        m_ready_queue.reserve(qs);
        std::size_t num_not_ready = 0;
        for (std::size_t i = 0; i < qs; ++i)
        {
            auto* req = m_queue[i];
            if (req->m_req.is_ready()) { m_ready_queue.push_back(req); }
            else
            {
                if (num_not_ready != i)
                {
                    m_queue[num_not_ready] = req;
                }
                ++num_not_ready;
            }
        }
        m_queue.erase(m_queue.begin() + num_not_ready, m_queue.end());

        int completed = m_ready_queue.size();
        for (auto* req : m_ready_queue)
        {
            auto ptr = req->release_self_ref();
            req->invoke_cb();
        }

        in_progress = false;

        return completed;
    }

    bool cancel(element_type*) { return false; }
};

class shared_request_queue
{
  private:
    using element_type = detail::shared_request_state;
    using queue_type = boost::lockfree::queue<element_type*, boost::lockfree::fixed_sized<false>,
        boost::lockfree::allocator<std::allocator<void>>>;

  private: // members
    queue_type               m_queue;
    std::atomic<std::size_t> m_size;

  public: // ctors
    shared_request_queue()
    : m_queue(256)
    , m_size(0)
    {
    }

  public: // member functions
    std::size_t size() const noexcept { return m_size.load(); }

    void enqueue(element_type* e)
    {
        m_queue.push(e);
        ++m_size;
    }

    int progress()
    {
        static thread_local bool                       in_progress = false;
        static thread_local std::vector<element_type*> m_local_queue;
        int                                            found = 0;

        if (in_progress) return 0;
        in_progress = true;

        element_type* e;
        while (m_queue.pop(e))
        {
            if (e->m_req.is_ready())
            {
                found = 1;
                break;
            }
            else { m_local_queue.push_back(e); }
        }

        for (auto x : m_local_queue) m_queue.push(x);
        m_local_queue.clear();

        if (found) { --m_size; }

        in_progress = false;

        if (found)
        {
            auto ptr = e->release_self_ref();
            e->invoke_cb();
        }

        return found;
    }

    bool cancel(element_type*) { return false; }
};
} // namespace oomph
