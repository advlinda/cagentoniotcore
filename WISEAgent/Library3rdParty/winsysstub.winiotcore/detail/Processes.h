#pragma once

#include "detail/CpuArchitech.h"
#include <windef.h>
#include <WinBase.h>
#include <vector>
#include "detail/Handle.h"

class Processes
{
public:
    using proc_id         = DWORD;
    using proc_id_vec     = std::vector<proc_id>;
    using proc_handle_vec = std::vector<mstc::tckernel::Handle>;

    static void id2Handle(const proc_id_vec&, proc_handle_vec&);
};
