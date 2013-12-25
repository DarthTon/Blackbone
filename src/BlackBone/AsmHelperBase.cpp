#include "AsmHelperBase.h"

namespace ds_mmap
{
    CAsmHelperBase::CAsmHelperBase(AsmJit::Assembler& _a)
        : a(_a)
    {
    }

    CAsmHelperBase::~CAsmHelperBase(void)
    {
    }
}
