#include "src/core/resume_ticket.h"

namespace blue
{
    void try_resume(const std::shared_ptr<ResumeTicket> &rt)
    {
        if (!rt->m_armed.exchange(false, std::memory_order_acq_rel))
        {
            return;   // 已经 resume 过，或 AsyncIo 析构时置了 false
        }
        if (rt->m_handle && rt->m_handle.address() && !rt->m_handle.done())
        {
            rt->m_handle.resume();
        }
    }
}