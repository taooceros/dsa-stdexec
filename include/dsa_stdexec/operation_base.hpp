#pragma once
#ifndef DSA_STDEXEC_OPERATION_BASE_HPP
#define DSA_STDEXEC_OPERATION_BASE_HPP

#include <proxy/proxy.h>

namespace dsa_stdexec {

PRO_DEF_MEM_DISPATCH(CheckCompletion, check_completion);
PRO_DEF_MEM_DISPATCH(Notify, notify);

struct OperationFacade
    : pro::facade_builder ::add_convention<
          CheckCompletion, bool()>::add_convention<Notify, void()>::build {};

struct OperationBase {
  pro::proxy<OperationFacade> proxy;
  OperationBase *next = nullptr;
};

} // namespace dsa_stdexec

#endif
